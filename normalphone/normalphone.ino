// papayapeter
// 2019

// to finally work, disable all serial communication (slows it down if not connected to a computer somehow)
// and comment out the line "#define ADAFRUIT_FONA_DEBUG" in "FONAConfig.h" in the modified library

// defines -------------------------------------------------
#define UTILS_ERROR 200

// libraries -----------------------------------------------
#include <Metro.h>
#include <Bounce2.h>
#include <SoftwareSerial.h>
#include <Adafruit_FONA.h>
#include <Keypad.h>
#include <JQ6500_Serial.h>
#include <SendOnlySoftwareSerial.h>
#include <SerLCD.h>

// pins ----------------------------------------------------
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

// constants -----------------------------------------------
// keymap
const char keys[4][4] = {{'1', '2', '3', 'X'},
                         {'4', '5', '6', 'A'},
                         {'7', '8', '9', 'D'},
                         {'*', '0', '#', 'R'}};

// wait ... milliseconds to call after last dial
const uint64_t call_delay = 3000;

// objects -------------------------------------------------
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

// variables -----------------------------------------------
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

// functions -----------------------------------------------
// output to display
void display(String i, uint16_t b, uint8_t r);
// check for hook pick up and act
int8_t checkHook(uint8_t p, uint16_t i, bool* s = NULL);
// check signal an battery
void checkUtils(Adafruit_FONA* f, uint16_t* b, uint8_t* r);
// play corresponding keytone
void playKeyTone(Adafruit_FONA* f, char k);

// setup ---------------------------------------------------
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
  keypad.setDebounceTime(5);

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

  // display welcoming message
  display_plugged.update();
  if (!display_plugged.read())  display("RANDOMPHONE", battery, rssi);
}

// loop  ----------------------------------------------------
void loop()
{
  // check to ring
  if (ring_timer.check())
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
    else // else if something was dialed
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

    // display
    display_plugged.update();
    if (!display_plugged.read())  display(key_input, battery, rssi);

    // reset dialability
    user_status = 0;
  }

  // run dial and call routines
  if (user_status == 1) // nothing dialled
  {
    if (key_input != "")
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

        // display
        display_plugged.update();
        if (!display_plugged.read())  display(key_input, battery, rssi);
      }
    }
    else
    {
      // reset comparison
      key_copy = key_input;
    }
  }
  else if (user_status == 2) // something dialled
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

      // display
      display_plugged.update();
      if (!display_plugged.read())  display(key_input, battery, rssi);
    }
  }

  // check keypad
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

    // display
    display_plugged.update();
    if (!display_plugged.read())  display(key_input, battery, rssi);
  }

  // check signal and battery
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

    // display
    display_plugged.update();
    if (!display_plugged.read())  display(key_input, battery, rssi);
  }

  // DEBUG
  if (serial_timer.check())
  {
    display_plugged.update();
    if (display_plugged.fell())
    {
      display(key_input, battery, rssi);
    }
    Serial.println("D: " + String(display_plugged.read()) + "\tS: " + String(rssi) + "\tB: " + String(battery) + "\tK: " + key_input + "\tL: " + last_input);
  }
}

// functions -----------------------------------------------
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
