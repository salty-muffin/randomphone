// papayapeter
// 2019

// libraries -----------------------------------------------
#include <Metro.h>
#include <Bounce2.h>
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

// keypad pins
const uint8_t KEYPAD[9] = {2, 3, 5, 6, 9, 10, 11, 12, 13};

// constants -----------------------------------------------
// keymap                 y   0    1    2    3    4    5    6    7    8        x
// const char keys[9][9] = {{'-', '-', '-', '-', '-', '-', '-', '-', '-'},  // 0
//                          {'-', '-', '-', '1', '2', '3', 'X', '-', '-'},  // 1
//                          {'-', '-', '-', '4', '5', '6', 'A', '-', '-'},  // 2
//                          {'-', '1', '4', '-', '-', '-', '-', '*', '7'},  // 3
//                          {'-', '2', '5', '-', '-', '-', '-', '0', '8'},  // 4
//                          {'-', '3', '6', '-', '-', '-', '-', '#', '-'},  // 5
//                          {'-', 'X', 'A', '-', '-', '-', '-', 'R', 'D'},  // 6
//                          {'-', '-', '-', '*', '0', '#', 'R', '-', '-'},  // 7
//                          {'-', '-', '-', '7', '8', '9', 'D', '-', '-'}}; // 8
const char keys[4][4] = {{'1', '2', '3', 'X'},
                         {'4', '5', '6', 'A'},
                         {'7', '8', '9', 'D'},
                         {'*', '0', '#', 'R'}};

// objects -------------------------------------------------
// fona
SoftwareSerial fona_ss = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fona_serial = &fona_ss;

Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

// debounce for hook and keypad columns
Bounce hook = Bounce();
Bounce column[4];

// timers
Metro serial(1000); // DEBUG
Metro utils(3000); // battery and signal timer
Metro read_keys(10);

// variables -----------------------------------------------
// fona communication c-strings
char replybuffer[255];
char number[30];

// keypad-input
String key_input = "";
String last_input = "";

// values for utitlies
uint16_t battery;
uint8_t rssi;

// pin translations for keypad rows
uint8_t row[4] = {1, 2, 8, 7};

// row counter
uint8_t row_counter = 0;

// setup ---------------------------------------------------
void setup()
{
  // setup serial (DEBUG)
  Serial.begin(115200);

  // set pins
  pinMode(HOOK_GND, OUTPUT);
  digitalWrite(HOOK_GND, LOW);
  hook.attach(HOOK, INPUT_PULLUP);
  hook.interval(25);

  for (uint8_t i; i < 4; i++) // set keypad rows
  {
    pinMode(row[i], OUTPUT);
    digitalWrite(row[i], HIGH);
  }

  for (uint8_t i; i < 4; i++) // set keypad columns
  {
    column[i] = Bounce();
    column[i].attach(KEYPAD[i + 3], INPUT_PULLUP);
    column[i].interval(25);
  }

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
  // DEBUG
  if (serial.check())
  {
    Serial.println("S: " + String(rssi) + "/5\tB: " + String(battery) + "\tH: " + String(hook.read()) +  "\tK: " + key_input);
  }

  // read keypad
  if (read_keys.check())
  {
    // read columns
    for (uint8_t i = 0; i < 4; i++)
    {
      if (column[i].fell())
      {
        if (i != 3) key_input += keys[row_counter][i]; // numbers
        else if (row_counter == 2 && last_input.length() > 0) key_input = last_input; // redail key
        else if (row_counter == 3 key_input.length() > 0) key_input.remove(key_input.length() - 1); // R (remove) key
      }
    }
    // set rows
    for (uint8_t i = 0; i < 4; i++)
    {
      if (i == row_counter) digitalWrite(rows[i], LOW);
      else digitalWrite(rows[i], HIGH);
    }
    if (++row_counter >= 4) row_counter = 0;
  }

  // check hook status
  hook.update();
  if (hook.rose()) // if picked up
  {
    uint8_t call_status = fona.getCallStatus();
    if (call_status == 0) // ready -> call
    {
      if (!fona.callPhone(key_input.c_str())) Serial.println("failed to call!");
    }
    else if (call_status == 3) // incoming -> pick up
    {
      if (!fona.pickUp()) Serial.println("failed to pick up!");
    }
  }
  else if (hook.fell()) // if put down
  {
    uint8_t call_status = fona.getCallStatus();
    if (call_status == 4) // in progress -> hang up
    {
      if (!fona.hangUp()) Serial.println("failed to hang up!");
    }
  }

  // check for battery and signal every 3 seconds
  if (utils.check())
  {
    if (!fona.getBattPercent(&battery)) battery = UTILS_ERROR;
    if (!fona.getNetworkStatus()) rssi = UTILS_ERROR; else rssi = map(fona.getRSSI(), 0, 31, 0, 5);
  }
}
