// papayapeter
// 2019

// to finally work, disable all serial communication (slows it down if not connected to a computer somehow)
// and comment out the line "#define ADAFRUIT_FONA_DEBUG" in "FONAConfig.h" in the modified library

// defines ---------------------------------------------------------------------
#define UTILS_ERROR 200

// libraries -------------------------------------------------------------------
#include <Metro.h>
#include <Bounce2.h>
#include <SoftwareSerial.h>
#include <Adafruit_FONA.h>
#include <Keypad.h>
#include <JQ6500_Serial.h>
#include <SendOnlySoftwareSerial.h>
#include <SerLCD.h>
#include <EEPROMex.h>

// settings (things to agjust) -------------------------------------------------
const uint64_t call_delay = 3000; // wait ... milliseconds to call after last dial
const uint8_t bytes_per_number = 15; // how many bytes of storage does any number take up?
const uint16_t max_number_index = EEPROMSizeATmega32u4 / bytes_per_number; // maximum numbers that can be stored

// pins ------------------------------------------------------------------------
// fona pins
const uint8_t FONA_RX  = 9;
const uint8_t FONA_TX  = 8;
const uint8_t FONA_RST = 4;
const uint8_t FONA_RI  = 7;

// led pin
const uint8_t LED = 3;

// jq pins
const uint8_t JQ_RX = 20; // arduino tx (orange)
const uint8_t JQ_TX = 21; // arduino rx (green)

// display pins
const uint8_t DISPLAY_RX = 22; // (green)
const uint8_t DISPLAY_CHECK = 2; // (white)

// hook pins
const uint8_t HOOK_GND = 19;
const uint8_t HOOK = 18;

// keaypad pins
const uint8_t row_pins[4]    = {14, 16, 13, 12};
const uint8_t column_pins[4] = {15, 23, 10, 11};

// constants -------------------------------------------------------------------
// keymap
const char keys[4][4] = {{'1', '2', '3', 'X'},
                         {'4', '5', '6', 'A'},
                         {'7', '8', '9', 'D'},
                         {'*', '0', '#', 'R'}};

// objects ---------------------------------------------------------------------
// fona
SoftwareSerial fona_ss = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fona_serial = &fona_ss;

Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

// jq
JQ6500_Serial ringer(JQ_TX, JQ_RX);

// timers
Metro serial_timer(1000); // DEBUG ***
Metro utils_timer(20000); // battery and signal timer
Metro tone_timer(300); // timer for dial tones
Metro led_timer(2000); // led blink timer
Metro ring_timer(1000); // ring check timer

// keypad
Keypad keypad = Keypad(makeKeymap(keys), row_pins, column_pins, 4, 4);

// display
Bounce display_plugged = Bounce();

// variables -------------------------------------------------------------------
// fona communication c-strings
char replybuffer[255];
char number[30];

// keypad-input
String key_input = "";
String last_input = "";
String key_copy = "";

// values for utitlies
uint16_t battery;
uint8_t rssi;

// timer variables
uint64_t last_key;

// other
bool hook_status;
uint8_t user_status = 0;
uint8_t tone_sequence;
bool led_status = true;
bool ringing;

// random
String random_number = "";
uint8_t number_index;
bool allow_incoming; // allow incoming calls to ring and pick up

// functions -------------------------------------------------------------------
// output to display (input, battery, rssi)
void display(String i, uint16_t b, uint8_t r);
// check for hook pick up and act (pin, debounce interval, hook status pointer)
int8_t checkHook(uint8_t p, uint16_t i, bool* s = NULL);
// check signal an battery (fona pointer, battery pointer, rssi pointer)
void checkUtils(Adafruit_FONA* f, uint16_t* b, uint8_t* r);
// play corresponding keytone(fona pointer, key)
void playKeyTone(Adafruit_FONA* f, char k);
// get current eeprom index
uint8_t getIndex();
// store number in eeprom
bool storeNumber(String n, uint16_t i);
// read number from eeprom (index)
String readNumber(uint16_t i);
// clear all clearNumbers
void clearNumbers(uint16_t i);
// get the status of allow_incoming in eeprom
bool getIncoming();
// set the status of allow_incoming in eeprom
void setIncoming(bool b);

