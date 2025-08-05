#include <FastLED.h> //control and utility library for addressable LEDs
#include <avr/pgmspace.h>
#include "sputnikLookupTables.h"

#define NUM_LEDS 200 //number of LEDs per antenna, must be <256
#define NUM_ANTENNAE 3 //number of antennae to control
#define MIN_STAR_SPEED 20 //lower bound for speed of a star, >=20 recommended to prevent too many slow stars at once
#define MAX_STAR_SPEED 255 //upper bound for speed of a star, <127 recommended to avoid aliasing, 255 max
#define MIN_STAR_SAT 48 //lower bound for star color saturation, lower = whiter
#define MAX_STAR_SAT 96 //upper bound for star color saturation, higher = more color, 255 max
#define MIN_STAR_VAL 192 //lower bound for brightness of a star
#define MAX_STAR_VAL 255 //upper bound for brightness of a star
#define MIN_STAR_DELAY 40 //lower bound for number of cycles between new stars
#define MAX_STAR_DELAY 80 //upper bound for number of cycles between new stars
#define MIN_STAR_SIZE 3
#define MAX_STAR_SIZE 5
#define CMBR_BRIGHTNESS 6 //max brightness for the cmbr effect
#define CMBR_SPEED 1 //how fast the cmbr effect changes. bitshift right; 0=fastest
#define CMBR_SCALE 4 //density of the cmbr effect. bitshift left; 0=least dense
#define NUM_FIRE_PALETTES 4 //number of palette options for fire effect
#define FIRE_HEIGHT 100 //number of LEDs to use for fire effect, min 2
#define FIRE_RISE_SPEED 8 //how fast the fire effect rises
#define FIRE_FLICKER_SPEED 1 //how fast the fire flickers. bitshift right; 0=fastest
#define FRAME_TIME 6 //how many millis to delay each loop; ~1000/fps
#define CMBR_CYCLE_LENGTH 30 //how often the cmbr effect changes color, in seconds
#define FIRE_CYCLE_LENGTH 10 //how often the fire effect changes color, in seconds

CRGB **antennae = new CRGB*[NUM_ANTENNAE]; //buffer for writing to LEDs
CRGB **cmbrLayer = new CRGB*[NUM_ANTENNAE]; //background layer
CRGB **starLayer = new CRGB*[NUM_ANTENNAE]; //star effect layer
CRGB **fireLayer = new CRGB*[NUM_ANTENNAE]; //fire effect layer

DEFINE_GRADIENT_PALETTE(fireCorrected_p) { //classic fire heatmap, color corrected for FCOB LEDs
  0, 255, 255, 255,
  128, 255, 144, 0,
  192, 255, 0, 0,
  255, 0, 0, 0
};

DEFINE_GRADIENT_PALETTE(rmc_p) { //classic fire heatmap, color corrected for FCOB LEDs
  0, 255, 255, 255,
  90, 225, 225, 225,
  120, 255, 100, 0,
  180, 145, 70, 0,
  220, 125, 50, 0,
  240, 255, 0, 0,
  255, 0, 0, 0
};

DEFINE_GRADIENT_PALETTE(yellowGreen_p) { //white->yellow->green->black gradient
  0, 255, 255, 255,
  128, 255, 160, 0,
  192, 0, 96, 0,
  255, 0, 0, 0
};

DEFINE_GRADIENT_PALETTE(pinkPurpleBlue_p) { //white->pink->purple->blue->black gradient
  0, 255, 255, 255,
  128, 255, 0, 224,
  192, 0, 0, 128,
  255, 0, 0, 0
};

uint8_t cmbrColor; //0=red, 1=green, 2=blue
uint8_t fireColor; //choice for fire palette, range [0 - NUM_FIRE_PALETTES)
CRGBPalette256 firePal; //active palette for fire effect

const TProgmemRGBGradientPaletteRef firePalettes[NUM_FIRE_PALETTES] = { //palettes to use for fire effect
  
  rmc_p,
  fireCorrected_p,
  yellowGreen_p,
  pinkPurpleBlue_p
};

class StarNode {
  //structure to store data for an individual star
  public: //everything public because fast, be careful
    uint8_t antenna; //which antenna the star is on
    uint16_t x; //position of the star; 1st byte=LED index, 2nd byte=offset
    uint8_t deltaX; //speed; how much does 'x' increase per cycle
    CHSV color; //hue, saturation, value of star
    uint8_t maxV; //max value (aka brightness) of star
    uint8_t size;
    StarNode *next; //pointer to the next StarNode in list, or null

