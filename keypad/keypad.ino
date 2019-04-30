// papayapeter
// 2019

// libraries -----------------------------------------------
#include <Metro.h>
#include <Bounce2.h>

// pins ----------------------------------------------------
// hook pins
const uint8_t HOOK     = 18;
const uint8_t HOOK_GND = 19;

// keypad pins
const uint8_t KEYPAD[9] = {2, 3, 5, 6, 9, 10, 11, 12, 13};

// pin translations for keypad rows
const uint8_t row[4] = {KEYPAD[1], KEYPAD[2], KEYPAD[8], KEYPAD[7]};

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
// debounce for hook and keypad columns
Bounce hook = Bounce();
Bounce column[4];

// timers
Metro read_keys(20); // keypad row switch timer

// variables -----------------------------------------------
// keypad-input
String key_input = "";
String last_input = "";

// row counter
uint8_t row_counter = 0;

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
    column[i].interval(10);
  }
}

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
      column[i].update();
      if (column[i].fell())
      {
        if (i != 3) key_input += keys[row_counter][i]; // numbers
        else if (row_counter == 2 && last_input.length() > 0) key_input = last_input; // redail key
        else if (row_counter == 3 && key_input.length() > 0) key_input.remove(key_input.length() - 1); // R (remove) key
      }
    }
    // set rows
    for (uint8_t i = 0; i < 4; i++)
    {
      if (i == row_counter) digitalWrite(row[i], LOW);
      else digitalWrite(row[i], HIGH);
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
}