// setup -----------------------------------------------------------------------
void setup()
{
  // DEBUG ***
  Serial.begin(115200);

  // set pins
  pinMode(HOOK_GND, OUTPUT);
  digitalWrite(HOOK_GND, LOW);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  // set debounce time for hook
  keypad.setDebounceTime(3);

  // attach debouncer to display and set time
  display_plugged.attach(DISPLAY_CHECK, INPUT_PULLUP);
  display_plugged.interval(50);

  // set up sim800 communication
  fona_serial->begin(4800);
  while (!fona.begin(*fona_serial))
  {
    Serial.println("can't find fona"); // DEBUG ***

    while (true) // infine fast blink
    {
      digitalWrite(LED, HIGH);
      delay(100);
      digitalWrite(LED, LOW);
      delay(100);
    }
  }
  Serial.println("found fona"); // DEBUG ***
  // one blink to signal startup
  digitalWrite(LED, HIGH);
  delay(1000);
  digitalWrite(LED, LOW);

  // ringer (jq) serial initaiate
  ringer.begin(9600);
  ringer.reset();
  ringer.setVolume(30);
  ringer.setLoopMode(MP3_LOOP_ALL);

  // set audio
  fona.setAudio(FONA_EXTAUDIO);
  fona.setVolume(20);
  fona.setRingerVolume(0);
  fona.setToneVolume(20);

  // check signal and battery
  checkUtils(&fona, &battery, &rssi);

  // random
  number_index = getIndex();
  allow_incoming = getIncoming(); // allow incoming calls to ring and pick up
  EEPROM.setMaxAllowedWrites(8096);

  // display welcoming message
  display_plugged.update();
  if (!display_plugged.read())  display("RANDOMPHONE", battery, rssi);
}

