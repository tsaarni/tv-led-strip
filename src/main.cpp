//
// Remote control for LED-strip
//
// Copyright (C) 2018 tero.saarni@gmail.com
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

// output buffer for writing led values out
struct cRGBW leds[num_leds] = {0};


#define COLOR(r,g,b,w) {g,r,b,w}

// color themes for LEDS. Four colors per theme: top left, top right, bottom left, bottom right
struct cRGBW themes[][4] = {
   {
      COLOR(0,0,0,255), COLOR(0,0,0,255),
      COLOR(0,0,0,255), COLOR(0,0,0,255)
   },

   {
      COLOR(0,0,0,255), COLOR(0,0,0,255),
      COLOR(0,0,0,255), COLOR(0,0,0,255),
   },

   {
      COLOR(100,120,170,0), COLOR(100,120,170,0),
      COLOR(50,60,85,0),    COLOR(50,60,85,0),
   },
};


const size_t num_themes = sizeof(themes) / sizeof(themes[0]);


// program status, also persisted in EEPROM
int8_t current_theme;
int8_t brightness;


// IR receiver
IRrecvPCI ir_receiver(2);
IRdecode ir_decoder;

// button codes
const uint16_t button_theme_next      = 0x32E9;
const uint16_t button_theme_prev      = 0x52E9;
const uint16_t button_brightness_up   = 0x12E9;
const uint16_t button_brightness_down = 0x72E9;



////////////////////////////////////////////////////////////////////////////


// calculate linear color gradient from color_a to color_b
void fill_with_gradient(struct cRGBW color_a, struct cRGBW color_b, struct cRGBW* dest, uint16_t num_values)
{
   double r_steps = (double)(color_b.r - color_a.r) / num_values;
   double g_steps = (double)(color_b.g - color_a.g) / num_values;
   double b_steps = (double)(color_b.b - color_a.b) / num_values;
   double w_steps = (double)(color_b.w - color_a.w) / num_values;

   for (uint16_t i=0; i<num_values; i++)
   {
      uint8_t r = r_steps * i + color_a.r;
      uint8_t g = g_steps * i + color_a.g;
      uint8_t b = b_steps * i + color_a.b;
      uint8_t w = w_steps * i + color_a.w;
      dest[i].r = r * brightness / 100;
      dest[i].g = g * brightness / 100;
      dest[i].b = b * brightness / 100;
      dest[i].w = w * brightness / 100;
   }
}


// write colors out to the leds
void set_theme(struct cRGBW* theme)
{
   Serial.println("update leds");
   fill_with_gradient(theme[2], theme[0], &leds[0], num_leds_vertical);
   fill_with_gradient(theme[0], theme[1], &leds[num_leds_vertical], num_leds_horizontal);
   fill_with_gradient(theme[1], theme[3], &leds[num_leds_vertical+num_leds_horizontal], num_leds_vertical);
   fill_with_gradient(theme[3], theme[2], &leds[num_leds_vertical+num_leds_horizontal+num_leds_vertical], num_leds_horizontal);
   ws2812_setleds_rgbw(leds, num_leds);
}


// persist program state to EEPROM
void persist_program_state()
{
   EEPROM.update(0, current_theme);
   EEPROM.update(1, brightness);
}

// restore program state from EEPROM
void restore_program_state()
{
   current_theme = EEPROM.read(0);
   brightness = EEPROM.read(1);

   // reset defaults when EEPROM is uninitialized
   if (current_theme == 255) current_theme = 0;
   if (brightness == 255)    brightness = 100;
   persist_program_state();

   Serial.print("number of leds: ");
   Serial.println(num_leds);
   Serial.print("number of themes: ");
   Serial.println(num_themes);
   Serial.print("program state in eeprom: current_theme=");
   Serial.print(current_theme);
   Serial.print(", brightness=");
   Serial.println(brightness);
}


void setup()
{
   Serial.begin(115200);
   restore_program_state();

   Serial.print("setting theme: ");
   Serial.println(current_theme);

   set_theme(themes[current_theme]);
   ir_receiver.enableIRIn();
}



void loop()
{
   if (ir_receiver.getResults())
   {
      ir_decoder.decode();
      //ir_decoder.dumpResults(true);

      if (ir_decoder.protocolNum == SONY)
      {
         bool state_changed = false;

         switch (ir_decoder.value)
         {
            case button_theme_next:
               current_theme++;
               if (current_theme == num_themes)
                  current_theme = 0;
               Serial.print("next theme: ");
               Serial.println(current_theme);
               state_changed = true;
               break;

            case button_theme_prev:
               current_theme--;
               if (current_theme < 0)
                  current_theme = num_themes - 1;
               Serial.print("previous theme: ");
               Serial.println(current_theme);
               state_changed = true;
               break;

            case button_brightness_up:
               Serial.print("brightness up: ");

               brightness = brightness + 10;
               if (brightness > 100)
                  brightness = 100;

               Serial.println(brightness);
               state_changed = true;
               break;

            case button_brightness_down:
               Serial.print("brightness down: ");

               brightness = brightness - 10;
               if (brightness < 10)
                  brightness = 10;

               Serial.println(brightness);
               state_changed = true;
               break;

            default:
               break;
         }
         if (state_changed)
         {
            set_theme(themes[current_theme]);
            persist_program_state();
            delay(150);
         }
      }
   }
   ir_receiver.enableIRIn();
}
