# HomeAssistant entity sensor ESP OLED display
Display HA sensors like temperature or power consumption on a tiny OLED 1,3" display (other resolutions should work too, but code modifications might be required)

![20221030_200049](https://user-images.githubusercontent.com/59659167/198964605-3b8d9dac-153a-4761-849e-2428d5607474.jpg)

# Hardware
You need:
- Some ESP8266 board
- 1,3" OLED display
- 3 buttons
- Optional: 3D-Printed case*

I am using a NodeMCUv2 and these OLED displays: https://smile.amazon.de/gp/product/B07FYG8MZN/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1
I don't know how it works with other hardware, but displays with a different resolution might require code modifications, other than u8g2 display change.

*Case:
The case isn't finished yet and doesn't fit 100%.
To place the NodeMCU v2 inside it, you have to knip the 2 front pins (near the usb hole) and a third one at the back.
The usb connector to the top is also pretty tight.
I also didn't made a cover yet.

Then connect everything.<br>
There are 3 buttons (pins can be changed of course):
- pin 12: back button - to go a page back to the last entity
- pin 13: set button - currently only to toggle the display on and off
- pin 14: forward button - to go a page forward to the next entity

# Installation
- Open the project in Arduino IDE
- Install required libraries*
- Configure WiFi, HA Token and entities
- Upload to your ESP

*Libraries:
I don't know exactly what I have installed.
From what I know:
- U8G2 by oliver
- NTPClient by Fabrice Weinberg
- ArduinoJSON by Benoit Blanchon
- ArduinoHttpClient by Arduino

It will boot up and should show a little startup-screen while connecting to WiFi.
After that it should show one of your configured entities. It automatically gets the entity name, state and unit.
You only need to configure the entity id.
You can use the forward and back button to "scroll" trough your entities.
With the "set"-button you can toggle the display on and off ("standby").



# Planned
- Make the display dimmable
- Turn on WiFi sleep while the display is toggled off (Modem-Sleep: https://diyi0t.com/how-to-reduce-the-esp8266-power-consumption/)


