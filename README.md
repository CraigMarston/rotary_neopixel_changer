# rotary_neopixel_changer
C++ project for Arduino Nano / Uno (ATMega328) to control WS2812 RGB LEDs (NeoPixels) with a rotary encoder. Also includes a TFT screen via SPI.

This is my first attempt at using a rotary encoder, and there are many excellent well-written examples available. This one in particular stood out to me: http://www.hobbytronics.co.uk/arduino-tutorial6-rotary-encoder

I have amalgamated many examples, so the majority of this code is not original..!

The rotary encoder is connected to the two hardware interrupts int0 and int 1 of the ATMega328, and the push-button is merely polled. Rotating the device changes the value of a variable which is used to set various parameters. Pressing the button sequences through a 'case' function to choose which parameter to change. To add more parameters, this part of the sketch would need altering and most likely refining — something I'd like to see!

The WS2812 LEDs are driven using the HSV conversion, and here is a brilliant explanation: https://www.arduinoslovakia.eu/blog/2018/4/neopixel-ring-hsv-test?lang=en

Also, I've stuck with the NeoPixel library rather than FastLED because I want to use RGBW LEDs with this project. They have a fourth white LED which facilitates a wider gamut of colour, including pastel shades — PROPER rainbows!!

The hue parameter for the NeoPixels is 16-bit (0 - 65535) but I've used the 'map' function to convert 255 steps (twiddles of the rotary encoder!) to cover this much larger value. Now this does mean that a lot of hue values are skipped, but it saves over 65000 twiddles to get round the colour wheel!!
Talking of colour wheels: https://learn.adafruit.com/adafruit-neopixel-uberguide/arduino-library-use#hsv-hue-saturation-value-colors-dot-dot-dot-17-41 — this should explain where the numbers come from.

The TFT I used is a generic cheapo 128x128 pixel, slightly wonky one via AliExpress. 
I connected a different screen and my icons were all displaced vertically. I don't know if there's a simple asjustment for this, y'know like you have a desktop computer screen, but I couldn't find anything with preliminary internet searches…

The colour-space for TFT screens is a right old PITA!!! It's RGB565 and has four characters representing the 3 channels we're familiar with. So, greys look like this: 

cementGREY  0xBDF7
LIGHTGREY   0xC618
steelGREY   0xE71C
DARKGREY    0x7BEF

instead of equal values e.g. 0x555555 or 0xCCCCCC in RGB888. It's hideous and you'll need to use an online converter to find your own colours. And they won't be exact either, just so you know.

Please, please let me know of any improvements you can make as I still consider myself new to this — thanks!
