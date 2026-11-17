# Boards and pins

The console SDK has one board file per version of the console main board. Pick it with `-DPICO_BOARD=...` when you build the OS.

| `PICO_BOARD` | Main board | Display | Sound | Power |
| ------------ | ---------- | ------- | ----- | ----- |
| `chilichip_vgc` | v3.0.0 (current) | 1.5" SSD1351 OLED | MAX98357A audio module, I²S | no switch: [sleep mode](sleep.md) |
| `chilichip_vgc_tft` | v3.0.0 | 1.44" ST7735S TFT ([vgc-zero-hardware #41](https://github.com/chili-chip/vgc-zero-hardware/issues/41)) | same | same |
| `chilichip_vgc_v1` | v1.1.4 (older boards) | 1.5" SSD1351 OLED | PWM on GPIO 22 | hard power switch |

Games don't depend on the board. A `.blit` built with any of them runs on all three, because the drivers live in the loader. Build games with `chilichip_vgc`.

The files are in `32blit-pico/board/`: a Pico SDK board header (`chilichip_vgc.h`) and a folder with `config.h` (pins) and `config.cmake` (drivers). The TFT board reuses the OLED board's files and only changes the display settings.

## Pin map, board v3.0.0

All 26 GPIOs on the RP2350-Plus header are used. The hardware docs have the full [pin map](https://hardware.chilichip.eu/console/pin-map/) with nets and module pads.

| GPIO | Signal | Firmware setting |
| ---- | ------ | ---------------- |
| 0, 1 | Utility module I²C0 SDA, SCL | `PICO_DEFAULT_I2C_SDA_PIN`, `PICO_DEFAULT_I2C_SCL_PIN` |
| 2 | Utility module interrupt | not used yet |
| 3 | Audio I²S data | `AUDIO_I2S_DATA_PIN` |
| 4, 5 | Audio I²S bit clock, word select | `AUDIO_I2S_CLOCK_PIN_BASE` |
| 6 | Amplifier on (high) | `AUDIO_ENABLE_PIN` |
| 7, 8 | A, B | `BUTTON_A_PIN`, `BUTTON_B_PIN` |
| 9 | Radio power (WL_ON, BT_ON) | `RADIO_POWER_PIN`, held low |
| 10, 11 | Y, X | `BUTTON_Y_PIN`, `BUTTON_X_PIN` |
| 12 | Select | `BUTTON_SELECT_PIN` (= HOME) |
| 13, 14, 15 | Radio gSPI CS, CLK, DIO | not used yet ([#64](https://github.com/chili-chip/32blit-sdk/issues/64)) |
| 16, 17 | D-pad down, right | `BUTTON_DOWN_PIN`, `BUTTON_RIGHT_PIN` |
| 18 | Start | `BUTTON_START_PIN` (= MENU), `SLEEP_BUTTON_PIN` |
| 19, 20 | D-pad up, left | `BUTTON_UP_PIN`, `BUTTON_LEFT_PIN` |
| 21 | Display reset | `LCD_RESET_PIN` |
| 22 | Display data / command | `LCD_DC_PIN` |
| 26, 27 | Display clock, data | `LCD_SCK_PIN`, `LCD_MOSI_PIN` |
| 28 | Display chip select | `LCD_CS_PIN` |
| 29 | Battery (VSYS ÷ 3), inside the RP2350-Plus | `BATTERY_ADC_PIN` |

The display runs on PIO0 and the audio on PIO1. Debug output goes over USB serial: GPIO 0 and 1 are the I²C bus, so there is no UART.

## Start and Select

32blit games know MENU and HOME. Board v3.0.0 has Start and Select instead:

| Button | Is | In a game | Held |
| ------ | -- | --------- | ---- |
| Start | `MENU` | the game's own pause button | 2 s: [sleep](sleep.md) |
| Select | `HOME` | short press: system menu | 1 s: quit to the home screen |

## Optional parts

The radio and the utility module are optional, so the loader looks for them at start-up (`32blit-pico/modules.cpp`). A launcher reads the result from [`SystemInfo::hardware`](../api/system.md#system-info) and hides settings for parts the console doesn't have.

| Flag | Set when | How it's found |
| ---- | -------- | -------------- |
| `HW_BRIGHTNESS` | the display brightness can be set | the OLED driver; the TFT has no brightness control |
| `HW_CLOCK` | real-time clock | RV-3028-C7 answers at I²C address `0x52` |
| `HW_MOTION` | accelerometer and gyroscope | LSM6DS3TR-C answers at `0x6A` |
| `HW_VIBRATION` | vibration motor | DRV2605L answers at `0x5A` |
| `HW_RADIO` | Wi-Fi and Bluetooth | not set yet: comes with the radio driver ([#64](https://github.com/chili-chip/32blit-sdk/issues/64)) |

Without the utility module the I²C bus has no pull-up resistors. The RP2350's own pull-ups keep it high, so the checks just get no answer.

## Board v1.1.4

Older boards build with `chilichip_vgc_v1`. Most signals moved between v1.1.4 and v3.0.0:

| Signal | v1.1.4 | v3.0.0 |
| ------ | ------ | ------ |
| Display SCK / MOSI | GP18 / GP19 | GP26 / GP27 |
| Display CS / DC / RST | GP17 / GP21 / GP20 | GP28 / GP22 / GP21 |
| Sound | PWM on GP22 | I²S on GP3, GP4, GP5, amplifier on GP6 |
| D-pad up / down / left / right | GP9 / GP8 / GP10 / GP7 | GP19 / GP16 / GP20 / GP17 |
| A / B / X / Y | GP3 / GP5 / GP6 / GP4 | GP7 / GP8 / GP11 / GP10 |
| MENU / HOME | GP11 / GP2 | Start GP18 / Select GP12 |
| LED | GP16 | none |
| I²C | I2C1 on GP14 / GP15 | I2C0 on GP0 / GP1 |
| Power | hard switch | sleep mode |
| BOOTSEL | hold X while switching on | BOOT button on the RP2350-Plus |
