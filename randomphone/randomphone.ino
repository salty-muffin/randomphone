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

// settings --------------------------------------------------------------------
const uint64_t call_delay = 3000; // wait ... milliseconds to call after last dial
const boolean allow_incoming = true; // allow incoming calls to ring and pick up

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
JQ6500_Serial ringer(JQ_TX, JQ_RX);

Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

// timers
Metro serial_timer(1000); // DEBUG
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
void storeNumber(String n)
// read number from eeprom (index)
String readNumber(uint8_t i)
// clear all clearNumbers
void clearNumbers();

// setup -----------------------------------------------------------------------
void setup()
{
  // setup serial (DEBUG)
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
  fona_serial->begin(38400);
  while (!fona.begin(*fona_serial))
  {
    Serial.println("can't find fona");

    while (true) delay(10);
  }
  Serial.println("found fona");
  digitalWrite(LED, HIGH);
  delay(1000);
  digitalWrite(LED, LOW);

  // ringer serial initaiate
  ringer.begin(9600);
  ringer.reset();
  ringer.setVolume(30);
  ringer.setLoopMode(MP3_LOOP_ALL);

  // set audio
  fona.setAudio(FONA_EXTAUDIO);
  fona.setVolume(20);
  fona.setRingerVolume(0);

  // check signal and battery
  checkUtils(&fona, &battery, &rssi);

  // random
  number_index = getIndex();

  // display welcoming message
  display("RANDOMPHONE", battery, rssi);
}

// loop  ------------------------------------------------------------------------
void loop()
{
  // check to ring (set allow_incoming to enable/disable)
  if (ring_timer.check() && allow_incoming)
  {
    if (fona.getCallStatus() == 3 && !ringing)
    {
      ringer.play();

      ringing = true;
    }
    else if (fona.getCallStatus() != 3 && ringing)
    {
      ringer.pause();

      ringing = false;
    }
  }
  // check hook
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
    else if (call_status != 3 || !allow_incoming)// else if no incoming call / answering is disabled
    {
      if (number_index == 0) // no number is stored -> play dialtone
      {
        fona.playToolkitTone(FONA_STTONE_USADIALTONE, 15300000);
      }
      else
      {
        // read random number from eeprom
        random_number = readNumber(random(number_index));

        // start calling random number
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

    // display
    display(key_input, battery, rssi);

    // reset dialability
    user_status = 0;
  }

  // run dial and call routines
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

  // check keypad
  char key = keypad.getKey();

  // check key
  if (user_status == 0 && key != NO_KEY && key != 'D' && key != 'X') // if earpiece put down and only for certain keys
  {
    // remove
    if (key == 'R' && key_input.length() > 0) key_input.remove(key_input.length() - 1);
    // enter
    else if (key == 'A')
    {
      if (key_input == "*1*") // clear
      {
        clearNumbers();
        number_index = 0;
        // clear input
        key_input = "";

        // display
        display("cleared", battery, rssi);
      }
      else if (key_input == "*2*") // allow incoming calls
      {
        storeNumber(key_input);
        allow_incoming = true;

        // clear input
        key_input = "";

        // display
        display("incoming on", battery, rssi);
      }
      else if (key_input == "*3*") // disallow incoming calls
      {
        storeNumber(key_input);
        allow_incoming = false;

        // clear input
        key_input = "";

        // display
        display("incoming off", battery, rssi);
      }
      else if (key_input.length() >= 10 && key_input.length() <= 15) // if the number has the right length -> store
      {
        if (number_index <= max_number_index) // stace in eeprom left
        {
          storeNumber(key_input);
          number_index++;
          // clear input
          key_input = "";

          // display
          display("stored", battery, rssi);
        }
        else // no space left
        {
          // clear input
          key_input = "";

          // display
          display("no more storage", battery, rssi);
        }
      }
    }
    // number
    else if (key != 'R') key_input += String(key);

    // display
    display(key_input, battery, rssi);
  }

  // check signal and battery
  if (utils_timer.check())
  {
    checkUtils(&fona, &battery, &rssi);

    // display
    display(key_input, battery, rssi);
  }

  // DEBUG
  if (serial_timer.check())
    Serial.println("D: " + String(display_plugged.read()) + "\tS: " + String(rssi) + "\tB: " + String(battery) + "\tK: " + key_input + "\tL: " + last_input);
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

}

void storeNumber(String n)
{

}

String readNumber(uint8_t i)
{

}

void clearNumbers()
{

}