// loop  -----------------------------------------------------------------------
void loop()
{
  // DEBUG ***
  if (serial_timer.check())
  {
    Serial.println("D: " + String(display_plugged.read()) +
                   "\tS: " + String(rssi) +
                   "\tB: " + String(battery) +
                   "\tK: " + key_input +
                   "\tL: " + last_input +
                   "\tI: " + number_index);
  }

  // check whether display is plugged in
  display_plugged.update();
  // if display has just been pulled -------------------------------------------
  if (display_plugged.rose())
  {
    // clear significant variables
    key_input = "";
    key_copy = "";

    // act as if hook is down (if it was up it has to be put down to proceed)
    user_status = 0;
  }
  // else if display has just been pulled --------------------------------------
  else if (display_plugged.fell())
  {
    // clear significant variables
    key_input = "";

    // act as if hook is down (if it was up it has to be put down to proceed)
    user_status = 0;

    // if ringing is not allowed, but the phone is ringing -> stop
    if (!allow_incoming && ringing)
    {
      ringer.pause();

      ringing = false;
    }

    // display name
    display("RANDOMPHONE", battery, rssi);
  }
  // if diplay if plugged out -> normalphone -----------------------------------
  if (display_plugged.read())
  {
    // check to ring -----------------------------------------------------------
    if (ring_timer.check())
    {
      uint8_t call_status = fona.getCallStatus();

      if (call_status == 3 && !ringing) // if incoming call -> ring
      {
        ringer.play();

        ringing = true;
      }
      else if (call_status != 3 && ringing) // no incoming call, but is ringing -> stop
      {
        ringer.pause();

        ringing = false;
      }
    }

    // check hook --------------------------------------------------------------
    int8_t hook_change = checkHook(HOOK, 25, &hook_status);
    if (hook_change == 1) // if picked up
    {
      uint8_t call_status = fona.getCallStatus();

      if (call_status == 3) // if incoming call -> pick up
      {
        ringer.pause();

        ringing = false;

        fona.pickUp();
      }
      else if (key_input == "") // else if nothing dialed -> play dial tone
      {
        fona.playToolkitTone(FONA_STTONE_USADIALTONE, 15300000);

        // enable dialling
        user_status = 1;
      }
      else // else if something was dialed -> go to dial sequence
      {
        // enable calling
        user_status = 2;

        tone_sequence = 0;

        // reset dial tone timer
        tone_timer.reset();
      }
    }
    else if (hook_change == -1) // if put down
    {
      uint8_t call_status = fona.getCallStatus();

      if (call_status == 4) // in progress -> hang up
      {
        fona.hangUp();
      }
      else
      {
        // stop playing dial tone
        fona.stopToolkitTone();
      }

      // clear input
      key_input = "";
      key_copy = "";

      // reset dialability
      user_status = 0;
    }

    // run dial and call routines ----------------------------------------------
    if (user_status == 1) // nothing dialled yet
    {
      if (key_input != "") // play dial tone until something is dialed
      {
        fona.stopToolkitTone();

        if (key_input != key_copy) // if something has been dialled
        {
          // reset timer
          last_key = millis();

          // reset comparison
          key_copy = key_input;
        }
        else if (millis() > last_key + call_delay) // if something has been dialled and time has passed -> call
        {
          // disable dialling
          user_status = 3;

          // save redial number
          last_input = key_input;

          // call
          fona.callPhone(key_input.c_str());

          // clear input
          key_input = "";
          key_copy = "";
        }
      }
      else
      {
        // reset comparison
        key_copy = key_input;
      }
    }
    else if (user_status == 2) // something dialled already
    {
      if (tone_sequence < key_input.length()) // play tones for number in string
      {
        if (tone_timer.check())
        {
          playKeyTone(&fona, key_input[tone_sequence++]);
        }
      }
      else
      {
        // disable dialling
        user_status = 3;

        // save redial number
        last_input = key_input;

        // call
        fona.callPhone(key_input.c_str());

        // clear input
        key_input = "";
        key_copy = "";
      }
    }

    // check keypad ------------------------------------------------------------
    char key = keypad.getKey();

    // check key
    if (key != NO_KEY && key != 'A' && key != 'X' && key != '*' && key != '#')
    {
      // remove
      if (key == 'R' && key_input.length() > 0) key_input.remove(key_input.length() - 1);
      // redial
      else if (key == 'D') key_input = last_input;
      // number
      else if (key != 'R') key_input += String(key);

      // key sound when picked up
      if (user_status > 0)
      {
        playKeyTone(&fona, key);
      }
    }

    // check signal and battery ------------------------------------------------
    if (utils_timer.check())
    {
      checkUtils(&fona, &battery, &rssi);

      // led battery indicator
      if (battery <= 50 && battery > 25)
      {
        digitalWrite(LED, HIGH);
      }
      else if (battery <= 25)
      {
        if (led_timer.check())
        {
          if (led_status) digitalWrite(LED, HIGH);
          else digitalWrite(LED, LOW);
          led_status = !led_status;
        }
      }
    }
  }
  // if diplay if plugged in -> randomphone ------------------------------------
  else
  {
    // check to ring (set allow_incoming to enable/disable) ----------------------
    if (ring_timer.check() && allow_incoming)
    {
      uint8_t call_status = fona.getCallStatus();

      if (call_status == 3 && !ringing) // if incoming call -> ring
      {
        ringer.play();

        ringing = true;
      }
      else if (call_status != 3 && ringing) // no incoming call, but is ringing -> stop
      {
        ringer.pause();

        ringing = false;
      }
    }

    // check hook ----------------------------------------------------------------
    int8_t hook_change = checkHook(HOOK, 25, &hook_status);
    if (hook_change == 1) // if picked up
    {
      uint8_t call_status = fona.getCallStatus();

      if (call_status == 3 && allow_incoming) // if incoming call -> pick up
      {
        ringer.pause();

        ringing = false;

        fona.pickUp();
      }
      else if (call_status != 3 || !allow_incoming) // else if no incoming call/answering is disabled
      {
        if (number_index == 0) // no number is stored -> play dialtone
        {
          fona.playToolkitTone(FONA_STTONE_USADIALTONE, 15300000);

          display("nothing stored", battery, rssi);
        }
        else
        {
          // read random number from eeprom
          random_number = readNumber(random(number_index));

          // goto dial sequence for random number
          user_status = 2;

          tone_sequence = 0;

          // reset dial tone timer
          tone_timer.reset();
        }

        // clear input
        key_input = "";

        // display
        display(key_input, battery, rssi);
      }
    }
    else if (hook_change == -1) // if put down
    {
      uint8_t call_status = fona.getCallStatus();

      if (call_status == 4) // in progress -> hang up
      {
        fona.hangUp();
      }
      else if (number_index == 0) // no number is stored -> stop playing dial tone
      {
        fona.stopToolkitTone();
      }

      // clear input
      key_input = "";

      // update display
      display(key_input, battery, rssi);

      // reset dialability
      user_status = 0;
    }

    // run dial and call routines ------------------------------------------------
    if (user_status == 2) // autodial random number
    {
      if (tone_sequence < random_number.length()) // play tones for number in string
      {
        if (tone_timer.check())
        {
          playKeyTone(&fona, random_number[tone_sequence++]);
        }
      }
      else
      {
        // disable dialling
        user_status = 3;

        // call
        fona.callPhone(random_number.c_str());
      }
    }

    // check keypad --------------------------------------------------------------
    char key = keypad.getKey();

    // check key
    if (user_status == 0 && key != NO_KEY && key != 'D' && key != 'X') // if earpiece put down and only for certain keys
    {
      // remove
      if (key == 'R' && key_input.length() > 0)
      {
        key_input.remove(key_input.length() - 1);

        // update display
        display(key_input, battery, rssi);
      }
      // enter
      else if (key == 'A')
      {
        if (key_input == "*1*") // clear
        {
          clearNumbers(number_index);
          number_index = 0;
          // clear input
          key_input = "";

          // update display
          display("cleared", battery, rssi);
        }
        else if (key_input == "*2*") // allow incoming calls
        {
          allow_incoming = true;
          setIncoming(true);

          // clear input
          key_input = "";

          // update display
          display("incoming on", battery, rssi);
        }
        else if (key_input == "*3*") // disallow incoming calls
        {
          allow_incoming = false;
          setIncoming(false);

          // clear input
          key_input = "";

          // update display
          display("incoming off", battery, rssi);
        }
        else if (key_input == "*4*") // show how many number are saved
        {
          // clear input
          key_input = "";

          // update display
          display("stored: " + String(number_index) + "/" + String(max_number_index), battery, rssi);
        }
        else if (key_input.length() >= 10 && key_input.length() <= 15) // if the number has the right length -> store
        {
          // if storage is full overwrite from index 0
          if (number_index >= max_number_index)
            number_index = 0;

          // check for inegrety of input and store
          if (storeNumber(key_input, number_index++))
            display("stored", battery, rssi); // update display
          else
            display("only numbers", battery, rssi); // update display
          // clear input
          key_input = "";
        }
        else
        {
          // clear input
          key_input = "";

          // update display
          display("too short/long", battery, rssi);
        }
      }
      // number
      else if (key != 'R')
      {
        key_input += String(key);

        // update display
        display(key_input, battery, rssi);
      }
    }

    // check signal and battery --------------------------------------------------
    if (utils_timer.check())
    {
      checkUtils(&fona, &battery, &rssi);

      // update display
      display(key_input, battery, rssi);
    }
  }
}

