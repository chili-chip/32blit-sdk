#include <cstdlib>
#include <math.h>

#include "display.hpp"
#include "display_commands.hpp"

#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "pico/binary_info.h"
#include "pico/time.h"

#include "config.h"

#ifdef DBI_8BIT
#include "dbi-8bit.pio.h"
#else
#include "dbi-spi.pio.h"
#endif

using namespace blit;

enum SSD1351 : uint8_t {
    SET_COLUMN        = 0x15, ///< See datasheet
    SET_ROW           = 0x75, ///< See datasheet
    WRITE_RAM         = 0x5C, ///< See datasheet
    READ_RAM          = 0x5D, ///< Not currently used
    SET_REMAP         = 0xA0, ///< See datasheet
    START_LINE        = 0xA1, ///< See datasheet
    DISPLAY_OFFSET    = 0xA2, ///< See datasheet
    DISPLAY_ALL_OFF   = 0xA4, ///< Not currently used
    DISPLAY_ALL_ON    = 0xA5, ///< Not currently used
    NORMAL_DISPLAY    = 0xA6, ///< See datasheet
    INVERT_DISPLAY    = 0xA7, ///< See datasheet
    FUNCTION_SELECT   = 0xAB, ///< See datasheet
    DISPLAY_OFF       = 0xAE, ///< See datasheet
    DISPLAY_ON        = 0xAF, ///< See datasheet
    PRECHARGE         = 0xB1, ///< See datasheet
    DISPLAY_ENHANCE   = 0xB2, ///< Not currently used
    CLOCK_DIV         = 0xB3, ///< See datasheet
    SET_VSL           = 0xB4, ///< See datasheet
    SET_GPIO          = 0xB5, ///< See datasheet
    PRECHARGE_2       = 0xB6, ///< See datasheet
    SET_GRAY          = 0xB8, ///< Not currently used
    USE_LUT           = 0xB9, ///< Not currently used
    PRECHARGE_LEVEL   = 0xBB, ///< Not currently used
    VCOMH             = 0xBE, ///< See datasheet
    CONTRAST_ABC      = 0xC1, ///< See datasheet
    CONTRAST_MASTER   = 0xC7, ///< See datasheet
    MUX_RATIO         = 0xCA, ///< See datasheet
    COMMAND_LOCK      = 0xFD, ///< See datasheet
    HORIZ_SCROLL      = 0x96, ///< Not currently used
    STOP_SCROLL       = 0x9E, ///< Not currently used
    START_SCROLL      = 0x9F, ///< Not currently used
};

//rotation
#ifndef LCD_ROTATION
#define LCD_ROTATION 0
#endif

static uint8_t rotation = LCD_ROTATION;

// double buffering for lores
static volatile int buf_index = 0;

static volatile bool do_render = true;

static uint32_t last_render = 0;

static PIO pio = pio0;
static uint pio_sm = 0;
static uint pio_offset = 0, pio_double_offset = 0;

static uint32_t dma_channel = 0;

static uint16_t win_w, win_h; // window size

static bool write_mode = false; // in RAMWR
static bool pixel_double = false;
static uint16_t *upd_frame_buffer = nullptr;

// frame buffer where pixel data is stored
static uint16_t *frame_buffer = nullptr;

// pixel double scanline counter
static volatile int cur_scanline = DISPLAY_HEIGHT;

// PIO helpers
static void pio_put_byte(PIO pio, uint sm, uint8_t b) {
  while (pio_sm_is_tx_fifo_full(pio, sm));
  *(volatile uint8_t*)&pio->txf[sm] = b;
}

static void pio_wait(PIO pio, uint sm) {
  uint32_t stall_mask = 1u << (PIO_FDEBUG_TXSTALL_LSB + sm);
  pio->fdebug |= stall_mask;
  while(!(pio->fdebug & stall_mask));
}