    StarNode(uint8_t a=0, StarNode *n=nullptr) {
      antenna = a;
      x = 0;
      deltaX = random8(MIN_STAR_SPEED, MAX_STAR_SPEED); //generate random speed
      color = CHSV(random8(), random8(MIN_STAR_SAT, MAX_STAR_SAT), 0); //generate random color/saturation
      maxV = random8(MIN_STAR_VAL, MAX_STAR_VAL); //generate random brightness
      size = random8(MIN_STAR_SIZE, MAX_STAR_SIZE + 1);
      next = n; //pointer to next node in list, or null
    }
};

StarNode *updateStarNodes(StarNode *node, CRGB **leds) {
  //helper function for stars()
  //walks the list with recursion, updating nodes and writing output to 'leds' array
  //returns a pointer to the updated list
  if (node == nullptr) {
    return nullptr;
  }

  node->x += node->deltaX; //increment position
  StarNode *tmp = node->next; //save the next node, in case we delete the current one
  uint8_t pos = static_cast<uint8_t>(node->x >> 8); //get first byte of 'x', which maps to the LED index

  if (pos >= NUM_LEDS) { //check if we've reached the end of the antenna
    delete node; //release memory
    return updateStarNodes(tmp, leds); //return the rest of the list, processed
  }

  uint8_t vOffset = static_cast<uint8_t>(node->x & 0x00FF);// >> 1; //get second byte of 'x', which maps to the offset of the waveform. bitshift simplifies math in the next step

  //now we draw the waveform across 4 LEDs, checking if each index is inbounds
  /*node->color.v = scale8(ease8InOutApprox(127 - vOffset), node->maxV);
  leds[node->antenna][pos] += node->color;
  pos++;
  if (pos < NUM_LEDS) {
    node->color.v = scale8(ease8InOutApprox(255 - vOffset), node->maxV);
    leds[node->antenna][pos] += node->color;
    pos++;
    if (pos < NUM_LEDS) {
      node->color.v = scale8(ease8InOutApprox(127 + vOffset), node->maxV);
      leds[node->antenna][pos] += node->color;
      pos++;
      if (pos < NUM_LEDS) {
        node->color.v = scale8(ease8InOutApprox(vOffset), node->maxV);
        leds[node->antenna][pos] += node->color;
      }
    }
  }*/
  for (uint8_t i = 0; i < node->size; i++) {
    if ((pos < NUM_LEDS) && (pos > (FIRE_HEIGHT - 64))) {
      switch (node->size) {
        /*case 2:
          node->color.v = scale8(pgm_read_byte(&(star2w[i][vOffset])), node->maxV);
          break;*/
        case 3:
          node->color.v = scale8(pgm_read_byte(&(star3w[i][vOffset])), node->maxV);
          break;
        case 4:
          node->color.v = scale8(pgm_read_byte(&(star4w[i][vOffset])), node->maxV);
          break;
        case 5:
          node->color.v = scale8(pgm_read_byte(&(star5w[i][vOffset])), node->maxV);
          break;
        /*case 6:
          node->color.v = scale8(pgm_read_byte(&(star6w[i][vOffset])), node->maxV);
          break;*/
      }
      if (pos < FIRE_HEIGHT) {
        node->color.v = scale8(node->color.v, mul8(4, sub8(64, sub8(FIRE_HEIGHT, pos))));
      }
      leds[node->antenna][pos] += node->color;
      pos++;
    }
    else {break;}
  }

  node->next = updateStarNodes(tmp, leds); //process the rest of the list
  return node; //return processed node
}

void stars(CRGB **leds) {
  //main function controlling the star effect
  static StarNode *head = nullptr; //static pointer to the first node in the list
  static uint8_t cooldown = 0; //how many cycles to wait before making a new star
  static uint8_t nextAntenna = 0; //which antenna to put a new star on

  //first clear the array
  for (uint8_t i = 0; i < NUM_ANTENNAE; i++) {
    for (uint8_t j = 0; j < NUM_LEDS; j++) {
      leds[i][j].setRGB(0, 0, 0);
    }
  }

  //check if it's time to make a new star
  if (!cooldown) {
    StarNode *newStar = new StarNode(nextAntenna, head); //make new star at the front of the list
    head = newStar; //update head
    cooldown = random8(MIN_STAR_DELAY, MAX_STAR_DELAY); //generate a new cooldown
    nextAntenna = addmod8(nextAntenna, 1, NUM_ANTENNAE); //increment the antenna choice for new stars, with wraparound
  }

  head = updateStarNodes(head, leds); //process the list of stars and write values to 'leds'
  cooldown--; //increment cooldown
}

