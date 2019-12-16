# ESP32-NeoPixel-WS2812-RMT
NeoPixel (WS2812) Driver Example code using RMT peripheral (Based on https://github.com/JSchaenzle/ESP32-NeoPixel-WS2812-RMT)

This project contains example code for driving a chain of NeoPixels connected to an ESP32 using the RMT peripheral build into the micro.

This code assumes you are using FreeRTOS.

## Pros
- It's very simple to use!
- The code utilizes hardware to drive the data out line so your application can be free to do other things.
- Easily and consistently meets the timing requirements spec'd out by the WS2812 datasheet.

## Cons
Because of the way the ESP32 RMT peripheral works this technique for driving NeoPixels is a little heavy on memory usage. It requires (4 bytes * 24 * NUM_LEDS) of dedicated memory.

## Usage
Copy the source and header files into your project. Update the following defines based on your project needs / arrangement. 
- NUM_LEDS
- LED_RMT_TX_CHANNEL
- LED_RMT_TX_GPIO

In your application init section call `void ws2812_control_init(void)` to initialize the RMT peripheral with the correct configuration.

Whenever you need to update the LEDs simply call `void ws2812_write_leds(struct led_state new_state)`. The `led_state` structure just contains an array of 32-bit integers - one for each LED - that you must set to the desired RGB values. The bottom three bytes of each value are R, G and B.

### Example
```c
#define NUM_LEDS 3
#include "ws2812_control.h"

#define RED   0xFF0000
#define GREEN 0x00FF00
#define BLUE  0x0000FF

int main(void) {
  ws2812_control_init();

  struct led_state new_state;
  new_state.leds[0] = RED;
  new_state.leds[1] = GREEN;
  new_state.leds[2] = BLUE;

  ws2812_write_leds(new_state);
}
```


### Timing
This code is tuned based on the timing specifications indicated in the following datasheet provided by Sparkfun: https://cdn.sparkfun.com/datasheets/Components/LED/COM-12877.pdf

I'm pretty sure there are variations of the NeoPixel out there that have different timing requirements to you may have to tweak the code accordingly.


## Contribution
If you find a problem or have ideas about how to improve this please submit a PR and I will happily review and merge. Thanks!

Enjoy!
