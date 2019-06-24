# randomphone
hacked landline phone that calls random stored numbers

* [x] created a normal phone that can dial, call, ring and answer
* [x] adjust debounce time on keypad down (if possible)
* [x] attach display
* [x] create random phone
* [x] merge (so that with display plugged in it will be the random phone and without display it will be a normal cell phone disguised as an old landline phone)
* [ ] put in capacitors to filter out noise

## libraries
* [Metro](https://github.com/thomasfredericks/Metro-Arduino-Wiring) by Thomas Fredericks
* [Bounce2](https://github.com/thomasfredericks/Bounce2) by Thomas Fredericks
* [Adafruit_FONA (modified)](https://github.com/papayapeter/Adafruit_FONA)
* [Keypad](https://github.com/Chris--A/Keypad) by Christopher Andrews
* [JQ6500_Serial](https://github.com/sleemanj/JQ6500_Serial) by James Sleeman
* [SendOnlySoftwareSerial](https://github.com/nickgammon/SendOnlySoftwareSerial) by Nick Gammon
* [Sparkfun Serial LCD](https://github.com/sparkfun/SparkFun_SerLCD_Arduino_Library)
* [EEPROMex](https://github.com/thijse/Arduino-EEPROMEx) by Thijs Elenbaas

## hardware
* german telephone made by deutsche post from 1991
* [Adafruit Feather Fona](https://www.adafruit.com/product/3027)
* 2000mAh LiPo battery
* JQ6500 module
* electret microphone similar to [this](https://www.adafruit.com/product/1064)
* [Adafruit 8 Ohm 0.25 W plastic speaker](https://www.adafruit.com/product/1891) for the earpiece
* [Adafruit 4 Ohm 3 W speaker](https://www.adafruit.com/product/3351) for ringing
* [Sparkfun 16x2 LCD display](https://www.sparkfun.com/products/14073)
* [panel mount USB extender](https://www.adafruit.com/product/937)

## other
* ring sound from [freesound.org](https://freesound.org/people/Timbre/sounds/391870/) by Timbre which is a modification of [this recording](https://freesound.org/people/inchadney/sounds/391215/) by inchadney

## manual
When the display isn't plugged in it will function like a normal old landline phone (except that it runs on a battery). In other words you can dial, call, be called, pick and hang up normally. The R-key functions to remove last input. The Redial-key (two small overlapping circles) doesn't need any further explanation I think.
When the display is plugged in it functions as the RANDOMPHONE. you can only input a number when the earpiece is on the hook. with the Enter-key (the arrow pointing into the square). That number is anonymously stored. If the earpiece is picked up, a random number from the storage is called. The R-key functions as mentioned, the Redial-key doesn't do anything anymore.
You can type in certain combination to view and change and view settings:
* \*1\* clears all numbers from storage
* \*2\* allows incoming calls
* \*3\* doesn't allow incoming calls
* \*4\* displays how many numbers are and can be stored
