/*
   Sol: https://github.com/evilgeniuslabs/sol
   Copyright (C) 2016 Jason Coon, Evil Genius Labs

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <FastLED.h>
#include <IRremote.h>
#include <EEPROM.h>
#include <Time.h>

#define IR_RECV_PIN 23

IRrecv irReceiver(IR_RECV_PIN);

#include "Commands.h"
#include "GradientPalettes.h"

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

#define DATA_PIN    7
#define CLOCK_PIN   14
#define COLOR_ORDER BGR
#define LED_TYPE    APA102
#define NUM_LEDS    255

CRGB leds[NUM_LEDS];

const uint8_t brightnessCount = 5;
uint8_t brightnessMap[brightnessCount] = { 16, 32, 64, 128, 255 };
uint8_t brightness = brightnessMap[0];

int patternIndex = 0;
CRGB solidColor = CRGB::Red;

uint8_t gHue = 0; // rotating "base color" used by many of the patterns

int autoPlayDurationSeconds = 10;
unsigned int autoPlayTimout = 0;
bool autoplayEnabled = false;

InputCommand command;

CRGBPalette16 IceColors_p = CRGBPalette16(CRGB::Black, CRGB::Blue, CRGB::Aqua, CRGB::White);

uint8_t paletteIndex = 0;

// List of palettes to cycle through.
CRGBPalette16 palettes[] =
{
  RainbowColors_p,
  RainbowStripeColors_p,
  CloudColors_p,
  OceanColors_p,
  ForestColors_p,
  HeatColors_p,
  LavaColors_p,
  PartyColors_p,
  IceColors_p,
};

uint8_t paletteCount = ARRAY_SIZE(palettes);

CRGBPalette16 currentPalette(CRGB::Black);
CRGBPalette16 targetPalette = palettes[paletteIndex];

// ten seconds per color palette makes a good demo
// 20-120 is better for deployment
#define SECONDS_PER_PALETTE 10

///////////////////////////////////////////////////////////////////////

// Forward declarations of an array of cpt-city gradient palettes, and
// a count of how many there are.  The actual color palette definitions
// are at the bottom of this file.
extern const TProgmemRGBGradientPalettePtr gGradientPalettes[];
extern const uint8_t gGradientPaletteCount;

// Current palette number from the 'playlist' of color palettes
uint8_t gCurrentPaletteNumber = 0;

CRGBPalette16 gCurrentPalette( CRGB::Black);
CRGBPalette16 gTargetPalette( gGradientPalettes[0] );

uint8_t coordsX[NUM_LEDS] =
{
  255, 253, 250, 245, 237, 228, 217, 205, 191, 176, 160, 144, 127, 110, 94, 78, 63, 49, 37, 26, 17, 9, 4, 1, 0, 1, 4, 9, 17, 26, 37, 49, 63, 78, 94, 110, 127, 144, 160, 176, 191, 205, 217, 228, 237, 245, 250, 253,
  240, 239, 236, 230, 222, 213, 201, 188, 174, 159, 143, 127, 111, 95, 80, 66, 53, 41, 32, 24, 18, 15, 14, 15, 18, 24, 32, 41, 53, 66, 80, 95, 111, 127, 143, 159, 174, 188, 201, 213, 222, 230, 236, 239,
  226, 225, 221, 215, 207, 197, 185, 172, 158, 143, 127, 111, 96, 82, 69, 57, 47, 39, 33, 29, 28, 29, 33, 39, 47, 57, 69, 82, 96, 111, 127, 143, 158, 172, 185, 197, 207, 215, 221, 225,
  212, 210, 206, 198, 187, 174, 160, 144, 127, 110, 94, 80, 67, 56, 48, 44, 42, 44, 48, 56, 67, 80, 94, 110, 127, 144, 160, 174, 187, 198, 206, 210,
  198, 196, 191, 182, 171, 158, 143, 127, 111, 96, 83, 72, 63, 58, 56, 58, 63, 72, 83, 96, 111, 127, 143, 158, 171, 182, 191, 196,
  184, 182, 176, 167, 155, 142, 127, 112, 99, 87, 78, 72, 70, 72, 78, 87, 99, 112, 127, 142, 155, 167, 176, 182,
  170, 167, 161, 152, 140, 127, 114, 102, 93, 87, 85, 87, 93, 102, 114, 127, 140, 152, 161, 167,
  155, 152, 141, 127, 113, 102, 99, 102, 113, 127, 141, 152,
  141, 134, 120, 113, 120, 134,
  127
};

uint8_t coordsY[NUM_LEDS] =
{
  127, 144, 160, 176, 191, 205, 217, 228, 237, 245, 250, 253, 255, 253, 250, 245, 237, 228, 217, 205, 191, 176, 160, 144, 127, 110, 94, 78, 63, 49, 37, 26, 17, 9, 4, 1, 0, 1, 4, 9, 17, 26, 37, 49, 63, 78, 94, 110,
  127, 143, 159, 174, 188, 201, 213, 222, 230, 236, 239, 240, 239, 236, 230, 222, 213, 201, 188, 174, 159, 143, 127, 111, 95, 80, 66, 53, 41, 32, 24, 18, 15, 14, 15, 18, 24, 32, 41, 53, 66, 80, 95, 111,
  127, 143, 158, 172, 185, 197, 207, 215, 221, 225, 226, 225, 221, 215, 207, 197, 185, 172, 158, 143, 127, 111, 96, 82, 69, 57, 47, 39, 33, 29, 28, 29, 33, 39, 47, 57, 69, 82, 96, 111,
  127, 144, 160, 174, 187, 198, 206, 210, 212, 210, 206, 198, 187, 174, 160, 144, 127, 110, 94, 80, 67, 56, 48, 44, 42, 44, 48, 56, 67, 80, 94, 110,
  127, 143, 158, 171, 182, 191, 196, 198, 196, 191, 182, 171, 158, 143, 127, 111, 96, 83, 72, 63, 58, 56, 58, 63, 72, 83, 96, 111,
  127, 142, 155, 167, 176, 182, 184, 182, 176, 167, 155, 142, 127, 112, 99, 87, 78, 72, 70, 72, 78, 87, 99, 112,
  127, 140, 152, 161, 167, 170, 167, 161, 152, 140, 127, 114, 102, 93, 87, 85, 87, 93, 102, 114,
  127, 141, 152, 155, 152, 141, 127, 113, 102, 99, 102, 113,
  127, 139, 139, 127, 115, 115,
  127
};

// Params for width and height
const uint8_t kMatrixWidth = 16;
const uint8_t kMatrixHeight = 16;

const uint8_t maxX = kMatrixWidth - 1;
const uint8_t maxY = kMatrixHeight - 1;

uint8_t coordsX16[NUM_LEDS] =
{
  16, 16, 16, 15, 15, 14, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 2, 1, 1, 0, 0, 0, 0, 0, 1, 1, 2, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 14, 15, 15, 16, 16,
  15, 15, 15, 14, 14, 13, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 3, 2, 2, 1, 1, 1, 1, 1, 2, 2, 3, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 13, 14, 14, 15, 15,
  14, 14, 14, 13, 13, 12, 12, 11, 10, 9, 8, 7, 6, 5, 4, 4, 3, 2, 2, 2, 2, 2, 2, 2, 3, 4, 4, 5, 6, 7, 8, 9, 10, 11, 12, 12, 13, 13, 14, 14,
  13, 13, 13, 12, 12, 11, 10, 9, 8, 7, 6, 5, 4, 4, 3, 3, 3, 3, 3, 4, 4, 5, 6, 7, 8, 9, 10, 11, 12, 12, 13, 13,
  12, 12, 12, 11, 11, 10, 9, 8, 7, 6, 5, 5, 4, 4, 4, 4, 4, 5, 5, 6, 7, 8, 9, 10, 11, 11, 12, 12,
  12, 11, 11, 10, 10, 9, 8, 7, 6, 5, 5, 5, 4, 5, 5, 5, 6, 7, 8, 9, 10, 10, 11, 11,
  11, 10, 10, 10, 9, 8, 7, 6, 6, 5, 5, 5, 6, 6, 7, 8, 9, 10, 10, 10,
  10, 10, 9, 8, 7, 6, 6, 6, 7, 8, 9, 10,
  9, 8, 8, 7, 8, 8,
  8
};

uint8_t coordsY16[NUM_LEDS] =
{
  8, 9, 10, 11, 12, 13, 14, 14, 15, 15, 16, 16, 16, 16, 16, 15, 15, 14, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 2, 1, 1, 0, 0, 0, 0, 0, 1, 1, 2, 2, 3, 4, 5, 6, 7,
  8, 9, 10, 11, 12, 13, 13, 14, 14, 15, 15, 15, 15, 15, 14, 14, 13, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 3, 2, 2, 1, 1, 1, 1, 1, 2, 2, 3, 3, 4, 5, 6, 7,
  8, 9, 10, 11, 12, 12, 13, 13, 14, 14, 14, 14, 14, 13, 13, 12, 12, 11, 10, 9, 8, 7, 6, 5, 4, 4, 3, 2, 2, 2, 2, 2, 2, 2, 3, 4, 4, 5, 6, 7,
  8, 9, 10, 11, 12, 12, 13, 13, 13, 13, 13, 12, 12, 11, 10, 9, 8, 7, 6, 5, 4, 4, 3, 3, 3, 3, 3, 4, 4, 5, 6, 7,
  8, 9, 10, 11, 11, 12, 12, 12, 12, 12, 11, 11, 10, 9, 8, 7, 6, 5, 5, 4, 4, 4, 4, 4, 5, 5, 6, 7,
  8, 9, 10, 10, 11, 11, 12, 11, 11, 10, 10, 9, 8, 7, 6, 5, 5, 5, 4, 5, 5, 5, 6, 7,
  8, 9, 10, 10, 10, 11, 10, 10, 10, 9, 8, 7, 6, 6, 5, 5, 5, 6, 6, 7,
  8, 9, 10, 10, 10, 9, 8, 7, 6, 6, 6, 7,
  8, 9, 9, 8, 7, 7,
  8
};

void setPixelXY(uint8_t x, uint8_t y, CRGB color) {
  if (x >= kMatrixWidth || y >= kMatrixHeight)
    return;

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    if(coordsX16[i] != x)
      continue;

    if(coordsY16[i] != y)
      continue;
    
    leds[i] = color;
  }
}

typedef uint8_t (*SimplePattern)();
typedef SimplePattern SimplePatternList[];

// List of patterns to cycle through.  Each is defined as a separate function below.

#include "Twinkles.h"
#include "Disk.h"
#include "Noise.h"
#include "PolarNoise.h"
#include "Clock.h"

const SimplePatternList patterns = {
  handClock,
  arcClock,

  pride,
  colorWaves,
  radialPaletteShift,
  paletteArcs,
  incrementalDrift,
  decayingOrbits,
  solarSystem,

  // XY map patterns
  wave,
  pulse,
  horizontalRainbow,
  verticalRainbow,
  diagonalRainbow,
  fire,
  water,

  // noise patterns (XY and Polar variations)
  gradientPalettePolarNoise,
  palettePolarNoise,
  fireNoise,
  firePolarNoise,
  fireNoise2,
  firePolarNoise2,
  lavaNoise,
  lavaPolarNoise,
  rainbowNoise,
  rainbowPolarNoise,
  rainbowStripeNoise,
  rainbowStripePolarNoise,
  partyNoise,
  partyPolarNoise,
  forestNoise,
  forestPolarNoise,
  cloudNoise,
  cloudPolarNoise,
  oceanNoise,
  oceanPolarNoise,
  blackAndWhiteNoise,
  blackAndWhitePolarNoise,
  blackAndBlueNoise,
  blackAndBluePolarNoise,

  // 1D patterns
  rainbow,
  rainbowWithGlitter,
  rainbowSolid,
  sinelon,
  bpm,
  juggle,
  juggle2,
  confetti,
  rainbowTwinkles,
  snowTwinkles,
  cloudTwinkles,
  incandescentTwinkles,

  showSolidColor,
};

int patternCount = ARRAY_SIZE(patterns);

void setup()
{
  FastLED.addLeds<LED_TYPE, DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS); // , DATA_RATE_MHZ(1)
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 6000);
  // FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(brightness);
  FastLED.setDither(false);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  Serial.begin(9600);

  // Initialize the IR receiver
  irReceiver.enableIRIn();
  irReceiver.blink13(true);

  loadSettings();

  FastLED.setBrightness(brightness);
  FastLED.setDither(brightness < 255);

  setupRings();

  setTime(1357041600);
}

void loop()
{
  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy(random());

  uint8_t requestedDelay = patterns[patternIndex]();

  // send the 'leds' array out to the actual LED strip
  FastLED.show();

  handleInput(requestedDelay);

  if (autoplayEnabled && millis() > autoPlayTimout) {
    move(1);
    autoPlayTimout = millis() + (autoPlayDurationSeconds * 1000);
  }

  // blend the current palette to the next
  EVERY_N_MILLISECONDS(40) {
    nblendPaletteTowardPalette(currentPalette, targetPalette, 16);
  }

  EVERY_N_MILLISECONDS( 40 ) {
    gHue++;  // slowly cycle the "base color" through the rainbow
  }

  // slowly change to a new palette
  EVERY_N_SECONDS(SECONDS_PER_PALETTE) {
    paletteIndex++;
    if (paletteIndex >= paletteCount) paletteIndex = 0;
    targetPalette = palettes[paletteIndex];
  };

  // slowly change to a new cpt-city gradient palette
  EVERY_N_SECONDS( SECONDS_PER_PALETTE ) {
    gCurrentPaletteNumber = addmod8( gCurrentPaletteNumber, 1, gGradientPaletteCount);
    gTargetPalette = gGradientPalettes[ gCurrentPaletteNumber ];
  }

  // blend the current cpt-city gradient palette to the next
  EVERY_N_MILLISECONDS(40) {
    nblendPaletteTowardPalette( gCurrentPalette, gTargetPalette, 16);
  }
}

void powerOff()
{
  // clear the display
  // fill_solid(leds, NUM_LEDS, CRGB::Black);
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
    FastLED.show(); // display this frame
    delay(1);
  }

  FastLED.show(); // display this frame

  while (true) {
    InputCommand command = readCommand();
    if (command == InputCommand::Power ||
        command == InputCommand::Brightness)
      return;

    // go idle for a while, converve power
    delay(250);
  }
}

int getBrightnessLevel() {
  int level = 0;
  for (int i = 0; i < brightnessCount; i++) {
    if (brightnessMap[i] >= brightness) {
      level = i;
      break;
    }
  }
  return level;
}

void adjustBrightness(int delta) {
  int level = getBrightnessLevel();

  level += delta;
  if (level < 0)
    level = brightnessCount - 1;
  if (level >= brightnessCount)
    level = 0;

  brightness = brightnessMap[level];
  FastLED.setBrightness(brightness);

  EEPROM.write(0, brightness);
}

uint8_t cycleBrightness() {
  adjustBrightness(1);

  if (brightness == brightnessMap[0])
    return 8;

  return brightness;
}

// algorithm from http://en.wikipedia.org/wiki/Midpoint_circle_algorithm
void drawCircle(uint8_t x0, uint8_t y0, uint8_t radius, const CRGB color)
{
  int a = radius, b = 0;
  int radiusError = 1 - a;

  if (radius == 0) {
    setPixelXY(x0, y0, color);
    return;
  }

  while (a >= b)
  {
    setPixelXY(a + x0, b + y0, color);
    setPixelXY(b + x0, a + y0, color);
    setPixelXY(-a + x0, b + y0, color);
    setPixelXY(-b + x0, a + y0, color);
    setPixelXY(-a + x0, -b + y0, color);
    setPixelXY(-b + x0, -a + y0, color);
    setPixelXY(a + x0, -b + y0, color);
    setPixelXY(b + x0, -a + y0, color);

    b++;
    if (radiusError < 0)
      radiusError += 2 * b + 1;
    else
    {
      a--;
      radiusError += 2 * (b - a + 1);
    }
  }
}

// scale the brightness of all pixels down
void dimAll(byte value)
{
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(value);
  }
}

uint8_t horizontalRainbow()
{
  unsigned long t = millis();

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV( coordsX[i] + (t / 10), 255, 255);
  }

  return 1;
}

uint8_t verticalRainbow()
{
  unsigned long t = millis();

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV( coordsY[i] + (t / 10), 255, 255);
  }

  return 1;
}

uint8_t diagonalRainbow()
{
  unsigned long t = millis();

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV( coordsX[i] + coordsY[i] + (t / 10), 255, 255);
  }

  return 1;
}

uint8_t wave()
{
  const uint8_t scale = 256 / kMatrixWidth;

  static uint8_t rotation = 0;
  static uint8_t theta = 0;
  static uint8_t waveCount = 1;

  uint8_t n = 0;

  switch (rotation) {
    case 0:
      for (int x = 0; x < kMatrixWidth; x++) {
        n = quadwave8(x * 2 + theta) / scale;
        setPixelXY(x, n, ColorFromPalette(currentPalette, x + gHue, 255, LINEARBLEND));
        if (waveCount == 2)
          setPixelXY(x, maxY - n, ColorFromPalette(currentPalette, x + gHue, 255, LINEARBLEND));
      }
      break;

    case 1:
      for (int y = 0; y < kMatrixHeight; y++) {
        n = quadwave8(y * 2 + theta) / scale;
        setPixelXY(n, y, ColorFromPalette(currentPalette, y + gHue, 255, LINEARBLEND));
        if (waveCount == 2)
          setPixelXY(maxX - n, y, ColorFromPalette(currentPalette, y + gHue, 255, LINEARBLEND));
      }
      break;

    case 2:
      for (int x = 0; x < kMatrixWidth; x++) {
        n = quadwave8(x * 2 - theta) / scale;
        setPixelXY(x, n, ColorFromPalette(currentPalette, x + gHue));
        if (waveCount == 2)
          setPixelXY(x, maxY - n, ColorFromPalette(currentPalette, x + gHue, 255, LINEARBLEND));
      }
      break;

    case 3:
      for (int y = 0; y < kMatrixHeight; y++) {
        n = quadwave8(y * 2 - theta) / scale;
        setPixelXY(n, y, ColorFromPalette(currentPalette, y + gHue, 255, LINEARBLEND));
        if (waveCount == 2)
          setPixelXY(maxX - n, y, ColorFromPalette(currentPalette, y + gHue, 255, LINEARBLEND));
      }
      break;
  }

  dimAll(254);

  EVERY_N_SECONDS(10)
  {
    rotation = random(0, 4);
    // waveCount = random(1, 3);
  };

  EVERY_N_MILLISECONDS(7) {
    theta++;
  }

  return 8;
}

uint8_t pulse()
{
  dimAll(200);

  uint8_t maxSteps = 16;
  static uint8_t step = maxSteps;
  static uint8_t centerX = 0;
  static uint8_t centerY = 0;
  float fadeRate = 0.8;

  if (step >= maxSteps)
  {
    centerX = random(kMatrixWidth);
    centerY = random(kMatrixWidth);
    step = 0;
  }

  if (step == 0)
  {
    drawCircle(centerX, centerY, step, ColorFromPalette(currentPalette, gHue, 255, LINEARBLEND));
    step++;
  }
  else
  {
    if (step < maxSteps)
    {
      // initial pulse
      drawCircle(centerX, centerY, step, ColorFromPalette(currentPalette, gHue, pow(fadeRate, step - 2) * 255, LINEARBLEND));

      // secondary pulse
      if (step > 3) {
        drawCircle(centerX, centerY, step - 3, ColorFromPalette(currentPalette, gHue, pow(fadeRate, step - 2) * 255, LINEARBLEND));
      }

      step++;
    }
    else
    {
      step = -1;
    }
  }

  return 30;
}

// Patterns from FastLED example DemoReel100: https://github.com/FastLED/FastLED/blob/master/examples/DemoReel100/DemoReel100.ino

uint8_t rainbow()
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 255 / NUM_LEDS);
  return 8;
}

void addGlitter( uint8_t chanceOfGlitter)
{
  if ( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

uint8_t rainbowWithGlitter()
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
  return 8;
}

uint8_t rainbowSolid()
{
  fill_solid(leds, NUM_LEDS, CHSV(gHue, 255, 255));
  return 8;
}

uint8_t confetti()
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
  return 8;
}

// Updated sinelon (no visual gaps)
uint8_t sinelon()
{
  // a colored dot sweeping
  // back and forth, with
  // fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  uint16_t pos = beatsin16(13, 0, NUM_LEDS);
  static uint16_t prevpos = 0;
  if ( pos < prevpos ) {
    fill_solid( leds + pos, (prevpos - pos) + 1, CHSV(gHue, 220, 255));
  } else {
    fill_solid( leds + prevpos, (pos - prevpos) + 1, CHSV( gHue, 220, 255));
  }
  prevpos = pos;

  return 8;
}

uint8_t bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for ( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(currentPalette, gHue + (i * 2), beat - gHue + (i * 10));
  }

  return 8;
}

uint8_t juggle() {
  // colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  uint8_t dotcount = 4;
  for ( int i = 0; i < dotcount; i++) {
    leds[beatsin16(i + (dotcount - 1), 0, NUM_LEDS)] |= ColorFromPalette(gCurrentPalette, dothue); // CHSV(dothue, 200, 255);
    dothue += (256 / dotcount);
  }

  return 8;
}

uint8_t juggle2()
{
  static uint8_t    numdots =   4; // Number of dots in use.
  static uint8_t   faderate =   2; // How long should the trails be. Very low value = longer trails.
  static uint8_t     hueinc =  255 / numdots - 1; // Incremental change in hue between each dot.
  static uint8_t    thishue =   0; // Starting hue.
  static uint8_t     curhue =   0; // The current hue
  static uint8_t    thissat = 255; // Saturation of the colour.
  static uint8_t thisbright = 255; // How bright should the LED/display be.
  static uint8_t   basebeat =   5; // Higher = faster movement.

  static uint8_t lastSecond =  99;  // Static variable, means it's only defined once. This is our 'debounce' variable.
  uint8_t secondHand = (millis() / 1000) % 30; // IMPORTANT!!! Change '30' to a different value to change duration of the loop.

  if (lastSecond != secondHand) { // Debounce to make sure we're not repeating an assignment.
    lastSecond = secondHand;
    switch (secondHand) {
      case  0: numdots = 2; basebeat = 5; hueinc = 16; faderate = 2; thishue = 0; break; // You can change values here, one at a time , or altogether.
      case 10: numdots = 3; basebeat = 4; hueinc = 16; faderate = 8; thishue = 128; break;
      case 20: numdots = 4; basebeat =  3; hueinc =  0; faderate = 8; thishue = random8(); break; // Only gets called once, and not continuously for the next several seconds. Therefore, no rainbows.
      case 30: break;
    }
  }

  // Several colored dots, weaving in and out of sync with each other
  curhue = thishue; // Reset the hue values.
  fadeToBlackBy(leds, NUM_LEDS, faderate);
  for ( int i = 0; i < numdots; i++) {
    //beat16 is a FastLED 3.1 function
    leds[beatsin16(basebeat + i + numdots, 0, NUM_LEDS)] += CHSV(gHue + curhue, thissat, thisbright);
    curhue += hueinc;
  }

  return 8;
}

// based on FastLED example Fire2012WithPalette: https://github.com/FastLED/FastLED/blob/master/examples/Fire2012WithPalette/Fire2012WithPalette.ino
void heatMap(CRGBPalette16 palette, bool up)
{
  dimAll(254);

  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy(random(256));

  // COOLING: How much does the air cool as it rises?
  // Less cooling = taller flames.  More cooling = shorter flames.
  // Default 55, suggested range 20-100
  uint8_t cooling = 55;

  // SPARKING: What chance (out of 255) is there that a new spark will be lit?
  // Higher chance = more roaring fire.  Lower chance = more flickery fire.
  // Default 120, suggested range 50-200.
  uint8_t sparking = 120;

  // Array of temperature readings at each simulation cell
  static byte heat[kMatrixWidth + 2][kMatrixHeight + 2];

  for (uint8_t x = 0; x < kMatrixWidth; x++)
  {
    // Step 1.  Cool down every cell a little
    for (uint8_t y = 0; y < kMatrixHeight; y++)
    {
      heat[x][y] = qsub8(heat[x][y], random8(0, ((cooling * 10) / kMatrixHeight) + 2));
    }

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for (uint8_t y = 0; y < kMatrixHeight; y++)
    {
      heat[x][y] = (heat[x][y + 1] + heat[x][y + 2] + heat[x][y + 2]) / 3;
    }

    // Step 2.  Randomly ignite new 'sparks' of heat
    if (random8() < sparking)
    {
      heat[x][maxY] = qadd8(heat[x][maxY], random8(160, 255));
    }

    // Step 4.  Map from heat cells to LED colors
    for (uint8_t y = 0; y < kMatrixHeight; y++)
    {
      uint8_t colorIndex = 0;

      if (up)
        colorIndex = heat[x][y];
      else
        colorIndex = heat[x][(maxY) - y];

      // Recommend that you use values 0-240 rather than
      // the usual 0-255, as the last 15 colors will be
      // 'wrapping around' from the hot end to the cold end,
      // which looks wrong.
      colorIndex = scale8(colorIndex, 240);

      // override color 0 to ensure a black background
      if (colorIndex != 0)
      {
        setPixelXY(x, y, ColorFromPalette(palette, colorIndex, 255, LINEARBLEND));
      }
    }
  }
}

uint8_t fire()
{
  heatMap(HeatColors_p, true);

  return 8;
}

uint8_t water()
{
  heatMap(IceColors_p, false);

  return 8;
}

uint8_t showSolidColor()
{
  fill_solid(leds, NUM_LEDS, solidColor);

  return 30;
}

// Pride2015 by Mark Kriegsman: https://gist.github.com/kriegsman/964de772d64c502760e5
// This function draws rainbows with an ever-changing,
// widely-varying set of parameters.
uint8_t pride() {
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;

  uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 1, 3000);

  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5, 9);
  uint16_t brightnesstheta16 = sPseudotime;

  for ( uint16_t i = 0 ; i < NUM_LEDS; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);

    CRGB newcolor = CHSV( hue8, sat8, bri8);

    uint16_t pixelnumber = i;

    nblend(leds[pixelnumber], newcolor, 64);
  }

  return 15;
}

uint8_t radialPaletteShift()
{
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    // leds[i] = ColorFromPalette( currentPalette, gHue + sin8(i*16), brightness);
    uint8_t index = (NUM_LEDS - 1) - i;

    leds[index] = ColorFromPalette(gCurrentPalette, i + gHue, 255, LINEARBLEND);
  }

  return 8;
}

// ColorWavesWithPalettes by Mark Kriegsman: https://gist.github.com/kriegsman/8281905786e8b2632aeb
// This function draws color waves with an ever-changing,
// widely-varying set of parameters, using a color palette.
void colorwaves( CRGB* ledarray, uint16_t numleds, CRGBPalette16& palette)
{
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;

  // uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 300, 1500);

  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5, 9);
  uint16_t brightnesstheta16 = sPseudotime;

  for ( uint16_t i = 0 ; i < numleds; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;
    uint16_t h16_128 = hue16 >> 7;
    if ( h16_128 & 0x100) {
      hue8 = 255 - (h16_128 >> 1);
    } else {
      hue8 = h16_128 >> 1;
    }

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);

    uint8_t index = hue8;
    //index = triwave8( index);
    index = scale8( index, 240);

    CRGB newcolor = ColorFromPalette( palette, index, bri8);

    uint16_t pixelnumber = i;

    nblend( ledarray[pixelnumber], newcolor, 128);
  }
}

uint8_t colorWaves()
{
  colorwaves( leds, NUM_LEDS, gCurrentPalette);
  return 20;
}

// Alternate rendering function just scrolls the current palette
// across the defined LED strip.
void palettetest( CRGB* ledarray, uint16_t numleds, const CRGBPalette16& gCurrentPalette)
{
  static uint8_t startindex = 0;
  startindex--;
  fill_palette( ledarray, numleds, startindex, (256 / NUM_LEDS) + 1, gCurrentPalette, 255, LINEARBLEND);
}

uint8_t paletteArcs()
{
  dimAll(254);

  static uint8_t offset = 0;

  bool alternate = false;

  for (uint8_t i = 3; i < ringCount; i++)
  {
    uint8_t angle = beat8(i * 3);

    if (alternate)
      angle = 255 - angle;

    fillRing256(i, ColorFromPalette(gCurrentPalette, (i * (255 / ringCount)) + offset), angle - 16, angle + 16);

    alternate = !alternate;
  }

  EVERY_N_MILLISECONDS(60)
  {
    offset++;
  }

  return 4;
}

uint8_t incrementalDrift()
{
  dimAll(beatsin8(1, 220, 254));

  static uint8_t offset = 0;

  uint8_t beat = 64;

  for (uint8_t i = 0; i < ringCount; i++)
  {
    uint8_t angle = beat8(beat -= 2);

    uint8_t index = angleToPixel256(angle, i);
    leds[index] = ColorFromPalette(gCurrentPalette, (i * (255 / ringCount)) + offset);
    //    leds[index] = CHSV(i * (255 / ringCount) + offset, 255, 255);
  }

  EVERY_N_MILLISECONDS(60)
  {
    offset++;
  }

  return 1;
}

uint8_t decayingOrbits()
{
  dimAll(240);

  const uint8_t count = 12;
  static uint8_t positions[count];

  EVERY_N_MILLISECONDS(15)
  {
    for (uint8_t i = 0; i < count; i++)
    {
      uint8_t pos = positions[i];

      if (pos == 255 && random8() < 1)
        pos--;

      if (pos != 255)
        pos--;

      if (pos != 255)
        leds[NUM_LEDS - pos] = ColorFromPalette(gCurrentPalette, (255 / count) * i);

      positions[i] = pos;
    }
  }

  return 4;
}

uint8_t solarSystem() {
  dimAll(0);

  // sun
  fillRing256(0, random8(2) == 0 ? CRGB::Yellow : CRGB::OrangeRed, 0, 255);

  for (uint8_t i = rings[1][0]; i <= rings[1][1]; i++)
    leds[i] = random8(2) == 0 ? CRGB::Yellow : CRGB::OrangeRed;

  // fillRing256(1, CRGB::OrangeRed, 0, 255);

  // Mercury
  leds[angleToPixel256(255 - beat8(128), 2)] = 0x2D2D2D;

  // Venus
  leds[angleToPixel256(255 - beat8(112), 3)] = 0x1E1D00;

  // Earth
  leds[angleToPixel256(255 - beat8(62), 4)] = CRGB::Blue;

  // Mars
  leds[angleToPixel256(255 - beat8(25), 5)] = 0x330000;

  // Jupiter
  leds[angleToPixel256(255 - beat8(8), 6)] = 0x1E1D00;

  // Saturn
  leds[angleToPixel256(255 - beat8(4), 7)] = 0x121405;

  // Uranus
  leds[angleToPixel256(255 - beat8(3), 8)] = CRGB::Aqua;

  // Neptune
  leds[angleToPixel256(255 - beat8(1), 9)] = CRGB::Blue;

  return 1;
}

void moveTo(int index) {
  patternIndex = index;

  if (patternIndex >= patternCount)
    patternIndex = 0;
  else if (patternIndex < 0)
    patternIndex = patternCount - 1;

  fill_solid(leds, NUM_LEDS, CRGB::Black);

  EEPROM.write(1, patternIndex);
}

void move(int delta) {
  moveTo(patternIndex + delta);
}

void loadSettings() {
  // load settings from EEPROM

  // brightness
  brightness = EEPROM.read(0);
  if (brightness < 1)
    brightness = 1;
  else if (brightness > 255)
    brightness = 255;

  // patternIndex
  patternIndex = EEPROM.read(1);
  if (patternIndex < 0)
    patternIndex = 0;
  else if (patternIndex >= patternCount)
    patternIndex = patternCount - 1;

  // solidColor
  solidColor.r = EEPROM.read(2);
  solidColor.g = EEPROM.read(3);
  solidColor.b = EEPROM.read(4);

  if (solidColor.r == 0 && solidColor.g == 0 && solidColor.b == 0)
    solidColor = CRGB::White;
}

void setSolidColor(CRGB color) {
  solidColor = color;

  EEPROM.write(2, solidColor.r);
  EEPROM.write(3, solidColor.g);
  EEPROM.write(4, solidColor.b);

  moveTo(patternCount - 1);
}

void handleInput(unsigned int requestedDelay) {
  unsigned int requestedDelayTimeout = millis() + requestedDelay;

  while (true) {
    command = readCommand(defaultHoldDelay);

    if (command != InputCommand::None) {
      Serial.print("command: ");
      Serial.println((int) command);
    }

    if (command == InputCommand::Up) {
      move(1);
      break;
    }
    else if (command == InputCommand::Down) {
      move(-1);
      break;
    }
    else if (command == InputCommand::Brightness) {
      if (isHolding || cycleBrightness() == 0) {
        heldButtonHasBeenHandled();
        powerOff();
        break;
      }
    }
    else if (command == InputCommand::Power) {
      powerOff();
      break;
    }
    else if (command == InputCommand::BrightnessUp) {
      adjustBrightness(1);
    }
    else if (command == InputCommand::BrightnessDown) {
      adjustBrightness(-1);
    }
    else if (command == InputCommand::PlayMode) { // toggle pause/play
      autoplayEnabled = !autoplayEnabled;
    }
    //else if (command == InputCommand::Palette) { // cycle color pallete
    //    effects.CyclePalette();
    //}

    // pattern buttons

    else if (command == InputCommand::Pattern1) {
      moveTo(0);
      break;
    }
    else if (command == InputCommand::Pattern2) {
      moveTo(1);
      break;
    }
    else if (command == InputCommand::Pattern3) {
      moveTo(2);
      break;
    }
    else if (command == InputCommand::Pattern4) {
      moveTo(3);
      break;
    }
    else if (command == InputCommand::Pattern5) {
      moveTo(4);
      break;
    }
    else if (command == InputCommand::Pattern6) {
      moveTo(5);
      break;
    }
    else if (command == InputCommand::Pattern7) {
      moveTo(6);
      break;
    }
    else if (command == InputCommand::Pattern8) {
      moveTo(7);
      break;
    }
    else if (command == InputCommand::Pattern9) {
      moveTo(8);
      break;
    }
    else if (command == InputCommand::Pattern10) {
      moveTo(9);
      break;
    }
    else if (command == InputCommand::Pattern11) {
      moveTo(10);
      break;
    }
    else if (command == InputCommand::Pattern12) {
      moveTo(11);
      break;
    }

    // custom color adjustment buttons

    else if (command == InputCommand::RedUp) {
      solidColor.red += 1;
      setSolidColor(solidColor);
      break;
    }
    else if (command == InputCommand::RedDown) {
      solidColor.red -= 1;
      setSolidColor(solidColor);
      break;
    }
    else if (command == InputCommand::GreenUp) {
      solidColor.green += 1;
      setSolidColor(solidColor);

      break;
    }
    else if (command == InputCommand::GreenDown) {
      solidColor.green -= 1;
      setSolidColor(solidColor);
      break;
    }
    else if (command == InputCommand::BlueUp) {
      solidColor.blue += 1;
      setSolidColor(solidColor);
      break;
    }
    else if (command == InputCommand::BlueDown) {
      solidColor.blue -= 1;
      setSolidColor(solidColor);
      break;
    }

    // color buttons

    else if (command == InputCommand::Red) {
      setSolidColor(CRGB::Red);
      break;
    }
    else if (command == InputCommand::RedOrange) {
      setSolidColor(CRGB::OrangeRed);
      break;
    }
    else if (command == InputCommand::Orange) {
      setSolidColor(CRGB::Orange);
      break;
    }
    else if (command == InputCommand::YellowOrange) {
      setSolidColor(CRGB::Goldenrod);
      break;
    }
    else if (command == InputCommand::Yellow) {
      setSolidColor(CRGB::Yellow);
      break;
    }

    else if (command == InputCommand::Green) { // Red, Green, and Blue buttons can be used by ColorInvaders game, which is the next to last pattern
      setSolidColor(CRGB::Green);
      break;
    }
    else if (command == InputCommand::Lime) {
      setSolidColor(CRGB::Lime);
      break;
    }
    else if (command == InputCommand::Aqua) {
      setSolidColor(CRGB::Aqua);
      break;
    }
    else if (command == InputCommand::Teal) {
      setSolidColor(CRGB::Teal);
      break;
    }
    else if (command == InputCommand::Navy) {
      setSolidColor(CRGB::Navy);
      break;
    }

    else if (command == InputCommand::Blue) { // Red, Green, and Blue buttons can be used by ColorInvaders game, which is the next to last pattern
      setSolidColor(CRGB::Blue);
      break;
    }
    else if (command == InputCommand::RoyalBlue) {
      setSolidColor(CRGB::RoyalBlue);
      break;
    }
    else if (command == InputCommand::Purple) {
      setSolidColor(CRGB::Purple);
      break;
    }
    else if (command == InputCommand::Indigo) {
      setSolidColor(CRGB::Indigo);
      break;
    }
    else if (command == InputCommand::Magenta) {
      setSolidColor(CRGB::Magenta);
      break;
    }

    else if (command == InputCommand::White) {
      setSolidColor(CRGB::White);
      break;
    }
    else if (command == InputCommand::Pink) {
      setSolidColor(CRGB::Pink);
      break;
    }
    else if (command == InputCommand::LightPink) {
      setSolidColor(CRGB::LightPink);
      break;
    }
    else if (command == InputCommand::BabyBlue) {
      setSolidColor(CRGB::CornflowerBlue);
      break;
    }
    else if (command == InputCommand::LightBlue) {
      setSolidColor(CRGB::LightBlue);
      break;
    }

    if (millis() >= requestedDelayTimeout)
      break;
  }
}
