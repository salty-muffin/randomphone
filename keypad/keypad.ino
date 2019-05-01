// papayapeter
// 2019

// libraries -----------------------------------------------
#include <Metro.h>
#include <KeyMatrix.h>

// pins ----------------------------------------------------
// hook pins
const uint8_t row_pins[4]    = {3, 5, 13, 12};
const uint8_t column_pins[4] = {6, 9, 10, 11};

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
KeyMatrix keypad((char *) keys, 4, 4, row_pins, column_pins);

// variables -----------------------------------------------
// keypad-input
String key_input = "";
String last_input = "";

// functions -----------------------------------------------

void setup()
{
  // setup serial (DEBUG)
  Serial.begin(115200);
}

void loop()
{
  char key = keypad.getKey();
  // poll events from the keypad
  if (key != 0) Serial.println(key);
}
