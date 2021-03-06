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

//Pin number for Potentiometer
const int POT = 32;
int potVal = 0;

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
// Set timer to 5 seco  nds (5000)
unsigned long timerDelay = 5000;


//----------------------------------------------------------------------------------------
//State configuration
int moveCounter = 0;
bool gameWinner = false;

//define starting state
String newState = "startup";

int  gameMode =  0;
int color = 0;


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

bool placedToken = false;

int lastToken[3] = {};
int playerLastToken = -1;

// array for winning row
int winningTokens[NUM_TOWER * 4] = {};

//----------------------------------------------------------------------------------------
//player settings
//color of players 0 and 1
CHSV players[NUM_PLAYER] = {CHSV(150, 255, 255), CHSV(255, 255, 255)};
int brightness = 150;
int oldbrightness = 150;

unsigned long currentTime;
unsigned long previousTime;
// var for alternating player

int myPlayer = 0; // change for second player
int nextPlayer = -1;


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
  FastLED.setBrightness(brightness);

  pinMode(CASE_BUTTON_1, INPUT_PULLUP);
  pinMode(CASE_BUTTON_2, INPUT_PULLUP);
  pinMode(CASE_BUTTON_3, INPUT_PULLUP);

  //fix flashing LEDs on startup after powerloss
  FastLED.clear();
  FastLED.show();

  setupGame();
  currentTime = millis();
  previousTime = 0;

  xTaskCreate(
    potAction,
    "parallel potentiometer interaction",
    1000,
    NULL,
    1,
    NULL
    );

  connectWifi();
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
  resetAllLeds();
  gameWinner = false;
}

