/*
Copyright (c) 2024, Silas Hille
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.

*/

// definition for all LEDs on the neopixel strip

#define LED_NUM 6 // Number of LEDs

#define LED_TEAM 0       // LED that shows the team_color from below
#define LED_ALIVE 1      // LED that shows if you're alive
#define LED_AMMUNITION 2 // LEDs brightness shows the amount of ammonition
#define LED_SHOOTING 3   // LED blinks when pressing the shoot-button as an indicator
#define LED_VEST_CONNECTED 5

const uint32_t alive_color = 0x7F7F7F;   // not full brightness white
const uint32_t bullet_led_color = 45511; // HSV color from 0= red to 65536 = red