// used for pixel doubling
static void __isr dbi_dma_irq_handler() {
  if(dma_channel_get_irq0_status(dma_channel)) {
    dma_channel_acknowledge_irq0(dma_channel);

    if(++cur_scanline > win_h / 2)
      return;

    auto count = cur_scanline == (win_h + 1) / 2 ? win_w / 4  : win_w / 2;

    dma_channel_set_trans_count(dma_channel, count, false);
    dma_channel_set_read_addr(dma_channel, upd_frame_buffer + (cur_scanline - 1) * (win_w / 2), true);
  }
}

static void command(uint8_t command, size_t len = 0, const char *data = nullptr) {
  pio_wait(pio, pio_sm);

  if(write_mode) {
    // reconfigure to 8 bits
    pio_sm_set_enabled(pio, pio_sm, false);
    pio->sm[pio_sm].shiftctrl &= ~PIO_SM0_SHIFTCTRL_PULL_THRESH_BITS;
    pio->sm[pio_sm].shiftctrl |= (8 << PIO_SM0_SHIFTCTRL_PULL_THRESH_LSB) | PIO_SM0_SHIFTCTRL_AUTOPULL_BITS;

    // switch back to raw
    pio_sm_restart(pio, pio_sm);
    pio_sm_set_wrap(pio, pio_sm, pio_offset + dbi_raw_wrap_target, pio_offset + dbi_raw_wrap);
    pio_sm_exec(pio, pio_sm, pio_encode_jmp(pio_offset));

    pio_sm_set_enabled(pio, pio_sm, true);
    write_mode = false;
  }

  gpio_put(LCD_CS_PIN, 0);

  gpio_put(LCD_DC_PIN, 0); // command mode
  pio_put_byte(pio, pio_sm, command);

  if(data) {
    pio_wait(pio, pio_sm);
    gpio_put(LCD_DC_PIN, 1); // data mode

    for(size_t i = 0; i < len; i++)
      pio_put_byte(pio, pio_sm, data[i]);
  }

  pio_wait(pio, pio_sm);
  gpio_put(LCD_CS_PIN, 1);
}

#define ssd1351_swap(a, b)                                                     \
  (((a) ^= (b)), ((b) ^= (a)), ((a) ^= (b))) ///< No-temp-var swap operation


static void set_window(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h) {
  uint16_t x2 = x1 + w - 1, y2 = y1 + h - 1;
  if (rotation & 1) { // Vertical address increment mode
    ssd1351_swap(x1, y1);
    ssd1351_swap(x2, y2);
  }
  uint8_t cols[2] = { static_cast<uint8_t>(x1 & 0xFF), static_cast<uint8_t>(x2 & 0xFF) };
  uint8_t rows[2] = { static_cast<uint8_t>(y1 & 0xFF), static_cast<uint8_t>(y2 & 0xFF) };
  command(SSD1351::SET_COLUMN, 2, reinterpret_cast<const char *>(cols));
  command(SSD1351::SET_ROW, 2, reinterpret_cast<const char *>(rows));
  command(SSD1351::WRITE_RAM);
}

void set_rotation(uint8_t r) {
  // madctl bits:
  // 6,7 Color depth (01 = 64K)
  // 5   Odd/even split COM (0: disable, 1: enable)
  // 4   Scan direction (0: top-down, 1: bottom-up)
  // 3   Reserved
  // 2   Color remap (0: A->B->C, 1: C->B->A)
  // 1   Column remap (0: 0-127, 1: 127-0)
  // 0   Address increment (0: horizontal, 1: vertical)
  uint8_t madctl = 0b01100100; // 64K, enable split, CBA

  rotation = r & 3; // Clip input to valid range

  switch (rotation) {
  case 0:
    madctl |= 0b00010000; // Scan bottom-up
    win_w = DISPLAY_WIDTH;
    win_h = DISPLAY_HEIGHT;
    break;
  case 1:
    madctl |= 0b00010011; // Scan bottom-up, column remap 127-0, vertical
    win_w = DISPLAY_WIDTH;
    win_h = DISPLAY_HEIGHT;
    break;
  case 2:
    madctl |= 0b00000010; // Column remap 127-0
    win_w = DISPLAY_WIDTH;
    win_h = DISPLAY_HEIGHT;
    break;
  case 3:
    madctl |= 0b00000001; // Vertical
    win_w = DISPLAY_WIDTH;
    win_h = DISPLAY_HEIGHT;
    break;
  }

  command(SSD1351::SET_REMAP, 1, reinterpret_cast<const char *>(&madctl));
  // Start line must be within 0..127 for 128x128; use 0 to avoid wrap issues
  uint8_t startline = 0;
  command(SSD1351::START_LINE, 1, reinterpret_cast<const char *>(&startline));
}

