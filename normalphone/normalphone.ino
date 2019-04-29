// papayapeter
// 2019

// libraries -----------------------------------------------
#include <Metro.h>
#include <Bounce2.h>
#include <SoftwareSerial.h>
#include <Adafruit_FONA.h>

#define UTILS_ERROR 200

// pins ----------------------------------------------------
const uint8_t FONA_RX  = 9;
const uint8_t FONA_TX  = 8;
const uint8_t FONA_RST = 4;
const uint8_t FONA_RI  = 7;

const uint8_t HOOK     = 18;
const uint8_t HOOK_GND = 19;

const uint8_t KEYPAD[9] = {2, 3, 5, 6, 9, 10, 11, 12, 13};

// constants -----------------------------------------------
// keymap              y   0    1    2    3    4    5    6    7    8        x
const char keys[9][9] = {{'-', '-', '-', '-', '-', '-', '-', '-', '-'},  // 0
                         {'-', '-', '-', '1', '2', '3', 'X', '-', '-'},  // 1
                         {'-', '-', '-', '4', '5', '6', 'A', '-', '-'},  // 2
                         {'-', '1', '4', '-', '-', '-', '-', '*', '7'},  // 3
                         {'-', '2', '5', '-', '-', '-', '-', '0', '8'},  // 4
                         {'-', '3', '6', '-', '-', '-', '-', '#', '-'},  // 5
                         {'-', 'X', 'A', '-', '-', '-', '-', 'R', 'D'},  // 6
                         {'-', '-', '-', '*', '0', '#', 'R', '-', '-'},  // 7
                         {'-', '-', '-', '7', '8', '9', 'D', '-', '-'}}; // 8

// objects -------------------------------------------------
SoftwareSerial fona_ss = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fona_serial = &fona_ss;

Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

Bounce hook = Bounce();

Metro utils(3000); // battery and signal timer
Metro serial(1000);

// variables -----------------------------------------------
char replybuffer[255];
char number[30];
String key_input;

uint16_t battery;
uint8_t rssi;

// setup ---------------------------------------------------
void setup()
{
  // set pins
  pinMode(HOOK_GND, OUTPUT);
  digitalWrite(HOOK_GND, LOW);
  hook.attach(HOOK, INPUT_PULLUP);
  hook.interval(25);

  // setup serial (debug)
  Serial.begin(115200);

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

  // check hook status
  hook.update();
  if (hook.rose()) // if picked up
  {
    uint8_t call_status = fona.getCallStatus();
    if (call_status == 0) // ready -> call
    {
      // CALL
    }
    else if (call_status == 3) // incoming -> pick up
    {
      if (!fona.pickUp()) Serial.println("failed!");
    }
  }
  else if (hook.fell()) // if put down
  {
    uint8_t call_status = fona.getCallStatus();
    if (call_status == 4) // in progress -> hang up
    {
      if (!fona.hangUp()) Serial.println("failed!");
    }
  }

  // check for battery and signal every 3 seconds
  if (utils.check())
  {
    if (!fona.getBattPercent(&battery)) battery = UTILS_ERROR;
    if (!fona.getNetworkStatus()) rssi = UTILS_ERROR; else rssi = map(fona.getRSSI(), 0, 31, 0, 5);
  }
}
