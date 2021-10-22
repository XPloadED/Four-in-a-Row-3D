#include <WiFi.h>
#include <HTTPClient.h>

#include <Keypad.h>
#include <FastLED.h>

#define NUM_LEDS 128
#define LED_PIN 2
#define NUM_TOWER 16

#define NUM_PLAYER 2

//----------------------------------------------------------------------------------------
//const vars
//basic layout of the game
const byte ROWS = 4; // four rows
const byte COLS = 4; // four columns

//Pin numbers for case buttons
const int CASE_BUTTON_1 = 5;
const int CASE_BUTTON_2 = 4;
const int CASE_BUTTON_3 = 15;

//Pin numbers for button matrix
byte colPins[ROWS] = {33, 26, 27, 12};
byte rowPins[COLS] = {22, 21, 19, 18};


//----------------------------------------------------------------------------------------
//WiFi network:settings
const char* ssid = "esp";
const char* password = "connectESP4Row3D!";

//Server for exchange
String serverName = "https://ubicomp.net/sw/db1/var2db.php";

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000):
//unsigned long timerDelay = 600000;
// Set timer to 5 seconds (5000)
unsigned long timerDelay = 5000;


//----------------------------------------------------------------------------------------
//Game settings
//game status
// Layout of the gamestate array with 64 places: 1d representation of 3d playground
// e.g. T1 -> Tower 1 with places for tokens 4 to 7
//      T2 -> Tower 2 with places for tokens 8 to 11
//      ...
//
//y\x  |  0          1         2          3
//--------------------------------------------
//  0   | T0 (0-3)    |--> T4 (16-19)  |--> T8 (32-35)   |-> T12 (49-52)
//      |     |       |          |     |           |     |      |
//  1   | T1 (4-7)    |    T5 (20-23)  |    T9 (36-39)   |   T13 (53-56)
//      |     |       |          |     |           |     |      |
//  2   | T2 (8-11)   |    T6 (24-27)  |    T10 (40-43)  |   T14 (57-60)
//      |     |       |          |     |           |     |      |
//  3   | T3 (12-15)--|    T7 (28-31)--|    T11 (44-48)--|   T15 (61-64)

int gameState[NUM_TOWER * 4] = {};
int towerHeight[NUM_TOWER] = {};

int lastToken[3] = {};
int moveCounter = 0;

// array for winning row
//int winningTokens[4][3] = {};
int winningTokens[NUM_TOWER * 4] = {};

//define starting state
String state = "setupGameState";

//----------------------------------------------------------------------------------------
//player settings
//color of players 0 and 1
CHSV players[NUM_PLAYER] = {CHSV(150, 255, 255), CHSV(255, 255, 255)};
// var for alternating player

const int myPlayer = 0; // change for second player
int playerToken = -1;



//----------------------------------------------------------------------------------------
// Map the buttons to an array for the Keymap instance
char hexaKeys[ROWS][COLS] = {
  {'0', '1', '2', '3'},
  {'4', '5', '6', '7'},
  {'8', '9', 'A', 'B'},
  {'C', 'D', 'E', 'F'}
};
// Initialise the Keypad
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

//----------------------------------------------------------------------------------------
//CASE BUTTONS init

int lastStateCB1 = HIGH; // the previous state from the input pin CASEBUTTON1
int currentStateCB1;    // the current reading from the input pin

int lastStateCB2 = HIGH; // the previous state from the input pin CASEBUTTON1
int currentStateCB2;    // the current reading from the input pin

int lastStateCB3 = HIGH; // the previous state from the input pin CASEBUTTON1
int currentStateCB3;    // the current reading from the input pin


//----------------------------------------------------------------------------------------
//Led stipe init
uint8_t hue = 0;
//init length of the LED stripe
CRGB leds[NUM_LEDS];


