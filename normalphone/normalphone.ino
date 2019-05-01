// papayapeter
// 2019

// libraries -----------------------------------------------
#include <Metro.h>
#include <Bounce2.h>
#include <Keypad.h>
#include <Key.h>
#include <SoftwareSerial.h>
#include <Adafruit_FONA.h>

#define UTILS_ERROR 200

// pins ----------------------------------------------------
// fona pins
const uint8_t FONA_RX  = 9;
const uint8_t FONA_TX  = 8;
const uint8_t FONA_RST = 4;
const uint8_t FONA_RI  = 7;

// hook pins
const uint8_t HOOK     = 18;
const uint8_t HOOK_GND = 19;

// constants -----------------------------------------------
// keymap             pin-y   2    3    5    6    9    10   11   12   13    pin-x
// const char keys[9][9] = {{'-', '-', '-', '-', '-', '-', '-', '-', '-'},  // 2
//                          {'-', '-', '-', '1', '2', '3', 'X', '-', '-'},  // 3
//                          {'-', '-', '-', '4', '5', '6', 'A', '-', '-'},  // 5
//                          {'-', '1', '4', '-', '-', '-', '-', '*', '7'},  // 6
//                          {'-', '2', '5', '-', '-', '-', '-', '0', '8'},  // 9
//                          {'-', '3', '6', '-', '-', '-', '-', '#', '-'},  // 10
//                          {'-', 'X', 'A', '-', '-', '-', '-', 'R', 'D'},  // 11
//                          {'-', '-', '-', '*', '0', '#', 'R', '-', '-'},  // 12
//                          {'-', '-', '-', '7', '8', '9', 'D', '-', '-'}}; // 13
const char keys[4][4] = {{'1', '2', '3', 'X'},
                         {'4', '5', '6', 'A'},
                         {'7', '8', '9', 'D'},
                         {'*', '0', '#', 'R'}};

// objects -------------------------------------------------
// fona
SoftwareSerial fona_ss = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fona_serial = &fona_ss;

Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

// timers
Metro serial(1000); // DEBUG
Metro utils(3000); // battery and signal timer

// variables -----------------------------------------------
// fona communication c-strings
char replybuffer[255];
char number[30];

// keypad-input
String key_input = "+4915756037230";
String last_input = "";

// values for utitlies
uint16_t battery;
uint8_t rssi;

// functions -----------------------------------------------
// output to display
bool display(String text, uint16_t b, uint8_t r);
// check for hook pick up and act
bool checkHook(Adafruit_FONA* f, uint8_t p, uint16_t i, char* number, String* last_number);
// check signal an battery
void checkUtils(Adafruit_FONA* f, uint16_t* b, uint8_t* r);

// setup ---------------------------------------------------
void setup()
{
  // setup serial (DEBUG)
  Serial.begin(115200);

  // set pins
  pinMode(HOOK_GND, OUTPUT);
  digitalWrite(HOOK_GND, LOW);

  // set up sim800 communication
  fona_serial->begin(4800);
  while (!fona.begin(*fona_serial))
  {
    Serial.println("can't find fona");
    delay(1000);
  }
  Serial.println("found fona");
  delay(1000);

  // set audio
  fona.setAudio(FONA_EXTAUDIO);
}

// loop  ----------------------------------------------------
void loop()
{
  // check hook status
  int8_t hook_status = checkHook(HOOK, 25);
  uint8_t call_status = fona.getCallStatus();
  if (hook_status == 1) // if picked up
  {
    if (call_status == 0) // ready -> call
    {
      if (!fona.callPhone(key_input.c_str())) Serial.println("failed to call!");
    }
    else if (call_status == 3) // incoming -> pick up
    {
      if (!fona.pickUp()) Serial.println("failed to pick up!");
    }
  }
  else if (hook_status == -1) // if put down
  {
    if (call_status == 4) // in progress -> hang up
    {
      if (!fona.hangUp()) Serial.println("failed to hang up!");
    }
  }

  // check signal and battery
  if (utils.check()) checkUtils(&fona, &battery, &rssi);

  // DEBUG
  if (serial.check()) display(key_input, battery, rssi);
}

// functions -----------------------------------------------
bool display(String text, uint16_t b, uint8_t r)
{
  Serial.println("S: " + String(r) + "\tB: " + String(b) + "\tK: " + String(text));
}

int8_t checkHook(uint8_t p, uint16_t i)
{
  // static variables
  static Bounce hook = Bounce();
  static bool first = true;

  if (first) // if first -> attach pin and set interval
  {
    hook.attach(p, INPUT_PULLUP);
    hook.interval(i);

    first = false;
  }

  hook.update();
  // if picked up
  if (hook.rose()) return 1;
  // if put down
  else if (hook.fell()) return -1;
  // if nothing happens
  return 0;
}

void checkUtils(Adafruit_FONA* f, uint16_t* b, uint8_t* r)
{
  if (!f->getBattPercent(b)) *b = UTILS_ERROR;
  if (!f->getNetworkStatus()) *r = UTILS_ERROR; else *r = map(f->getRSSI(), 0, 31, 0, 5);
}
