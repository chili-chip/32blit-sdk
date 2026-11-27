// ST7789 320 x 240 SPI panel (EastRising ER-TFT020-3) showing 128 x 128 games.
//
// Games keep the DISPLAY_WIDTH x DISPLAY_HEIGHT (128 x 128) framebuffer. Each
// frame is scaled up with nearest neighbour to fill the panel height and is
// centred: 128 x 128 (and 64 x 64 lores) becomes 240 x 240 with 40 px black
// bars at the sides. One line buffer is filled from the DMA interrupt, and
// only when the source row changes.
//
// Select it with -DBLIT_DISPLAY_DRIVER=dbi_st7789_scaled.

#include <cmath>

#include "display.hpp"
#include "display_commands.hpp"
#include "scale_map.hpp"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "pico/binary_info.h"
#include "pico/time.h"
#include "config.h"
#include "dbi-spi.pio.h"

using namespace blit;

// Panel size after rotation. The ER-TFT020-3 is 240 x 320 portrait and is
// mounted sideways, so it is driven as 320 x 240.
#define PANEL_WIDTH  320
#define PANEL_HEIGHT 240

// 90 or 270, which ever puts the image upright for the way the panel is
// mounted. Can be set in the board config.h.
#ifndef ST7789_ROTATION
#define ST7789_ROTATION 90
#endif

// Set to 1 if red and blue are swapped on the panel.
#ifndef ST7789_BGR
#define ST7789_BGR 0
#endif

// ST7789 write cycle is 16 ns (62.5 MHz). At the 250 MHz system clock the
// PIO divider of 2 gives exactly that: about 15 ms for a 240 x 240 frame.
// Lower it in config.h if long wires to the panel give a noisy picture.
#ifndef ST7789_MAX_CLOCK
#define ST7789_MAX_CLOCK 62500000
#endif