//----------------------------------------------------------------------------------------
// initial startup rutine
void setup() {
  Serial.begin(9600);   // Initialise the serial monitor
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(50);

  pinMode(CASE_BUTTON_1, INPUT_PULLUP);
  pinMode(CASE_BUTTON_2, INPUT_PULLUP);
  pinMode(CASE_BUTTON_3, INPUT_PULLUP);


  //Setup WiFi
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");
}

//----------------------------------------------------------------------------------------
//setup game settings
void setupGame() {
  for (int i = 0; i < NUM_TOWER; i++) {
    towerHeight[i] = 0; //height per tower at startup is 0
  }
  for (int j = 0; j < NUM_TOWER * 4; j++) {
    gameState[j] = -1;  //-1 -> means no player, 0 -> player 1, 1 -> player 2
    winningTokens[j] = 0; // reset winningTokens array
  }

  playerToken = 0;
  moveCounter = 0;
  resetAllLeds();
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
// UTILITY ROUTINE
//-------------------------------------------------------------------------------------------------------------------
//
//  Layout how the LEDs are connected together
//  Total LED of the playboard: 128
//y\x  |  0          1         2          3
//--------------------------------------------
//  0   | (0-7)     (56-63)---(64-71)   (120-127)
//      |  |          |         |          |
//  1   | (8-15)    (48-55)   (72-79)   (112-119)
//      |  |          |         |          |
//  2   | (16-23)   (40-47)   (80-87)   (104-111)
//      |  |          |         |          |
//  3   | (24-31)---(32-39)   (88-95)---(96-103)
//
//Calculate the number of the first led of the tower (x,y) depending on the towers x (0 <= x <= 3) and y(0 <= y <= 3) coordinate
int calcLedNumbersOfTowerXY(uint8_t x, uint8_t y) {
  if (x % 2 == 0) {             // is x even?
    return ((x * 32) + (y * 8)); // even x column
  } else {
    return ((x * 32 + 32) - ((y + 1) * 8)); // odd x column
  }
}

void calcTowerXYHFromLedNum(uint8_t LedNum, uint8_t* x, uint8_t* y, uint8_t* h) {
  *h = LedNum % 4;
  *x = (int)((int)(LedNum / 4) / 4);
  *y = ((int)(LedNum / 4) % 4);

}

int calcTowerID(uint8_t x, uint8_t y) {
  return (x * 4 + y);
}

String convertArrayToString(int array[], int arraySize) {
  String output = "";

  for (int j = 0; j < arraySize; j++) {
    //Serial.print(array[j]);
    output = output + String(array[j]);
    if (j < arraySize - 1) {
      output = output + "_";
    }
  }
  //Serial.println();
  return output;
}

//int convertStringToArray(char *httpString, int arraySize, int *point) {
//  int i = 0;
//  char *token;
//
//  const char *delimiter = "_";
//
//  int array[arraySize];
//
//  token = strtok(httpString, delimiter);
//  while (token != NULL) {
//    Serial.println(token);
//    array[i] = (int)token;
//    token  = strtok(NULL, delimiter);
//    i = +1;
//  }
//
//  return 0;
//}

int setLastToken(uint8_t x, uint8_t y, uint8_t h) {
  lastToken[0] = x;
  lastToken[1] = y;
  lastToken[2] = h;

  return 0;
}

void getLastToken(uint8_t* x, uint8_t* y, uint8_t* h){
  *x = lastToken[0];
  *y = lastToken[1];
  *h = lastToken[2];
}

String sendHttpGet(String url) {
  String payload;

  if (WiFi.status() == WL_CONNECTED) {

    HTTPClient http;

    String serverPath = serverName + url;
    //String serverPath = serverName + "?varName=4row_test";

    // Your Domain name with URL path or IP address with path
    http.begin(serverPath.c_str());

    // Send HTTP GET request
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      payload = http.getString();
      //Serial.println(payload);
    }
    else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();
  }
  else {
    Serial.println("WiFi Disconnected");
  }
  
  return payload;
}

