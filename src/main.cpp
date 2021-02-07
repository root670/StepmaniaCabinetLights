/*
 * Decode SextetStream data to control a strip of RGB LEDs for cabinet lights
 * 
 * Copyright 2021 Wesley Castro
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <Arduino.h>
#include <FastLED.h>

#define NUM_LEDS    60
#define BRIGHTNESS  255
#define DATA_PIN    3
#define CHIPSET     NEOPIXEL
#define UPDATE_RATE 100
#define FADE_RATE   30

// Raw LED colors (no color correction)
CRGB leds[NUM_LEDS];

// Actual colors sent to the LEDs (with color correction)
CRGB framebuffer[NUM_LEDS];

// Buffer for receiving serial data
static uint8_t s_pLightData[14];

// Current state of cabinet lights
static uint8_t s_nCabinetLights = 0;

// Bits indicating a light as being active
typedef enum lightMask
{
    // Byte 0
    enumLightMaskMarqueeUpperLeft       = 0x01,
    enumLightMaskMarqueeUpperRight      = 0x02,
    enumLightMaskMarqueeLowerLeft       = 0x04,
    enumLightMaskMarqueeLowerRight      = 0x08,
    enumLightMaskBassLeft               = 0x10,
    enumLightMaskBassRight              = 0x20,
} lightMask_t;

#define CABINET_LIGHT_ACTIVE(mask)  (s_nCabinetLights & mask)

void setup()
{
  FastLED.addLeds<CHIPSET, DATA_PIN>(framebuffer, NUM_LEDS);
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);

  // Display test sequence
  for(int nColor = 0; nColor < 3; nColor++)
  {
        for(int i = 0; i < NUM_LEDS; i++)
        {
            CRGB &color = framebuffer[i];
            color.r = (nColor == 0) ? 255 : 0;
            color.g = (nColor == 1) ? 255 : 0;
            color.b = (nColor == 2) ? 255 : 0;
        }

        FastLED.show();
        FastLED.delay(200);
    }

    Serial.begin(9600);
}

// Source: https://learn.adafruit.com/led-tricks-gamma-correction/the-quick-fix
const uint8_t PROGMEM s_gamma8[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255
};

// Apply gamma correction to LED array
static inline void colorCorrect() __attribute__((always_inline));
static inline void colorCorrect()
{
    for(int nLED = 0; nLED < NUM_LEDS; nLED++)
    {
        const CRGB &src = leds[nLED];
        CRGB &dst = framebuffer[nLED];
        dst.r = pgm_read_byte(&s_gamma8[src.r]);
        dst.g = pgm_read_byte(&s_gamma8[src.g]);
        dst.b = pgm_read_byte(&s_gamma8[src.b]);
    }
}

void loop()
{
    if(Serial.available())
    {
        char c = Serial.read();
        if(c >= 0x30 && c <= 0x6F)
        {
            // Light state. Read remaining 13 bytes of stream
            s_pLightData[0] = c;
            Serial.readBytes(s_pLightData+1, 13);
            s_nCabinetLights = s_pLightData[0];
        }
    }

    fadeToBlackBy(leds, NUM_LEDS, FADE_RATE);

    if(CABINET_LIGHT_ACTIVE(enumLightMaskMarqueeUpperLeft))
    {
        for(int i = 0; i < 10; i++)
            leds[i] = CRGB::Yellow;
    }

    if(CABINET_LIGHT_ACTIVE(enumLightMaskMarqueeLowerLeft))
    {
        for(int i = 10; i < 20; i++)
            leds[i] = CRGB::Red;
    }

    if(CABINET_LIGHT_ACTIVE(enumLightMaskBassLeft))
    {
        for(int i = 20; i < 30; i++)
            leds[i] = CRGB(212, 0, 255);
    }

    if(CABINET_LIGHT_ACTIVE(enumLightMaskBassRight))
    {
        for(int i = 30; i < 40; i++)
            leds[i] = CRGB(212, 0, 255);
    }

    if(CABINET_LIGHT_ACTIVE(enumLightMaskMarqueeLowerRight))
    {
        for(int i = 40; i < 50; i++)
            leds[i] = CRGB::Red;
    }

    if(CABINET_LIGHT_ACTIVE(enumLightMaskMarqueeUpperRight))
    {
        for(int i = 50; i < 60; i++)
            leds[i] = CRGB::Yellow;
    }
    
    colorCorrect();
    FastLED.show();
    FastLED.delay(1000/UPDATE_RATE);
}