// functions -------------------------------------------------------------------
void display(String i, uint16_t b, uint8_t r)
{
  // static variables
  static SendOnlySoftwareSerial display_serial(DISPLAY_RX);
  static SerLCD lcd;

  static bool first = true;

  if (first) // if first -> attach pin and set interval
  {
    display_serial.begin(9600);

    lcd.begin(display_serial);

    lcd.setBacklight(255, 0, 0);
    lcd.setContrast(0);

    first = false;
  }

  lcd.clear();

  if (r == UTILS_ERROR)
    lcd.print("ERROR");
  else
    lcd.print(String(r) + "/5");

  if (b == UTILS_ERROR)
  {
    lcd.setCursor(11, 0);
    lcd.print("ERROR");
  }
  else
  {
    if (battery < 10)
      lcd.setCursor(14, 0);
    else
      lcd.setCursor(13, 0);

    lcd.print(String(b) + "%");
  }
  lcd.setCursor(0, 1);
  lcd.print(i);
}

int8_t checkHook(uint8_t p, uint16_t i, bool* s)
{
  // static variables
  static Bounce hook = Bounce();
  static bool first = true;

  if (first) // if first -> attach pin and set interval
  {
    hook.attach(HOOK, INPUT_PULLUP);
    hook.interval(i);

    first = false;
  }

  hook.update();
  // if picked up
  if (hook.rose())
  {
    if (s != NULL) *s = true;
    return 1;
  }
  // if put down
  else if (hook.fell())
  {
    if (s != NULL) *s = false;
    return -1;
  }
  // if nothing happens
  return 0;
}

