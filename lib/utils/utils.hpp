#include <FastLED.h>

uint8_t sinc_to_int8(double d);
double deg_to_rad(double deg);
double sinc(double rad);
uint8_t sinc_brightness(double deg);
void fade(CRGB *leds, uint8_t num_leds);
void black_out(CRGB *leds, uint8_t num_leds);

void rainbow_cylon_loop(CRGB **leds, uint8_t num_strips, uint8_t num_leds);
void cylon_loop(CRGB **leds, uint8_t num_strips, uint8_t num_leds);
void sinc_loop(CRGB **leds, uint8_t num_strips, uint8_t num_leds);
void sinc_loop(CRGB **leds, uint8_t num_strips, uint8_t num_leds, CRGB color);
void set_full_antennae_color(CRGB **leds, uint8_t num_strips, uint8_t num_leds,
                             CRGB color);
void antenna_pong(CRGB *antenna, uint8_t num_leds);
void all_antennae_pong(CRGB **antennae, uint8_t num_antennae, uint8_t num_leds);
void all_antennae_pong_color(CRGB **antennae, uint8_t num_antennae,
                             uint8_t num_leds, CRGB color);
void all_antennae_pong_fast_color(CRGB **antennae, uint8_t num_antennae,
                                  uint8_t num_leds);
