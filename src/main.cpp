//
// Remote control for LED-strip
//
// Copyright 2018 tero.saarni@gmail.com
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <Arduino.h>
#include <EEPROM.h>

#include "light_ws2812.h"
#include "IRLibAll.h"



// number of leds across horizontal and vertical edges
const uint8_t num_leds_horizontal = 41;
const uint8_t num_leds_vertical = 22;

// total number of leds
const uint8_t num_leds = (num_leds_horizontal*2) + (num_leds_vertical*2);

// output buffer for writing led values to led strip
struct cRGBW leds[num_leds] = {0};

// swap rgb -> grb
#define COLOR(r,g,b,w) {g,r,b,w}

// color themes for leds.
// four colors per theme, one for each corner of the tv,
// order: top left, top right, bottom left, bottom right
struct cRGBW themes[][4] = {
   {
      COLOR(0,0,0,255), COLOR(0,0,0,255),
      COLOR(0,0,0,255), COLOR(0,0,0,255),
   },

   {
      COLOR(0,0,0,255), COLOR(0,0,0,255),
      COLOR(0,0,0,155), COLOR(0,0,0,155),
   },

   {
      COLOR(0,0,0,155), COLOR(0,0,0,155),
      COLOR(0,0,0,255), COLOR(0,0,0,255),
   },

   {
      COLOR(100,120,170,0), COLOR(100,120,170,0),
      COLOR(20,30,55,0),    COLOR(20,30,55,0),
   },
};


const size_t num_themes = sizeof(themes) / sizeof(themes[0]);


// program state, also persisted in EEPROM
struct {
   uint8_t theme_num;
   uint8_t brightness;
} state;


// ir receiver (pin must support interrupt)
IRrecvPCI ir_receiver(2);
IRdecode ir_decoder;

// ir protocol
const uint8_t ir_protocol_num = SONY;

// button codes
const uint16_t button_theme_next      = 0x32E9;
const uint16_t button_theme_prev      = 0x52E9;
const uint16_t button_brightness_up   = 0x12E9;
const uint16_t button_brightness_down = 0x72E9;



////////////////////////////////////////////////////////////////////////////


// calculate linear color gradient from color_a to color_b
void fill_with_gradient(struct cRGBW color_a, struct cRGBW color_b, struct cRGBW* dest, uint16_t num_values)
{
   double r_step = (double)(color_b.r - color_a.r) / num_values;
   double g_step = (double)(color_b.g - color_a.g) / num_values;
   double b_step = (double)(color_b.b - color_a.b) / num_values;
   double w_step = (double)(color_b.w - color_a.w) / num_values;

   for (uint16_t i=0; i<num_values; i++)
   {
      uint8_t r = r_step * i + color_a.r;
      uint8_t g = g_step * i + color_a.g;
      uint8_t b = b_step * i + color_a.b;
      uint8_t w = w_step * i + color_a.w;
      dest[i].r = r * state.brightness / 100;
      dest[i].g = g * state.brightness / 100;
      dest[i].b = b * state.brightness / 100;
      dest[i].w = w * state.brightness / 100;
   }
}


// write colors out to the leds
void set_theme(struct cRGBW* theme)
{
   Serial.println("updating leds");
   fill_with_gradient(theme[2], theme[0], &leds[0], num_leds_vertical);
   fill_with_gradient(theme[0], theme[1], &leds[num_leds_vertical], num_leds_horizontal);
   fill_with_gradient(theme[1], theme[3], &leds[num_leds_vertical+num_leds_horizontal], num_leds_vertical);
   fill_with_gradient(theme[3], theme[2], &leds[num_leds_vertical+num_leds_horizontal+num_leds_vertical], num_leds_horizontal);
   ws2812_setleds_rgbw(leds, num_leds);
}


// persist program state to eeprom
void persist_program_state()
{
   uint8_t *p = reinterpret_cast<uint8_t*>(&state);
   for (size_t i=0; i<sizeof(state); i++)
   {
      EEPROM.update(i, *p++);
   }
}

// restore program state from eeprom
void restore_program_state()
{
   uint8_t *p = reinterpret_cast<uint8_t*>(&state);
   for (size_t i=0; i<sizeof(state); i++)
   {
      *p++ = EEPROM.read(i);
   }

   // reset defaults when eeprom is uninitialized
   if (state.theme_num  == 255) state.theme_num  = 0;
   if (state.brightness == 255) state.brightness = 100;
   persist_program_state();

   Serial.print("number of leds: ");
   Serial.println(num_leds);
   Serial.print("number of themes: ");
   Serial.println(num_themes);
   Serial.print("program state in eeprom: theme_num=");
   Serial.print(state.theme_num);
   Serial.print(", brightness=");
   Serial.println(state.brightness);
}


void setup()
{
   Serial.begin(115200);
   restore_program_state();

   Serial.print("setting theme: ");
   Serial.println(state.theme_num);

   set_theme(themes[state.theme_num]);
   ir_receiver.enableIRIn();
}



void loop()
{
   if (ir_receiver.getResults())
   {
      ir_decoder.decode();
      //ir_decoder.dumpResults(true);

      if (ir_decoder.protocolNum == ir_protocol_num)
      {
         bool state_changed = false;

         switch (ir_decoder.value)
         {
            case button_theme_next:
               state.theme_num = (state.theme_num + 1) % num_themes;
               Serial.print("next theme: ");
               Serial.println(state.theme_num);
               state_changed = true;
               break;

            case button_theme_prev:
               state.theme_num = (state.theme_num + num_themes - 1) % num_themes;
               Serial.print("previous theme: ");
               Serial.println(state.theme_num);
               state_changed = true;
               break;

            case button_brightness_up:
               state.brightness = (state.brightness + 10) > 100 ? 100 : state.brightness + 10;
               Serial.print("brightness up: ");
               Serial.println(state.brightness);
               state_changed = true;
               break;

            case button_brightness_down:
               state.brightness = (state.brightness - 10) < 10 ? 10 : state.brightness - 10;
               Serial.print("brightness down: ");
               Serial.println(state.brightness);
               state_changed = true;
               break;

            default:
               break;
         }
         if (state_changed)
         {
            set_theme(themes[state.theme_num]);
            persist_program_state();
            delay(150);  // prevent too fast repeated button presses
         }
      }
   }
   ir_receiver.enableIRIn();
}
