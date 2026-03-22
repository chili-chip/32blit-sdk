#include "led.hpp"
#include "hardware/gpio.h"
#include "pico/binary_info.h"
#include "config.h"
#include "engine/api_private.hpp"

void init_led() {
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // Metadata for the Picotool
    bi_decl(bi_1pin_with_name(LED_PIN, "Status LED"));
}

void update_led() {
    using namespace blit;

    // Check if the requested color is "Black" (all channels 0)
    // If R, G, or B is greater than 0, we turn the LED on.
    bool should_be_on = (api_data.LED.r > 0) || 
                        (api_data.LED.g > 0) || 
                        (api_data.LED.b > 0);

#ifdef LED_INVERTED
    gpio_put(LED_PIN, !should_be_on);
#else
    gpio_put(LED_PIN, should_be_on);
#endif
}