namespace {

PIO pio = pio0;
uint pio_sm = 0;
uint pio_offset = 0;
uint32_t dma_channel = 0;
bool write_mode = false; // in RAMWR

uint16_t *frame_buffer = nullptr; // active frame buffer
uint16_t *src_buffer = nullptr;   // frame being sent
int src_w = DISPLAY_WIDTH, src_h = DISPLAY_HEIGHT;
int out_w = 0, out_h = 0;         // current window size

uint16_t x_map[PANEL_WIDTH];
uint16_t y_map[PANEL_HEIGHT];
uint16_t line_buf[PANEL_WIDTH];
volatile int cur_line = PANEL_HEIGHT; // output line being sent

volatile bool do_render = true;
uint32_t last_render = 0;
int buf_index = 0; // double buffer index

const uint8_t rotations[]{
  0,                                                                                // 0
  MADCTL::HORIZ_ORDER | MADCTL::SWAP_XY | MADCTL::COL_ORDER,                        // 90
  MADCTL::HORIZ_ORDER | MADCTL::SCAN_ORDER | MADCTL::COL_ORDER | MADCTL::ROW_ORDER, // 180
  MADCTL::SCAN_ORDER | MADCTL::SWAP_XY | MADCTL::ROW_ORDER                          // 270
};

enum ST7789Reg {
  PORCTRL   = 0xB2,
  GCTRL     = 0xB7,
  VCOMS     = 0xBB,
  LCMCTRL   = 0xC0,
  VDVVRHEN  = 0xC2,
  VRHS      = 0xC3,
  VDVS      = 0xC4,
  FRCTRL2   = 0xC6,
  PWCTRL1   = 0xD0,
  PVGAMCTRL = 0xE0,
  NVGAMCTRL = 0xE1,
};

void pio_put_byte(uint8_t b) {
  while(pio_sm_is_tx_fifo_full(pio, pio_sm));
  *(volatile uint8_t *)&pio->txf[pio_sm] = b;
}

void pio_wait() {
  uint32_t stall_mask = 1u << (PIO_FDEBUG_TXSTALL_LSB + pio_sm);
  pio->fdebug |= stall_mask;
  while(!(pio->fdebug & stall_mask));
}

void fill_line(int line) {
  const uint16_t *src = src_buffer + y_map[line] * src_w;
  for(int x = 0; x < out_w; x++)
    line_buf[x] = src[x_map[x]];
}

void __isr dma_irq_handler() {
  if(!dma_channel_get_irq0_status(dma_channel))
    return;
  dma_channel_acknowledge_irq0(dma_channel);

  int line = cur_line + 1;
  cur_line = line;
  if(line >= out_h)
    return;

  // The DMA has read the whole buffer, so it can be refilled now.
  if(y_map[line] != y_map[line - 1])
    fill_line(line);
  dma_channel_transfer_from_buffer_now(dma_channel, line_buf, out_w);
}

bool dma_is_busy() {
  return cur_line < out_h || dma_channel_is_busy(dma_channel);
}

void command(uint8_t cmd, size_t len = 0, const char *data = nullptr) {
  pio_wait();

  if(write_mode) {
    // back to 8 bit writes
    pio_sm_set_enabled(pio, pio_sm, false);
    pio->sm[pio_sm].shiftctrl &= ~PIO_SM0_SHIFTCTRL_PULL_THRESH_BITS;
    pio->sm[pio_sm].shiftctrl |= (8 << PIO_SM0_SHIFTCTRL_PULL_THRESH_LSB) | PIO_SM0_SHIFTCTRL_AUTOPULL_BITS;
    pio_sm_restart(pio, pio_sm);
    pio_sm_exec(pio, pio_sm, pio_encode_jmp(pio_offset));
    pio_sm_set_enabled(pio, pio_sm, true);
    write_mode = false;
  }

  gpio_put(LCD_CS_PIN, 0);
  gpio_put(LCD_DC_PIN, 0);
  pio_put_byte(cmd);

  if(data) {
    pio_wait();
    gpio_put(LCD_DC_PIN, 1);
    for(size_t i = 0; i < len; i++)
      pio_put_byte(data[i]);
  }

  pio_wait();
  gpio_put(LCD_CS_PIN, 1);
}

// Start RAMWR and switch the PIO to 16 bit pixels.
void prepare_write() {
  pio_wait();
  gpio_put(LCD_CS_PIN, 0);
  gpio_put(LCD_DC_PIN, 0);
  pio_put_byte(MIPIDCS::WriteMemoryStart);
  pio_wait();
  gpio_put(LCD_DC_PIN, 1);

  pio_sm_set_enabled(pio, pio_sm, false);
  pio_sm_restart(pio, pio_sm);
  pio->sm[pio_sm].shiftctrl &= ~PIO_SM0_SHIFTCTRL_PULL_THRESH_BITS;
  pio->sm[pio_sm].shiftctrl |= (16 << PIO_SM0_SHIFTCTRL_PULL_THRESH_LSB) | PIO_SM0_SHIFTCTRL_AUTOPULL_BITS;
  pio_sm_set_enabled(pio, pio_sm, true);
  write_mode = true;
}

void set_window(int x, int y, int w, int h) {
  uint32_t cols = __builtin_bswap32((x << 16) | (x + w - 1));
  uint32_t rows = __builtin_bswap32((y << 16) | (y + h - 1));
  command(MIPIDCS::SetColumnAddress, 4, (const char *)&cols);
  command(MIPIDCS::SetRowAddress, 4, (const char *)&rows);
}

// Black out the whole panel (side bars included), blocking.
void clear_panel() {
  set_window(0, 0, PANEL_WIDTH, PANEL_HEIGHT);
  prepare_write();
  for(int i = 0; i < PANEL_WIDTH * PANEL_HEIGHT; i++)
    pio_sm_put_blocking(pio, pio_sm, 0);
}

// Scale src_w x src_h to the largest centred window that fits the panel.
void set_source_size(int w, int h) {
  src_w = w;
  src_h = h;
  auto out = fit_scaled(w, h, PANEL_WIDTH, PANEL_HEIGHT);
  make_scale_map(x_map, out.w, w);
  make_scale_map(y_map, out.h, h);

  if(out.w != out_w || out.h != out_h) {
    clear_panel();
    out_w = out.w;
    out_h = out.h;
    set_window((PANEL_WIDTH - out_w) / 2, (PANEL_HEIGHT - out_h) / 2, out_w, out_h);
  }
}

void send_init_sequence() {
  command(MIPIDCS::SoftReset);
  sleep_ms(150);

  command(MIPIDCS::SetPixelFormat, 1, "\x05"); // 16 bits per pixel

  // Same as the 320 x 240 ST7789 in dbi.cpp (Pimoroni Display Pack 2.0)
  command(ST7789Reg::PORCTRL, 5, "\x0c\x0c\x00\x33\x33");
  command(ST7789Reg::GCTRL, 1, "\x35");
  command(ST7789Reg::VCOMS, 1, "\x1f");
  command(ST7789Reg::LCMCTRL, 1, "\x2c");
  command(ST7789Reg::VDVVRHEN, 1, "\x01");
  command(ST7789Reg::VRHS, 1, "\x12");
  command(ST7789Reg::VDVS, 1, "\x20");
  command(ST7789Reg::PWCTRL1, 2, "\xa4\xa1");
  command(ST7789Reg::PVGAMCTRL, 14, "\xD0\x08\x11\x08\x0C\x15\x39\x33\x50\x36\x13\x14\x29\x2D");
  command(ST7789Reg::NVGAMCTRL, 14, "\xD0\x08\x10\x08\x06\x06\x39\x44\x51\x0B\x16\x14\x2F\x31");
  command(ST7789Reg::FRCTRL2, 1, "\x0f"); // 60 Hz

  command(MIPIDCS::EnterInvertMode); // IPS panels need inverted colours
  command(MIPIDCS::ExitSleepMode);
  sleep_ms(120);

  uint8_t madctl = rotations[(ST7789_ROTATION / 90) & 3];
  if(ST7789_BGR)
    madctl |= MADCTL::RGB;
  command(MIPIDCS::SetAddressMode, 1, (const char *)&madctl);

  clear_panel();
  command(MIPIDCS::DisplayOn);
}

void start_frame() {
  while(dma_is_busy());

  auto bounds = cur_surf_info.bounds;
  if(bounds.w != src_w || bounds.h != src_h || !out_w)
    set_source_size(bounds.w, bounds.h);

  if(!write_mode)
    prepare_write();

  src_buffer = frame_buffer;
  cur_line = 0;
  fill_line(0);
  dma_channel_transfer_from_buffer_now(dma_channel, line_buf, out_w);
}

} // namespace