void checkUtils(Adafruit_FONA* f, uint16_t* b, uint8_t* r)
{
  if (!f->getBattPercent(b)) *b = UTILS_ERROR;
  if (!f->getNetworkStatus()) *r = UTILS_ERROR; else *r = map(f->getRSSI(), 0, 31, 0, 5);
}

void playKeyTone(Adafruit_FONA* f, char k)
{
  const uint16_t tone_length = 200; // min = 100; max = 500; interval = 100
  switch (k)
  {
    case '1': f->playUserXTone(697, 1209, 500, 100, tone_length); break;
    case '2': f->playUserXTone(697, 1336, 500, 100, tone_length); break;
    case '3': f->playUserXTone(697, 1477, 500, 100, tone_length); break;
    case 'X': f->playUserXTone(697, 1633, 500, 100, tone_length); break;
    case '4': f->playUserXTone(770, 1209, 500, 100, tone_length); break;
    case '5': f->playUserXTone(770, 1336, 500, 100, tone_length); break;
    case '6': f->playUserXTone(770, 1477, 500, 100, tone_length); break;
    case 'A': f->playUserXTone(770, 1633, 500, 100, tone_length); break;
    case '7': f->playUserXTone(852, 1209, 500, 100, tone_length); break;
    case '8': f->playUserXTone(852, 1336, 500, 100, tone_length); break;
    case '9': f->playUserXTone(852, 1477, 500, 100, tone_length); break;
    case 'D': f->playUserXTone(852, 1633, 500, 100, tone_length); break;
    case '*': f->playUserXTone(941, 1209, 500, 100, tone_length); break;
    case '0': f->playUserXTone(941, 1336, 500, 100, tone_length); break;
    case '#': f->playUserXTone(941, 1477, 500, 100, tone_length); break;
    case 'R': f->playUserXTone(941, 1633, 500, 100, tone_length); break;
  }
}

uint8_t getIndex()
{
  for (uint16_t i = 1; i < EEPROMSizeATmega32u4; i += bytes_per_number)
  {
    if (EEPROM.read(i) == 0)
      return (i - 1) / bytes_per_number;
  }
}

bool storeNumber(String n, uint16_t i)
{
  for (uint16_t j = 0; j < n.length(); j++)
  {
    if (n[j] < '0' || n[j] > '9') return false;
    EEPROM.update(1 + i * bytes_per_number + j, n[j]);
  }
  return true;
}

String readNumber(uint16_t i)
{
  String out = "";
  for (uint16_t j = 0; j < bytes_per_number; j++)
  {
    char c = EEPROM.read(1 + i * bytes_per_number + j);
    if (c != 0)
      out += String(c);
  }
  return out;
}

void clearNumbers(uint16_t i)
{
  for (uint16_t j = 1; j < i * bytes_per_number; j++)
  {
    EEPROM.update(j, 0);
  }
}

bool getIncoming()
{
  return EEPROM.readBit(0, 0);
}

void setIncoming(bool b)
{
  EEPROM.updateBit(0, 0, b);
}