void setNetVar(String varName, String varValue) {
  String url = "?varName=" + varName + "&varValue=" + varValue;
  sendHttpGet(url);
}
String getNetVar(String varName) {
  String url = "?varName=" + varName;
  String response = sendHttpGet(url);

  return response;
}


//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
// LED CONTROL
//-------------------------------------------------------------------------------------------------------------------
//
// set the color of a led pair at the tower (x,y) with (0 <= x <= 3) and y(0 <= y <= 3) in the height h ((0 <= h <= 3) (from bottom) in a CHSV color depending on player (0 or 1)
void setLedPair(uint8_t x, uint8_t y, uint8_t h, CHSV color) {
  int firstLed = calcLedNumbersOfTowerXY(x, y); // first led number of the tower

  leds[firstLed + h] = color; //  set color for led at tower (x,y) and height h
  leds[firstLed + 7 - h] = color; // set color for mirrored led at tower (x,y) and height h
  FastLED.show();
}

//-------------------------------------------------------------------------------------------------------------------
// reset the color of a led pair at the tower (x,y) with (0 <= x <= 3) and y(0 <= y <= 3) in the height h ((0 <= h <= 3) (from bottom)
void resetLedPair(uint8_t x, uint8_t y, uint8_t h) {
  int firstLed = calcLedNumbersOfTowerXY(x, y); // first led number of the tower

  leds[firstLed + h] = CRGB::Black; // reset color for led at tower (x,y) and height h
  leds[firstLed + 7 - h] = CRGB::Black; // reset color for mirrored led at tower (x,y) and height h
  FastLED.show();
}

//-------------------------------------------------------------------------------------------------------------------
// reset the color of all leds
void resetAllLeds() {
  for (int i = 0; i < 128; i++) {
    leds[i] = CRGB::Black; // reset color of all led
  }
  FastLED.show();
}

//-------------------------------------------------------------------------------------------------------------------
void animateGameToken(uint8_t x, uint8_t y, uint8_t player) {
  int towerID = calcTowerID(x, y); //towerID for towerHeight array required
  int time = 100;

  if (towerHeight[towerID] < 4) {
    int tokenHeight = 3; // starting height -> stone is falling down
    setLedPair(x, y, tokenHeight, players[player]);
    while (tokenHeight > towerHeight[towerID]) {
      delay(time -= 20);
      resetLedPair(x, y, tokenHeight);
      tokenHeight -= 1;
      setLedPair(x, y, tokenHeight, players[player]);
    }

    towerHeight[towerID] += 1;
    gameState[(towerID * 4 + towerHeight[towerID] - 1)] = player; // 1d representation of game for storing the game status

    setLastToken(x, y, towerHeight[towerID]);

  } else {
    Serial.println("Full! no more tokens!");
  }
}

void removeGameToken(uint8_t x, uint8_t y, uint8_t player) {
  int towerID = calcTowerID(x, y); //towerID for towerHeight array required

  if (gameState[(towerID * 4 + towerHeight[towerID] - 1)] == player) {
    resetLedPair(x, y, towerHeight[towerID]); // reset LED
    towerHeight[towerID] -= 1; // reduce height of tower with towerID
    gameState[(towerID * 4 + towerHeight[towerID] - 1)] = -1;
  } else {
    Serial.println("Can't remove token! Wrong color!");
  }


}

