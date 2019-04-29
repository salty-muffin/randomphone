const uint8_t KEYPAD[9] = {2, 3, 5, 6, 9, 10, 11, 12, 13};

char keys[9][9] = {{'-', '-', '-', '-', '-', '-', '-', '-', '-'},
                   {'-', '-', '-', '1', '2', '3', 'X', '-', '-'},
                   {'-', '-', '-', '4', '5', '6', 'A', '-', '-'},
                   {'-', '1', '4', '-', '-', '-', '-', '*', '7'},
                   {'-', '2', '5', '-', '-', '-', '-', '0', '8'},
                   {'-', '3', '6', '-', '-', '-', '-', '#', '-'},
                   {'-', 'X', 'A', '-', '-', '-', '-', 'R', 'D'},
                   {'-', '-', '-', '*', '0', '#', 'R', '-', '-'},
                   {'-', '-', '-', '7', '8', '9', 'D', '-', '-'}};

uint8_t x = 0;
uint8_t y = 0;

void setup()
{
  Serial.begin(115200);

  delay(3000);

  Serial.println("press every botton: return the button that prints '1'");
}

void loop()
{  
  while(x < 9)
  {
    for (uint8_t i = 0; i < 9; i++)
    {
        pinMode(KEYPAD[i], OUTPUT);
        digitalWrite(KEYPAD[i], HIGH);
    }
    digitalWrite(KEYPAD[x], LOW);
    pinMode(KEYPAD[y], INPUT_PULLUP);

    Serial.println("(" + String(x) + ", " + String(y)+ ")");

    delay(500);

    while (!Serial.available())
    {
      String out = "0, ";
      if (!digitalRead(KEYPAD[y]))
      {
        out += "1";
      }
      Serial.println(out);
      delay(100);
    }
    
    keys[x][y] = Serial.readString()[0];
    
    y++;
    if (!(y < 9))
    {
      y = 0;
      x++;
    }
  }

  Serial.println("{{'" + String(keys[0][0]) + "', '" + String(keys[0][1]) + "', '" + String(keys[0][2]) + "', '" + String(keys[0][3]) + "', '" + String(keys[0][4]) + "', '" + String(keys[0][5]) + "', '" + String(keys[0][6]) + "', '" + String(keys[0][7]) + "', '" + String(keys[0][8]) + "'},\n"
                 + "{'" + String(keys[1][0]) + "', '" + String(keys[1][1]) + "', '" + String(keys[1][2]) + "', '" + String(keys[1][3]) + "', '" + String(keys[1][4]) + "', '" + String(keys[1][5]) + "', '" + String(keys[1][6]) + "', '" + String(keys[1][7]) + "', '" + String(keys[1][8]) + "'},\n"
                 + "{'" + String(keys[2][0]) + "', '" + String(keys[2][1]) + "', '" + String(keys[2][2]) + "', '" + String(keys[2][3]) + "', '" + String(keys[2][4]) + "', '" + String(keys[2][5]) + "', '" + String(keys[2][6]) + "', '" + String(keys[2][7]) + "', '" + String(keys[2][8]) + "'},\n"
                 + "{'" + String(keys[3][0]) + "', '" + String(keys[3][1]) + "', '" + String(keys[3][2]) + "', '" + String(keys[3][3]) + "', '" + String(keys[3][4]) + "', '" + String(keys[3][5]) + "', '" + String(keys[3][6]) + "', '" + String(keys[3][7]) + "', '" + String(keys[3][8]) + "'},\n"
                 + "{'" + String(keys[4][0]) + "', '" + String(keys[4][1]) + "', '" + String(keys[4][2]) + "', '" + String(keys[4][3]) + "', '" + String(keys[4][4]) + "', '" + String(keys[4][5]) + "', '" + String(keys[4][6]) + "', '" + String(keys[4][7]) + "', '" + String(keys[4][8]) + "'},\n"
                 + "{'" + String(keys[5][0]) + "', '" + String(keys[5][1]) + "', '" + String(keys[5][2]) + "', '" + String(keys[5][3]) + "', '" + String(keys[5][4]) + "', '" + String(keys[5][5]) + "', '" + String(keys[5][6]) + "', '" + String(keys[5][7]) + "', '" + String(keys[5][8]) + "'},\n"
                 + "{'" + String(keys[6][0]) + "', '" + String(keys[6][1]) + "', '" + String(keys[6][2]) + "', '" + String(keys[6][3]) + "', '" + String(keys[6][4]) + "', '" + String(keys[6][5]) + "', '" + String(keys[6][6]) + "', '" + String(keys[6][7]) + "', '" + String(keys[6][8]) + "'},\n"
                 + "{'" + String(keys[7][0]) + "', '" + String(keys[7][1]) + "', '" + String(keys[7][2]) + "', '" + String(keys[7][3]) + "', '" + String(keys[7][4]) + "', '" + String(keys[7][5]) + "', '" + String(keys[7][6]) + "', '" + String(keys[7][7]) + "', '" + String(keys[7][8]) + "'},\n"
                 + "{'" + String(keys[8][0]) + "', '" + String(keys[8][1]) + "', '" + String(keys[8][2]) + "', '" + String(keys[8][3]) + "', '" + String(keys[8][4]) + "', '" + String(keys[8][5]) + "', '" + String(keys[8][6]) + "', '" + String(keys[8][7]) + "', '" + String(keys[8][8]) + "'}};");

  while (true) delay(1000);
}