void cmbr(CRGB **leds, uint8_t colorChoice=0) {
  //undulating background layer, 'colorChoice' 0=red, 1=green, 2=blue
  unsigned long t = millis() >> CMBR_SPEED; //current time, bitshifted

  //iterate through LEDs
  for (uint16_t j = 0; j < NUM_LEDS; j++) {
    uint16_t jScaled = j << CMBR_SCALE;
    for (uint8_t i = 0; i < NUM_ANTENNAE; i++) {
      leds[i][j].setRGB(0, 0, 0); //clear previous value
      //sample and scale perlin noise
      leds[i][j][colorChoice] = scale8(ease8InOutApprox(inoise8(i, jScaled, t)), CMBR_BRIGHTNESS);
    }
  }
}

void fire(CRGB **leds, const CRGBPalette256 &pal) {
  //fire effect growing from the base of each antennae
  static uint16_t offset = 255; //first index to sample perlin noise
  const static uint8_t stepSize = 255 / (FIRE_HEIGHT - 1); //distance between samples of palette/noise
  unsigned long t = millis() >> FIRE_FLICKER_SPEED; //current time, bitshifted
  uint8_t x = 0; //index of sample

  //iterate through the LEDs
  for (uint8_t j = 0; j < FIRE_HEIGHT; j++) {
    for (uint8_t i = 0; i < NUM_ANTENNAE; i++) {
      //sample and scale the noise, add to value of the gradient at that index, and get that color from palette
      leds[i][j] = ColorFromPalette(pal, qadd8(inoise8(mul8(i, stepSize), offset - x, t) >> 1, x));
    }
    x += stepSize; //increment sample index
  }

  offset += FIRE_RISE_SPEED; //increment offset for rising fire effect
}

void setup() {
  // put your setup code here, to run once:
  delay(3000); //powerup delay

  //initialize arrays
  for (uint8_t i = 0; i < NUM_ANTENNAE; i++) {
    antennae[i] = new CRGB[NUM_LEDS];
    cmbrLayer[i] = new CRGB[NUM_LEDS];
    starLayer[i] = new CRGB[NUM_LEDS];
    fireLayer[i] = new CRGB[NUM_LEDS];
    for (uint8_t j = 0; j < NUM_LEDS; j++) {
      antennae[i][j].setRGB(0, 0, 0);
      cmbrLayer[i][j].setRGB(0, 0, 0);
      starLayer[i][j].setRGB(0, 0, 0);
      fireLayer[i][j].setRGB(0, 0, 0);
    }
  }

  //set the antennae array as the buffer for FastLED
  FastLED.addLeds<WS2811, 4, RGB>(antennae[0], NUM_LEDS);
  FastLED.addLeds<WS2811, 5, RGB>(antennae[1], NUM_LEDS);
  FastLED.addLeds<WS2811, 6, RGB>(antennae[2], NUM_LEDS);
  FastLED.addLeds<WS2811, 7, RGB>(antennae[3], NUM_LEDS);

  FastLED.clear(); //clear LEDs
  FastLED.show(); //push

  cmbrColor = 0;
  fireColor = 1;
  firePal = firePalettes[fireColor];
}

void loop() {
  // put your main code here, to run repeatedly:

  //do 1 cycle of each effect
  cmbr(cmbrLayer, cmbrColor);
  stars(starLayer);
  fire(fireLayer, firePal);

  //write the sum of each effect layer to the FastLED buffer
  for (uint8_t j = 0; j < NUM_LEDS; j++) {
    for (uint8_t i = 0; i < NUM_ANTENNAE; i++) {
      antennae[i][j] = cmbrLayer[i][j] + starLayer[i][j] + fireLayer[i][j];
    }
  }

  FastLED.show(); //push

  delay(FRAME_TIME); //delay for stable fps

  EVERY_N_SECONDS(CMBR_CYCLE_LENGTH) {
    cmbrColor = addmod8(cmbrColor, 1, 3); //change cmbr color, with wraparound
  }

  /*EVERY_N_SECONDS(FIRE_CYCLE_LENGTH) {
    fireColor = addmod8(fireColor, 1, NUM_FIRE_PALETTES); //change fire color choice, with wraparound
    firePal = firePalettes[fireColor]; //update palette
  }*/
}