void init_display() {
  frame_buffer = screen_fb;

  gpio_set_function(LCD_DC_PIN, GPIO_FUNC_SIO);
  gpio_set_dir(LCD_DC_PIN, GPIO_OUT);
  gpio_set_function(LCD_CS_PIN, GPIO_FUNC_SIO);
  gpio_set_dir(LCD_CS_PIN, GPIO_OUT);
  gpio_put(LCD_CS_PIN, 1);
  bi_decl_if_func_used(bi_1pin_with_name(LCD_DC_PIN, "Display D/C"));
  bi_decl_if_func_used(bi_1pin_with_name(LCD_CS_PIN, "Display CS"));

  // On the chilichip TFT module RST also switches the backlight on.
  gpio_set_function(LCD_RESET_PIN, GPIO_FUNC_SIO);
  gpio_set_dir(LCD_RESET_PIN, GPIO_OUT);
  gpio_put(LCD_RESET_PIN, 0);
  sleep_ms(10);
  gpio_put(LCD_RESET_PIN, 1);
  sleep_ms(120);
  bi_decl_if_func_used(bi_1pin_with_name(LCD_RESET_PIN, "Display Reset"));

  pio_offset = pio_add_program(pio, &dbi_raw_program);
  pio_sm = pio_claim_unused_sm(pio, true);
  pio_sm_config cfg = dbi_raw_program_get_default_config(pio_offset);
  const int clkdiv = std::ceil(clock_get_hz(clk_sys) / float(ST7789_MAX_CLOCK * 2));
  sm_config_set_clkdiv(&cfg, clkdiv);
  sm_config_set_out_shift(&cfg, false, true, 8);
  sm_config_set_out_pins(&cfg, LCD_MOSI_PIN, 1);
  sm_config_set_fifo_join(&cfg, PIO_FIFO_JOIN_TX);
  sm_config_set_sideset_pins(&cfg, LCD_SCK_PIN);
  pio_gpio_init(pio, LCD_MOSI_PIN);
  pio_gpio_init(pio, LCD_SCK_PIN);
  pio_sm_set_consecutive_pindirs(pio, pio_sm, LCD_MOSI_PIN, 1, true);
  pio_sm_set_consecutive_pindirs(pio, pio_sm, LCD_SCK_PIN, 1, true);
  pio_sm_init(pio, pio_sm, pio_offset, &cfg);
  pio_sm_set_enabled(pio, pio_sm, true);
  bi_decl_if_func_used(bi_1pin_with_name(LCD_MOSI_PIN, "Display TX"));
  bi_decl_if_func_used(bi_1pin_with_name(LCD_SCK_PIN, "Display SCK"));

  send_init_sequence();

  dma_channel = dma_claim_unused_channel(true);
  dma_channel_config config = dma_channel_get_default_config(dma_channel);
  channel_config_set_transfer_data_size(&config, DMA_SIZE_16);
  channel_config_set_dreq(&config, pio_get_dreq(pio, pio_sm, true));
  dma_channel_configure(dma_channel, &config, &pio->txf[pio_sm], line_buf, 0, false);
  irq_add_shared_handler(DMA_IRQ_0, dma_irq_handler, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
  dma_channel_set_irq0_enabled(dma_channel, true);
  irq_set_enabled(DMA_IRQ_0, true);
}

void update_display(uint32_t time) {
  if(!do_render && time - last_render < 20)
    return;

  if(fb_double_buffer) {
    buf_index ^= 1;
    screen.data = (uint8_t *)screen_fb + buf_index * get_display_page_size();
    frame_buffer = (uint16_t *)screen.data;
  }
  render(time);
  start_frame();
  last_render = time;
  do_render = false;
}

void init_display_core1() {}
void update_display_core1() {}

bool display_render_needed() {
  return do_render;
}

bool display_mode_supported(blit::ScreenMode new_mode, const blit::SurfaceTemplate &new_surf_template) {
  if(new_surf_template.format != blit::PixelFormat::RGB565)
    return false;
  return new_surf_template.bounds.w <= DISPLAY_WIDTH && new_surf_template.bounds.h <= DISPLAY_HEIGHT;
}

void display_mode_changed(blit::ScreenMode new_mode, blit::SurfaceTemplate &new_surf_template) {
  if(new_mode == ScreenMode::hires)
    frame_buffer = screen_fb;
}

void set_display_power(bool on) {
  while(dma_is_busy());
  if(on) {
    command(MIPIDCS::ExitSleepMode);
    sleep_ms(120);
    command(MIPIDCS::DisplayOn);
  } else {
    command(MIPIDCS::DisplayOff);
    command(MIPIDCS::EnterSleepMode);
    sleep_ms(5);
  }
  // Next frame re-sends the window and RAMWR.
  out_w = 0;
}