//-------------------------------------------------------------------------------------------------------------------
// plays the stone drop down animation until it reaches the bottom spot
void playStoneAnimation(int towerID, int stackHeight, bool startup, CHSV Player) {
  // find led spot of the top of the tower and initialice the mirror stone
  int runindex = (towerID * 8) + 3;
  int mirrorstone = runindex + 1;
  int deltaDuration = 100;

  if (stackHeight < 3) {
    //Display the stone for deltaduration
    leds[runindex] = Player;
    leds[mirrorstone] = Player;
    FastLED.show();
    delay(deltaDuration);
    if (not startup) {
      leds[runindex] = CRGB::Black;
      leds[mirrorstone] = CRGB::Black;
      FastLED.show();
    }

  }

  // while the tower has empty spaces, repeat until it reaches the state bottom
  while (stackHeight < 3) {
    runindex = runindex - 1;
    mirrorstone = mirrorstone + 1;
    leds[runindex] = Player;
    leds[mirrorstone] = Player;
    FastLED.show();
    delay(deltaDuration = deltaDuration - 20);

    stackHeight++;

    if (not startup) {
      leds[runindex] = CRGB::Black;
      leds[mirrorstone] = CRGB::Black;
      FastLED.show();
      if (stackHeight == 3) {
        leds[runindex] = Player;
        leds[mirrorstone] = Player;
        FastLED.show();
      }
    }
  }
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// WINNING DETECTION
//-------------------------------------------------------------------------------------------------------------------
//
// check for a winner function
bool checkForWinner(uint8_t x, uint8_t y, uint8_t h, int player) {
  //all possible neighbor fields as vector
  int vector[13][3] =   {{1, 0, 0}, //{-1,0,0},
    {0, 1, 0}, //{0,-1,0},
    {0, 0, 1}, //{0,0,-1},
    {1, 1, 0}, //{-1,-1,0},
    {1, 0, 1}, //{-1,0,-1},
    {0, 1, 1}, //{0,-1,-1},
    {1, -1, 0}, // {-1,1,0},
    {1, 0, -1}, // {-1,0,1},
    {0, 1, -1}, // {0,-1,1},
    {1, 1, -1}, // {-1,1,-1},
    {1, -1, -1}, // {-1,1,1},
    {1, -1, 1}, // {1,-1,1},
    {1, 1, 1}
  }; //,{-1,-1,-1}};

  bool winnerFound = false;
  int i = 0;
  while (!winnerFound && i < 13) {
    int direction = countToken(x + vector[i][0], y + vector[i][1], h + vector[i][2], vector[i][0], vector[i][1], vector[i][2], player) + countToken(x - vector[i][0], y - vector[i][1], h - vector[i][2], -vector[i][0], -vector[i][1], - vector[i][2], player);

    if (direction >= 3) {
      int fieldNum = ((x * 4 + y) * 4 + h);
      winningTokens[fieldNum] = 1;
      winnerFound = true;
    } else {
      i++;
    }
  }

  if (winnerFound) {
    return true;
  } else {
    return false;
  }
}

int countToken(uint8_t x, uint8_t y, uint8_t h, int delta_x, int delta_y, int delta_h, int player) { //, int** tokenRow, int count){
  if ((x >= 0 && x < 4) && (y >= 0 && y < 4) && (h >= 0 && h < 4)) { //if token is in the bounderies of the game
    int fieldNum = ((x * 4 + y) * 4 + h);
    if (gameState[fieldNum] == player) {
      winningTokens[fieldNum] = 1;
      return countToken(x + delta_x, y + delta_y, h + delta_h, delta_x, delta_y, delta_h, player) + 1;
    } else {
      for (int j = 0; j < NUM_TOWER * 4; j++) {
        winningTokens[j] = 0;       // reset winningTokens array
      }
      return 0; // gameState[newField] and player are not identical
    }
  } else {
    return 0; // return 0 if token is out of bounderies
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GAME LOGIC / GAME ROUTINE
//-------------------------------------------------------------------------------------------------------------------
//

void loop() {
  //StartUp routine
  if (state == "startup") {

    for (int towerID = 0; towerID < 16; towerID ++) {
      playStoneAnimation(towerID, random(0, 3), true, CHSV(random8(), 255, 255));
    }
    for (int towerID = 15; towerID >= 0; towerID --) {
      playStoneAnimation(random(0, 15), random(0, 3), false, CHSV(random8(), 255, 255));
    }
    for (int towerID = 0; towerID < 16; towerID ++) {
      playStoneAnimation(towerID, random(0, 3), true, CHSV(random8(), 255, 255));
    }

    // End of the startup routine, later switch to gamemode [singleplayer/multiplayer/atmospheric lamp]
    // For now leads into the tower select state, in which the player can select a tower through the button matrix
    resetAllLeds();
    state = "setupGameState";
  }

  if (state == "setupGameState") {
    setupGame();

    // read the state of the switch/button: -> select button
    currentStateCB1 = digitalRead(CASE_BUTTON_1);
    if (lastStateCB1 == LOW && currentStateCB1 == HIGH) {
      state = "syncState";
    }
    // save the the last state
    lastStateCB1 = currentStateCB1;

    // read the state of the switch/button:
    currentStateCB3 = digitalRead(CASE_BUTTON_3);
    if (lastStateCB3 == LOW && currentStateCB3 == HIGH) {

    }
    // save the the last state
    lastStateCB3 = currentStateCB3;
  }

  if (state == "pickTowerState") {
    //Button matrix
    char button = customKeypad.getKey();
    if (button) {
      switch (button) {
        case '0':
          animateGameToken(0, 0, playerToken);
          break;
        case '1':
          animateGameToken(0, 1, playerToken);
          break;
        case '2':
          animateGameToken(0, 2, playerToken);
          break;
        case '3':
          animateGameToken(0, 3, playerToken);
          break;

        case '4':
          animateGameToken(1, 0, playerToken);
          break;
        case '5':
          animateGameToken(1, 1, playerToken);
          break;
        case '6':
          animateGameToken(1, 2, playerToken);
          break;
        case '7':
          animateGameToken(1, 3, playerToken);
          break;

        case '8':
          animateGameToken(2, 0, playerToken);
          break;
        case '9':
          animateGameToken(2, 1, playerToken);
          break;
        case 'A':
          animateGameToken(2, 2, playerToken);
          break;
        case 'B':
          animateGameToken(2, 3, playerToken);
          break;

        case 'C':
          animateGameToken(3, 0, playerToken);
          break;
        case 'D':
          animateGameToken(3, 1, playerToken);
          break;
        case 'E':
          animateGameToken(3, 2, playerToken);
          break;
        case 'F':
          animateGameToken(3, 3, playerToken);
          break;
      }
    }

    // read the state of the switch/button: -> select button
    currentStateCB1 = digitalRead(CASE_BUTTON_1);
    if (lastStateCB1 == LOW && currentStateCB1 == HIGH) {
      state = "check4WinState";
    }
    // save the the last state
    lastStateCB1 = currentStateCB1;

    // read the state of the switch/button: -> reset button
    currentStateCB3 = digitalRead(CASE_BUTTON_3);
    if (lastStateCB3 == LOW && currentStateCB3 == HIGH) {
      state = "setupGameState"; //reset game button
    }
    // save the the last state
    lastStateCB3 = currentStateCB3;

  }

  if (state == "syncState") {
    Serial.println("moveCounter: " + String(moveCounter));
   setNetVar("4row_test",String(moveCounter));
   Serial.println(getNetVar("4row_test"));
   moveCounter +=  1;
   
    
  }


  if (state == "check4WinState") {
    int x = lastToken[0];
    int y = lastToken[1];

    if (checkForWinner(x, y, (towerHeight[calcTowerID(x, y)] - 1), playerToken)  ) { // check if there is already a winner of the game
      state = "winnerState"; //if there is a winner change the state to winner
    }else{
      state = "pickTowerState";
    }

  }

  if (state == "winnerState") {
    resetAllLeds();
    for (int i = 0; i < 64; i++) {
      uint8_t x, y, h = 0;
      if (winningTokens[i] == 1) {
        calcTowerXYHFromLedNum(i, &x, &y, &h);
        setLedPair(x, y, h, players[gameState[i]]);
      }
    }

    delay(500);
    resetAllLeds();

  }

}