static void send_init_sequence() {

  command(SSD1351::COMMAND_LOCK, 1, "\x12"); // COMMANDLOCK
  command(SSD1351::COMMAND_LOCK, 1, "\xB1"); // COMMANDLOCK
  command(SSD1351::DISPLAY_OFF);

  command(SSD1351::CLOCK_DIV, 1, "\xF1"); // CLOCKDIV
  command(SSD1351::MUX_RATIO, 1, "\x7F"); // MUXRATIO (127)
  command(SSD1351::DISPLAY_OFFSET, 1, "\x00"); // DISPLAYOFFSET
  command(SSD1351::SET_GPIO, 1, "\x00"); // SETGPIO
  command(SSD1351::FUNCTION_SELECT, 1, "\x01"); // FUNCTIONSELECT (internal)
  command(SSD1351::PRECHARGE, 1, "\x32"); // PRECHARGE
  command(SSD1351::VCOMH, 1, "\x05"); // VCOMH
  command(SSD1351::NORMAL_DISPLAY);            // NORMALDISPLAY

  command(SSD1351::CONTRAST_ABC, 3, "\xC8\x80\xC8"); // CONTRASTABC
  command(SSD1351::CONTRAST_MASTER, 1, "\x0F");         // CONTRASTMASTER
  command(SSD1351::SET_VSL, 3, "\xA0\xB5\x55"); // SETVSL (B4)
  command(SSD1351::PRECHARGE_2, 1, "\x01");         // PRECHARGE2
  command(SSD1351::DISPLAY_ON);                    // DISPLAYON

  set_rotation(0);

  // finalize window
  set_window(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
}

static void prepare_write() {
  pio_wait(pio, pio_sm);

  uint8_t r = SSD1351::WRITE_RAM;
  gpio_put(LCD_CS_PIN, 0);

  gpio_put(LCD_DC_PIN, 0); // command mode
  pio_put_byte(pio, pio_sm, r);
  pio_wait(pio, pio_sm);

  gpio_put(LCD_DC_PIN, 1); // data mode

  pio_sm_set_enabled(pio, pio_sm, false);
  pio_sm_restart(pio, pio_sm);

  if(pixel_double) {
    pio_sm_set_wrap(pio, pio_sm, pio_double_offset + dbi_pixel_double_wrap_target, pio_double_offset + dbi_pixel_double_wrap);
    pio->sm[pio_sm].shiftctrl &= ~(PIO_SM0_SHIFTCTRL_PULL_THRESH_BITS | PIO_SM0_SHIFTCTRL_AUTOPULL_BITS);
    pio_sm_exec(pio, pio_sm, pio_encode_jmp(pio_double_offset));

    dma_channel_hw_addr(dma_channel)->al1_ctrl &= ~DMA_CH0_CTRL_TRIG_DATA_SIZE_BITS;
    dma_channel_hw_addr(dma_channel)->al1_ctrl |= DMA_SIZE_32 << DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB;
  } else {
    pio->sm[pio_sm].shiftctrl &= ~PIO_SM0_SHIFTCTRL_PULL_THRESH_BITS;
    pio->sm[pio_sm].shiftctrl |= (16 << PIO_SM0_SHIFTCTRL_PULL_THRESH_LSB) | PIO_SM0_SHIFTCTRL_AUTOPULL_BITS;

    dma_channel_hw_addr(dma_channel)->al1_ctrl &= ~DMA_CH0_CTRL_TRIG_DATA_SIZE_BITS;
    dma_channel_hw_addr(dma_channel)->al1_ctrl |= DMA_SIZE_16 << DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB;
  }

  pio_sm_set_enabled(pio, pio_sm, true);

  write_mode = true;
}

static void update() {
  dma_channel_wait_for_finish_blocking(dma_channel);

  // update window if needed
  auto expected_win = cur_surf_info.bounds * (pixel_double ? 2 : 1);

  if(expected_win.w != win_w || expected_win.h != win_h)
    set_window((DISPLAY_WIDTH - expected_win.w) / 2, (DISPLAY_HEIGHT - expected_win.h) / 2, expected_win.w, expected_win.h);

  if(!write_mode)
    prepare_write();

  if(pixel_double) {
    cur_scanline = 0;
    upd_frame_buffer = frame_buffer;
    dma_channel_set_trans_count(dma_channel, win_w / 4, false);
  } else
    dma_channel_set_trans_count(dma_channel, win_w * win_h, false);

  dma_channel_set_read_addr(dma_channel, frame_buffer, true);
}

static void set_pixel_double(bool pd) {
  pixel_double = pd;

  // nop to reconfigure PIO
  if(write_mode)
    command(0);

  if(pixel_double) {
    dma_channel_acknowledge_irq0(dma_channel);
    dma_channel_set_irq0_enabled(dma_channel, true);
  } else
    dma_channel_set_irq0_enabled(dma_channel, false);
}

static void clear() {
  if(!write_mode)
    prepare_write();

  for(int i = 0; i < win_w * win_h; i++)
    pio_sm_put_blocking(pio, pio_sm, 0);
}

static bool dma_is_busy() {
  if(pixel_double && cur_scanline <= win_h / 2)
    return true;

  return dma_channel_is_busy(dma_channel);
}

void init_display() {
  frame_buffer = screen_fb;

  // configure pins
  gpio_set_function(LCD_DC_PIN, GPIO_FUNC_SIO);
  gpio_set_dir(LCD_DC_PIN, GPIO_OUT);

  gpio_set_function(LCD_CS_PIN, GPIO_FUNC_SIO);
  gpio_set_dir(LCD_CS_PIN, GPIO_OUT);
  gpio_put(LCD_CS_PIN, 1);

  bi_decl_if_func_used(bi_1pin_with_name(LCD_DC_PIN, "Display D/C"));
  bi_decl_if_func_used(bi_1pin_with_name(LCD_CS_PIN, "Display CS"));

#ifdef DBI_8BIT
  // init RD
  gpio_init(LCD_RD_PIN);
  gpio_set_dir(LCD_RD_PIN, GPIO_OUT);
  gpio_put(LCD_RD_PIN, 1);

  bi_decl_if_func_used(bi_1pin_with_name(LCD_RD_PIN, "Display RD"));
#endif

#ifdef LCD_RESET_PIN
  gpio_set_function(LCD_RESET_PIN, GPIO_FUNC_SIO);
  gpio_set_dir(LCD_RESET_PIN, GPIO_OUT);
  gpio_put(LCD_RESET_PIN, 0);
  sleep_ms(100);
  gpio_put(LCD_RESET_PIN, 1);

  bi_decl_if_func_used(bi_1pin_with_name(LCD_RESET_PIN, "Display Reset"));
#endif

  // setup PIO
  pio_offset = pio_add_program(pio, &dbi_raw_program);
  pio_double_offset = pio_add_program(pio, &dbi_pixel_double_program);

  pio_sm = pio_claim_unused_sm(pio, true);

  pio_sm_config cfg = dbi_raw_program_get_default_config(pio_offset);

#ifdef DBI_8BIT
  const int out_width = 8;
#else // SPI
  const int out_width = 1;
#endif

  const int clkdiv = std::ceil(clock_get_hz(clk_sys) / float(LCD_MAX_CLOCK * 2));
  sm_config_set_clkdiv(&cfg, clkdiv);

  sm_config_set_out_shift(&cfg, false, true, 8);
  sm_config_set_out_pins(&cfg, LCD_MOSI_PIN, out_width);
  sm_config_set_fifo_join(&cfg, PIO_FIFO_JOIN_TX);
  sm_config_set_sideset_pins(&cfg, LCD_SCK_PIN);

  // init pins
  for(int i = 0; i < out_width; i++)
    pio_gpio_init(pio, LCD_MOSI_PIN + i);

  pio_gpio_init(pio, LCD_SCK_PIN);

  pio_sm_set_consecutive_pindirs(pio, pio_sm, LCD_MOSI_PIN, out_width, true);
  pio_sm_set_consecutive_pindirs(pio, pio_sm, LCD_SCK_PIN, 1, true);

  pio_sm_init(pio, pio_sm, pio_offset, &cfg);
  pio_sm_set_enabled(pio, pio_sm, true);

#ifdef DBI_8BIT
  // these are really D0/WR
  bi_decl_if_func_used(bi_pin_mask_with_name(0xFF << LCD_MOSI_PIN, "Display Data"));
  bi_decl_if_func_used(bi_1pin_with_name(LCD_SCK_PIN, "Display WR"));
#else
  bi_decl_if_func_used(bi_1pin_with_name(LCD_MOSI_PIN, "Display TX"));
  bi_decl_if_func_used(bi_1pin_with_name(LCD_SCK_PIN, "Display SCK"));
#endif

  // send initialisation sequence for our standard displays based on the width and height
  send_init_sequence();

  // initialise dma channel for transmitting pixel data to screen
  dma_channel = dma_claim_unused_channel(true);
  dma_channel_config config = dma_channel_get_default_config(dma_channel);
  channel_config_set_transfer_data_size(&config, DMA_SIZE_16);
  channel_config_set_dreq(&config, pio_get_dreq(pio, pio_sm, true));
  dma_channel_configure(
    dma_channel, &config, &pio->txf[pio_sm], frame_buffer, DISPLAY_WIDTH * DISPLAY_HEIGHT, false);

  irq_add_shared_handler(DMA_IRQ_0, dbi_dma_irq_handler, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
  irq_set_enabled(DMA_IRQ_0, true);

  clear();
}

void update_display(uint32_t time) {
  if((do_render || (time - last_render >= 20)) && (fb_double_buffer || !dma_is_busy())) {
    if(fb_double_buffer) {
      buf_index ^= 1;

      screen.data = (uint8_t *)screen_fb + (buf_index) * get_display_page_size();
      frame_buffer = (uint16_t *)screen.data;
    }

    ::render(time);

    while(dma_is_busy()) {} // may need to wait for lores.
    ::update();

    last_render = time;
    do_render = false;
  }
}

void init_display_core1() {
}

void update_display_core1() {
}

bool display_render_needed() {
  return do_render;
}

bool display_mode_supported(blit::ScreenMode new_mode, const blit::SurfaceTemplate &new_surf_template) {
  if(new_surf_template.format != blit::PixelFormat::RGB565)
    return false;

  // allow any size that will fit
  blit::Size expected_bounds(DISPLAY_WIDTH, DISPLAY_HEIGHT);

  if(new_surf_template.bounds.w <= expected_bounds.w && new_surf_template.bounds.h <= expected_bounds.h)
    return true;

  return false;
}

void display_mode_changed(blit::ScreenMode new_mode, blit::SurfaceTemplate &new_surf_template) {
  set_pixel_double(new_mode == ScreenMode::lores);

  if(new_mode == ScreenMode::hires)
    frame_buffer = screen_fb;
}

void enable_display(bool enable) {
  if(enable) {
    command(SSD1351::DISPLAY_ON);
  } else {
    command(SSD1351::DISPLAY_OFF);
  }
}

void invert_display(bool i) {
  if(i) {
    command(SSD1351::INVERT_DISPLAY);
  } else {
    command(SSD1351::NORMAL_DISPLAY);
  }
}