//----------------------------------------------------------------------------------------
//connect to wifi, true if successful
bool connectWifi(){
    //Setup WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("Connecting");

  //local variables for connect animation
  uint8_t locx = 0;
  uint8_t locy = 0;
  uint8_t loch = 0;
  uint8_t cyclecounter = 0;
  uint8_t hue = 0;
  bool direction = true;
  currentTime = millis();
  previousTime = millis();
  while (WiFi.status() != WL_CONNECTED && currentTime - previousTime < 10000) {
    //WIFI connect animation
    setLedPair(locx, locy, loch, CHSV(hue++, 255, 255));
    delay(40);
    resetLedPair(locx, locy, loch);

    if (locx == 0 && locy > 0) {
      locy -= 1;
    }
    if (locx > 0 && locy == 3) {
      locx -= 1;
    }
    if (locx == 3 && locy < 3) {
      locy += 1;
    }
    //edge case 0,0 being skipped
    if (locx < 3 && locy == 0) {
      locx += 1;
    }

    if (cyclecounter == 12) {
      if (direction) {
        loch += 1;
        cyclecounter = 0;
      }
      if (not direction) {
        loch -= 1;
        cyclecounter = 0;
      }
      if (loch == 3 || loch == 0) {
        direction = not direction;
        cyclecounter = 0;
      }
    }
    cyclecounter += 1;
    currentTime = millis();
  }
  currentTime = millis();
  previousTime = millis();

  if(WiFi.status() != WL_CONNECTED){
    Serial.println("No Wifi Connection established, continuing ...");
    displayError();
    return false;
    } else{
        Serial.println("");
        Serial.print("Connected to WiFi network with IP Address: ");
        Serial.println(WiFi.localIP());

        Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");
        displaySuccess();
        return true;
      }

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

void getLastToken(uint8_t* x, uint8_t* y, uint8_t* h) {
  *x = lastToken[0];
  *y = lastToken[1];
  *h = lastToken[2];
}

String sendHttpGet(String url) {
  String payload;

  if (WiFi.status() == WL_CONNECTED) {

    HTTPClient http;

    Serial.println("URL: " + url);
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
      Serial.println(payload);
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
  CHSV selectplayer = CHSV(95, 255, 255);

  if (towerHeight[towerID] < 4) {
    int tokenHeight = 3; // starting height -> stone is falling down
    if (player == myPlayer) {
      setLedPair(x, y, tokenHeight, selectplayer);
    } else {
      setLedPair(x, y, tokenHeight, players[player]);
    }

    while (tokenHeight > towerHeight[towerID]) {
      delay(time -= 20);
      resetLedPair(x, y, tokenHeight);
      tokenHeight -= 1;
      if (player == myPlayer) {
        setLedPair(x, y, tokenHeight, selectplayer);
      } else {
        setLedPair(x, y, tokenHeight, players[player]);
      }
    }

    towerHeight[towerID] += 1;
    gameState[(towerID * 4 + towerHeight[towerID] - 1)] = player; // 1d representation of game for storing the game status

    Serial.println("Set token: x " + String(x) + ",y  " + String(y) + ",P " + String(player));
    setLastToken(x, y, towerHeight[towerID]);
    playerLastToken = player;
    placedToken = true;

    Serial.println("GameState after Token: " + convertArrayToString(gameState, 64));
    Serial.println("TokenHeight after Token: " + convertArrayToString(towerHeight, 16));
  } else {
    Serial.println("Full! no more tokens!");
  }
}

void removeGameToken(uint8_t x, uint8_t y, uint8_t player) {
  Serial.println(convertArrayToString(towerHeight, 16));
  Serial.println("Remove token: x " + String(x) + ",y  " + String(y) + ",P " + String(player));
  int towerID = calcTowerID(x, y); //towerID for towerHeight array required

  if (gameState[(towerID * 4 + towerHeight[towerID] - 1)] == player) {
    resetLedPair(x, y, towerHeight[towerID] - 1); // reset LED
    gameState[(towerID * 4 + towerHeight[towerID] - 1)] = -1;
    towerHeight[towerID] -= 1; // reduce height of tower with towerID

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

void displayError(){
  for(int i = 0; i < 128; i++){
    leds[i] = CRGB::Red;
    }
    FastLED.show();
    delay(1000);
    
  }

void displaySuccess(){
  for(int i = 0; i < 128; i++){
    leds[i] = CRGB::Green;
    }
    FastLED.show();
    delay(1000);  
  }

void potAction(void * parameters){
    for( ;; ){
      potVal = analogRead(POT);
      brightness = map(potVal, 0, 4096, 0, 255);
      //care for fluctuations and slight turns
      if(brightness - oldbrightness > 3 || brightness - oldbrightness < -3){
        //adjusts max and min values to fluctuation checks
        if(brightness < 5){
          brightness = 0;
          }
        if(brightness > 250){
          brightness = 255;
          }
        FastLED.setBrightness(brightness);
        FastLED.show();
        oldbrightness =  brightness;
        Serial.print("Brightness adjusted: ");        
        Serial.println(brightness);
        }
      vTaskDelay(50);
      }
  
  }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GAME LOGIC / GAME ROUTINE
//-------------------------------------------------------------------------------------------------------------------
//

void loop() {
  //StartUp routine
  if (newState == "startup") {
    Serial.println("State: startup");
    for (int towerID = 0; towerID < 16; towerID ++) {
      playStoneAnimation(towerID, 0, true, CHSV(random8(), 255, 255));
    }
    /*for (int towerID = 15; towerID >= 0; towerID --) {
      playStoneAnimation(towerID, 0, false, CHSV(random8(), 255, 255));
      }
      for (int towerID = 0; towerID < 16; towerID ++) {
      playStoneAnimation(towerID, 0, true, CHSV(random8(), 255, 255));
      }*/

    // End of the startup routine, later switch to gamemode [singleplayer/multiplayer/atmospheric lamp]
    // For now leads into the tower select state, in which the player can select a tower through the button matrix
    resetAllLeds();
    Serial.println("State: exited startup; New State: setupGameState");
    for (int i = 0; i < 4; i++) {
      for (int b = 0; b < 4; b++) {
        setLedPair(i, b, 3, CHSV(50 + 50 * gameMode, 255, 255));
      }
    }
    newState = "setupGameState";
  }

  if (newState == "setupGameState") {

    // read the state of the switch/button: -> select button
    currentStateCB1 = digitalRead(CASE_BUTTON_1);
    if (lastStateCB1 == LOW && currentStateCB1 == HIGH) {

      setupGame();
      if (gameMode == 0) {
        Serial.println("State: setupGameState -> Button 1 gedr??ckt!");
        if(WiFi.status() != WL_CONNECTED){
          //Trying once more
            WiFi.disconnect();
            WiFi.begin(ssid, password);
        }
        if(WiFi.status() != WL_CONNECTED){
            Serial.println("Slected Multiplayer but Wifi is unconnected! Restarting");
            displayError();
            ESP.restart();
        }
        if (getNetVar("4row_moveCount").toInt() != 0) {
          resetAllLeds();
          //Set moveCounter to 0 and send to server
          moveCounter = 0;
          setNetVar("4row_moveCount", String(moveCounter));

          //Set playerToken -> change player
          nextPlayer = myPlayer; // Player selection
          setNetVar("4row_nextPlayer", String(nextPlayer));

          //reset lastToken
          setNetVar("4row_lastToken_x", String(-1));
          setNetVar("4row_lastToken_y", String(-1));
          setNetVar("4row_lastToken_h", String(-1));
          setNetVar("4row_player_lastToken", String(-1));
          Serial.println("State: exited setupGameState; New State: PickTowerState");
          newState = "pickTowerState";
        } else {
          moveCounter = getNetVar("4row_moveCount").toInt();
          nextPlayer = myPlayer;
          Serial.println("State: exited setupGameState; New State: syncState");
          newState = "syncState";

        }
      }
      if (gameMode == 1) {
        moveCounter = 0;
        setLastToken(-1, -1, 0);
        playerLastToken = -1;
        nextPlayer = myPlayer;

        Serial.println("State: exited setupGameState; New State: PickTowerState");
        newState = "pickTowerState";
      }
      if (gameMode == 2) {
        Serial.println("State: exited setupGameState; New State: light");
        newState = "light";
      }
      resetAllLeds();
    }
    // save the the last state
    lastStateCB1 = currentStateCB1;

    // read the state of the switch/button: -> select mode => Multi/Single mode
    currentStateCB2 = digitalRead(CASE_BUTTON_2);
    if (lastStateCB2 == LOW && currentStateCB2 == HIGH) {
      Serial.println("State: setupGameState -> Button 2 gedr??ckt!");
      Serial.println("Mode: switched" + String(gameMode));
      gameMode = (gameMode + 1) % 3;

      resetAllLeds();
      for (int i = 0; i < 4; i++) {
        for (int b = 0; b < 4; b++) {
          setLedPair(i, b, 3, CHSV(50 + 50 * gameMode, 255, 255));
        }
      }

    }
    // save the the last state
    lastStateCB2 = currentStateCB2;

    // read the state of the switch/button:
    currentStateCB3 = digitalRead(CASE_BUTTON_3);
    if (lastStateCB3 == LOW && currentStateCB3 == HIGH) {
      resetAllLeds();
      Serial.println("State: setupGameState -> Button 3 gedr??ckt!");
      moveCounter = 0;
      setNetVar("4row_moveCount", String(moveCounter));

      //Set playerToken -> change player
      nextPlayer = myPlayer; // Player selection
      setNetVar("4row_nextPlayer", String(nextPlayer));

      //reset lastToken
      setNetVar("4row_lastToken_x", String(-1));
      setNetVar("4row_lastToken_y", String(-1));
      setNetVar("4row_lastToken_h", String(-1));
      setNetVar("4row_player_lastToken", String(-1));

      Serial.println("State: exited setupGameState; New State: PickTowerState");
      newState = "pickTowerState";

    }
    // save the the last state
    lastStateCB3 = currentStateCB3;
  }

  if (newState == "pickTowerState") {
    //Serial.println("State: pickTowerState");
    //Button matrix
    char button = customKeypad.getKey();
    if (button) {
      switch (button) {
        case '0':
          if (placedToken) {
            removeGameToken(lastToken[0], lastToken[1], myPlayer);
          }
          animateGameToken(0, 0, myPlayer);
          break;
        case '1':
          if (placedToken) {
            removeGameToken(lastToken[0], lastToken[1], myPlayer);
          }
          animateGameToken(0, 1, myPlayer);
          break;
        case '2':
          if (placedToken) {
            removeGameToken(lastToken[0], lastToken[1], myPlayer);
          }
          animateGameToken(0, 2, myPlayer);
          break;
        case '3':
          if (placedToken) {
            removeGameToken(lastToken[0], lastToken[1], myPlayer);
          }
          animateGameToken(0, 3, myPlayer);
          break;

        case '4':
          if (placedToken) {
            removeGameToken(lastToken[0], lastToken[1], myPlayer);
          }
          animateGameToken(1, 0, myPlayer);
          break;
        case '5':
          if (placedToken) {
            removeGameToken(lastToken[0], lastToken[1], myPlayer);
          }
          animateGameToken(1, 1, myPlayer);
          break;
        case '6':
          if (placedToken) {
            removeGameToken(lastToken[0], lastToken[1], myPlayer);
          }
          animateGameToken(1, 2, myPlayer);
          break;
        case '7':
          if (placedToken) {
            removeGameToken(lastToken[0], lastToken[1], myPlayer);
          }
          animateGameToken(1, 3, myPlayer);
          break;

        case '8':
          if (placedToken) {
            removeGameToken(lastToken[0], lastToken[1], myPlayer);
          }
          animateGameToken(2, 0, myPlayer);
          break;
        case '9':
          if (placedToken) {
            removeGameToken(lastToken[0], lastToken[1], myPlayer);
          }
          animateGameToken(2, 1, myPlayer);
          break;
        case 'A':
          if (placedToken) {
            removeGameToken(lastToken[0], lastToken[1], myPlayer);
          }
          animateGameToken(2, 2, myPlayer);
          break;
        case 'B':
          if (placedToken) {
            removeGameToken(lastToken[0], lastToken[1], myPlayer);
          }
          animateGameToken(2, 3, myPlayer);
          break;

        case 'C':
          if (placedToken) {
            removeGameToken(lastToken[0], lastToken[1], myPlayer);
          }
          animateGameToken(3, 0, myPlayer);
          break;
        case 'D':
          if (placedToken) {
            removeGameToken(lastToken[0], lastToken[1], myPlayer);
          }
          animateGameToken(3, 1, myPlayer);
          break;
        case 'E':
          if (placedToken) {
            removeGameToken(lastToken[0], lastToken[1], myPlayer);
          }
          animateGameToken(3, 2, myPlayer);
          break;
        case 'F':
          if (placedToken) {
            removeGameToken(lastToken[0], lastToken[1], myPlayer);
          }
          animateGameToken(3, 3, myPlayer);
          break;
      }
    }

    // read the state of the switch/button: -> select button
    currentStateCB1 = digitalRead(CASE_BUTTON_1);
    if (lastStateCB1 == LOW && currentStateCB1 == HIGH) {


      uint8_t lastTokenX, lastTokenY, lastTokenH = -1;
      getLastToken(&lastTokenX, &lastTokenY, &lastTokenH);

      if (gameState[calcTowerID(lastTokenX, lastTokenY) * 4 + lastTokenH - 1] == myPlayer) {
        setLedPair(lastTokenX, lastTokenY, lastTokenH - 1, players[myPlayer]);

        placedToken = false; // reset placedToken
        nextPlayer = ((myPlayer + 1) % 2); //Set nextPlayer to the new player
        Serial.println("State: exited pickTowerState; New State: check4WinState");
        newState = "check4WinState";
      }
    }
    // save the the last state
    lastStateCB1 = currentStateCB1;

  }

  if (newState == "syncState") {
    if (nextPlayer == myPlayer) {

      for (int i = 0; i < 4; i++) {
        for (int b = 0; b < 4; b++) {
          setLedPair(i, b, 3, CHSV(95, 255, 255));
          delay(50);
          resetLedPair(i, b, 3);
          setLedPair(i, b, 3, players[gameState[calcTowerID(i, b) * 4  + 3]]);
        }
      }


      if (getNetVar("4row_nextPlayer").toInt() == myPlayer) {
        Serial.println("SyncState: Entscheidung n??chster Spieler: *lokaler* / entfernter ");
        int lastTokenX = getNetVar("4row_lastToken_x").toInt();
        int lastTokenY = getNetVar("4row_lastToken_y").toInt();
        int lastTokenH = getNetVar("4row_lastToken_h").toInt() - 1;
        int playerLastTokenS = getNetVar("4row_player_lastToken").toInt();

        if ((lastTokenX >= 0 && lastTokenX < 4) && (lastTokenY >= 0 && lastTokenY < 4) && (lastTokenH >= 0 && lastTokenH < 4) && (getNetVar("4row_moveCount").toInt() != moveCounter)) {
          Serial.println("SyncState: Entscheidung n??chster Spieler: *lokaler* / entfernter: Animation gegnerischer Spieler");
          animateGameToken(lastTokenX, lastTokenY, playerLastTokenS);
          moveCounter = getNetVar("4row_moveCount").toInt();

          Serial.println("change state: syncState -> check4winState");
          newState = "check4WinState";
        }

      }
    }

    if (nextPlayer == ((myPlayer + 1) % 2)) {
      Serial.println("SyncState: Entscheidung n??chster Spieler: lokaler / *entfernter*: " + String(nextPlayer));
      setNetVar("4row_lastToken_x", String(lastToken[0]));
      setNetVar("4row_lastToken_y", String(lastToken[1]));
      setNetVar("4row_lastToken_h", String(lastToken[2]));
      setNetVar("4row_player_lastToken", String(myPlayer));
      moveCounter += 1;

      setNetVar("4row_moveCount", String(moveCounter));
      setNetVar("4row_nextPlayer", String(nextPlayer));

      Serial.println("SyncState: gameWinner: " + String(gameWinner));
      if (gameWinner == 1) {  // exit condition if winner is myPlayer -> dont stay in syncState
        newState = "check4WinState";
        Serial.println("change state: syncState (winner found) -> check4WinState: " + newState);
      }

      nextPlayer = myPlayer;
    }

    Serial.println("State: syncState -> Simulation n??chster Spieler EIN");
    delay(500);

    // read the state of the switch/button: -> reset button
    currentStateCB3 = digitalRead(CASE_BUTTON_3);
    if (lastStateCB3 == LOW && currentStateCB3 == HIGH) {
      Serial.println("State: syncState -> Simulation n??chster Spieler AUS");
      setNetVar("4row_lastToken_x", String(lastToken[0]));
      if (lastToken[0] == -1) {
        setLastToken(0, 3, 1);
      }

      setNetVar("4row_lastToken_x", String(0));
      setNetVar("4row_lastToken_y", String(0));
      setNetVar("4row_lastToken_h", String(towerHeight[0] + 1));
      setNetVar("4row_player_lastToken", String(myPlayer + 1));

      setNetVar("4row_moveCount", String(moveCounter + 1));
      setNetVar("4row_nextPlayer", String(myPlayer));
    }
    // save the the last state
    lastStateCB3 = currentStateCB3;

  }



  if (newState == "check4WinState") {
    //Serial.println("State: check4WinState");
    uint8_t lastTokenX, lastTokenY, lastTokenH = -1;

    getLastToken(&lastTokenX, &lastTokenY, &lastTokenH);

    Serial.println("check4WinState: check LastToken: " + String(lastTokenX) + ", " + String(lastTokenY) + ", " + String(lastTokenH));

    if (gameWinner == 1) { // if winner is myPlayer -> go first into syncState to sync the lastToken to Server and then go into winnerState
      Serial.println("change state: check4winState -> winnerState: " + newState);
      newState = "winnerState";
    }

    if (checkForWinner(lastTokenX, lastTokenY, lastTokenH - 1, playerLastToken)  ) { // check if there is already a winner of the game
      if (nextPlayer == (myPlayer + 1) % 2 && gameMode == 0) {
        setNetVar("4row_lastToken_x", String(lastToken[0]));
        setNetVar("4row_lastToken_y", String(lastToken[1]));
        setNetVar("4row_lastToken_h", String(lastToken[2]));
        setNetVar("4row_player_lastToken", String(myPlayer));
        moveCounter += 1;

        setNetVar("4row_moveCount", String(moveCounter));
        setNetVar("4row_nextPlayer", String(nextPlayer));
      }
      newState = "winnerState";

    } else {
      if (gameMode == 0) {
        if (nextPlayer == myPlayer) {
          newState = "pickTowerState";
        }
        if (nextPlayer == ((myPlayer + 1) % 2)) {
          newState = "syncState";
        }
      }
      if (gameMode == 1) {
        myPlayer = nextPlayer;
        newState = "pickTowerState";
      }

    }



  }

  if (newState == "winnerState") {
    //Serial.println("State: winnerState");
    resetAllLeds();
    for (int i = 0; i < 64; i++) {
      uint8_t x, y, h = 0;
      if (winningTokens[i] == 1) {
        calcTowerXYHFromLedNum(i, &x, &y, &h);
        setLedPair(x, y, h, players[gameState[i]]);
      }
    }

    delay(100);
    resetAllLeds();
    delay(100);

    // read the state of the switch/button: -> start new game
    currentStateCB3 = digitalRead(CASE_BUTTON_3);
    if (lastStateCB3 == LOW && currentStateCB3 == HIGH) {
      newState = "setupGameState";
    }
    // save the the last state
    lastStateCB3 = currentStateCB3;


  }

  if (newState == "light") {
    for (int towerID = 0; towerID < 16; towerID ++) {
      playStoneAnimation(towerID, 0, true, CHSV(color, 255, 255));
      color = (color + 10) % 256;
    }
  }

}
