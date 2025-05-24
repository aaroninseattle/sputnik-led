#include "Arduino.h"
#include <FastLED.h>

#include <color_palette.hpp>
#include <pacifica.hpp>
#include <utils.hpp>

#define NUM_LEDS 200
#define NUM_ANTENNAE 4
#define ONE_MIN_IN_MILLIS 60000
#define CHOICES 5
#define PATTERN_CYCLE_TIME (ONE_MIN_IN_MILLIS * 1)

CRGB strip_6[NUM_LEDS];
CRGB strip_7[NUM_LEDS];
CRGB strip_8[NUM_LEDS];
CRGB strip_9[NUM_LEDS];
CRGB *antennae[NUM_ANTENNAE];

unsigned long last_millis;
void (*current_loop)(CRGB **, uint8_t, uint8_t);
int current_choice;

// from color_palette lib
extern CRGBPalette16 color_palette_palette;
extern TBlendType color_palette_blending;

void setup() {
  delay(3000); // power-up safety delay
  FastLED.addLeds<WS2811, 6, RGB>(strip_6, NUM_LEDS);
  FastLED.addLeds<WS2811, 7, RGB>(strip_7, NUM_LEDS);
  FastLED.addLeds<WS2811, 8, RGB>(strip_8, NUM_LEDS);
  FastLED.addLeds<WS2811, 9, RGB>(strip_9, NUM_LEDS);
  black_out(strip_6, NUM_LEDS);
  black_out(strip_7, NUM_LEDS);
  black_out(strip_8, NUM_LEDS);
  black_out(strip_9, NUM_LEDS);
  antennae[0] = strip_6;
  antennae[1] = strip_7;
  antennae[2] = strip_8;
  antennae[3] = strip_9;
  last_millis = millis();

  // init a pattern for the first five mins
  color_palette_palette = RainbowColors_p;
  color_palette_blending = LINEARBLEND;
  current_loop = color_palette_loop;
  current_choice = 0;
}
void loop() {
  unsigned long now = millis();
  if (now - last_millis >= PATTERN_CYCLE_TIME) {
    current_choice = (current_choice + 1) % CHOICES;
    last_millis = now;
    if (0 == current_choice) {
      current_loop = all_antennae_pong_fast_color;
    }
    if (1 == current_choice) {
      color_palette_palette = RainbowColors_p;
      color_palette_blending = LINEARBLEND;
      current_loop = color_palette_loop;
    }
    if (2 == current_choice) {
      color_palette_palette = RainbowStripeColors_p;
      color_palette_blending = LINEARBLEND;
      current_loop = color_palette_loop;
    }
    if (3 == current_choice) {
      color_palette_palette = PartyColors_p;
      color_palette_blending = LINEARBLEND;
      current_loop = color_palette_loop;
    }
    if (4 == current_choice) {
      current_loop = rainbow_cylon_loop;
    }
  }
  current_loop(antennae, NUM_ANTENNAE, NUM_LEDS);
}
