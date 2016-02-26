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

// scale the brightness of all pixels down
void dimAll(byte value)
{
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(value);
  }
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

uint8_t sinelon1()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16(13, 0, NUM_LEDS);
  leds[pos] += CHSV( gHue, 255, 192);
  return 8;
}

uint8_t bpm1()
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
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  uint8_t dotcount = 3;
  for ( int i = 0; i < dotcount; i++) {
    leds[beatsin16(i + (dotcount - 1), 0, NUM_LEDS)] |= CHSV(dothue, 200, 255);
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
      case  0: numdots = 1; basebeat = 20; hueinc = 16; faderate = 2; thishue = 0; break; // You can change values here, one at a time , or altogether.
      case 10: numdots = 4; basebeat = 10; hueinc = 16; faderate = 8; thishue = 128; break;
      case 20: numdots = 8; basebeat =  3; hueinc =  0; faderate = 8; thishue = random8(); break; // Only gets called once, and not continuously for the next several seconds. Therefore, no rainbows.
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

uint8_t showSolidColor()
{
  fill_solid(leds, NUM_LEDS, solidColor);

  return 30;
}

uint8_t incrementalDrift() {
  uint8_t stepwidth = 256 * (20 - 1) / NUM_LEDS;
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint8_t bri = beatsin88(1 * 256 + (NUM_LEDS - i) * stepwidth, 0, 256);
    leds[i] = ColorFromPalette(gCurrentPalette, 2.5 * i + gHue, bri, LINEARBLEND);
  }

  return 8;
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

#include "Disk.h"

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

uint8_t decayingOrbits()
{
  dimAll(240);

  const uint8_t cnt = 24;
  
  static uint8_t positions[cnt] = { 255, 255, 255 };

  EVERY_N_MILLISECONDS(15)
  {
    for (uint8_t i = 0; i < cnt; i++)
    {
      uint8_t pos = positions[i];

      if (pos == 255 && random8() < 1)
      {
        pos--;
      }

      if (pos != 255)
      {
        pos--;
      }

      positions[i] = pos;

      if (pos != 255)
        leds[NUM_LEDS - pos] = ColorFromPalette(gCurrentPalette, (255 / cnt) * i);
    }
  }

  return 4;
}

typedef uint8_t (*SimplePattern)();
typedef SimplePattern SimplePatternList[];

// List of patterns to cycle through.  Each is defined as a separate function below.

#include "Twinkles.h"

const SimplePatternList patterns = {
  pride,
  colorWaves,
  incrementalDrift,
  radialPaletteShift,
  paletteArcs,
  decayingOrbits,

  // polar coordinate mapping patterns from Adam Haile:

  chaseSinglePixelAroundOuterRing360,
  chaseSinglePixelAroundOuterRing256,
  chaseRadiusLine360,
  chaseRadiusLine256,
  halfCircleRainbow360,
  halfCircleRainbow256,

  // 1D patterns
  rainbow,
  rainbowWithGlitter,
  rainbowSolid,
  sinelon1,
  bpm1,
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

void setup()
{
  FastLED.addLeds<LED_TYPE, DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS); // , DATA_RATE_MHZ(1)
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 6000);
  FastLED.setCorrection(Typical8mmPixel);
  FastLED.setBrightness(brightness);
  FastLED.setDither(false);
  fill_solid(leds, NUM_LEDS, solidColor);
  FastLED.show();

  Serial.begin(9600);

  // Initialize the IR receiver
  irReceiver.enableIRIn();
  irReceiver.blink13(true);

  loadSettings();

  FastLED.setBrightness(brightness);
  FastLED.setDither(brightness < 255);

  setupRings();
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

