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

// array for winning row
int winningTokens[4][3] = {};

//define starting state
String state = "select";

//----------------------------------------------------------------------------------------
//player settings
//color of players 0 and 1
CHSV players[NUM_PLAYER] = {CHSV(150, 255, 255), CHSV(255, 255, 255)};
// var for alternating player
int alternate = 0;


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
//CASE BUTTONS

int lastStateCB1 = HIGH; // the previous state from the input pin CASEBUTTON1
int currentStateCB1;    // the current reading from the input pin

int lastStateCB2= HIGH; // the previous state from the input pin CASEBUTTON1
int currentStateCB2;    // the current reading from the input pin

int lastStateCB3 = HIGH; // the previous state from the input pin CASEBUTTON1
int currentStateCB3;    // the current reading from the input pin


//----------------------------------------------------------------------------------------
//Led stipe
uint8_t hue = 0;
//init length of the LED stripe
CRGB leds[NUM_LEDS];


//----------------------------------------------------------------------------------------
void setup() {
  Serial.begin(9600);   // Initialise the serial monitor
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(50);

  pinMode(CASE_BUTTON_1, INPUT_PULLUP);
  pinMode(CASE_BUTTON_2, INPUT_PULLUP);
  pinMode(CASE_BUTTON_3, INPUT_PULLUP);
  
  setupGame();
}

//----------------------------------------------------------------------------------------
//setup game settings
void setupGame() {
  for (int i = 0; i < NUM_TOWER; i++) {
    towerHeight[i] = 0; //height per tower at startup is 0
  }
  for (int j = 0; j < NUM_TOWER * 4; j++) {
    gameState[j] = -1;  //-1 -> means no player, 0 -> player 1, 1 -> player 2
  }
  
  alternate = 0;
  resetAllLeds();
}

//////////////////////////////////////////////////////////////////////////////////////
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
//
int calcLedNumbersOfTowerXY(uint8_t x, uint8_t y) {
  if (x % 2 == 0) {             // is x even?
    return ((x * 32) + (y * 8)); // even x column
  } else {
    return ((x * 32 + 32) - ((y + 1) * 8)); // odd x column
  }
}

//-------------------------------------------------------------------------------------------------------------------

// set the color of a led pair at the tower (x,y) with (0 <= x <= 3) and y(0 <= y <= 3) in the height h ((0 <= h <= 3) (from bottom) in a CHSV color depending on player (0 or 1)
void setLedPair(uint8_t x, uint8_t y, uint8_t h, int player) {
  int firstLed = calcLedNumbersOfTowerXY(x, y); // first led number of the tower

  leds[firstLed + h] = players[player]; //  set color for led at tower (x,y) and height h
  leds[firstLed + 7 - h] = players[player]; // set color for mirrored led at tower (x,y) and height h
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

void animateGameToken(uint8_t x, uint8_t y, uint8_t player) {
  int towerID = (x * 4 + y);
  int time = 100;

  if (towerHeight[towerID] < 4) {
    int tokenHeight = 3; // starting height -> stone is falling down
    setLedPair(x, y, tokenHeight, player);
    while (tokenHeight > towerHeight[towerID]) {
      delay(time -= 20);
      resetLedPair(x, y, tokenHeight);
      tokenHeight -= 1;
      setLedPair(x, y, tokenHeight, player);
    }

    towerHeight[towerID] += 1;
    gameState[(towerID * 4 + towerHeight[towerID] - 1)] = player; // 1d representation of game for storing the game status
    alternate = ((alternate + 1) % 2); //for testing only -> alternate player

    
    if (checkForWinner(x, y, (towerHeight[towerID] - 1), player)  ) { // check if there is already a winner of the game
      state = "winner"; //if there is a winner change the state to winner
    }
  } else {
    Serial.println("Full! no more tokens!");
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


//-------------------------------------------------------------------------------------------------------------------
// reset the color of all leds
void resetAllLeds() {
  for (int i = 0; i < 128; i++) {
    leds[i] = CRGB::Black; // reset color of all led
  }
  FastLED.show();
}
//-------------------------------------------------------------------------------------------------------------------
// check for a winner
bool checkForWinner(uint8_t x, uint8_t y, uint8_t h, int player) {
  //all possible neighbor fields as vector
  int vector[13][3] = {{1, 0, 0}, //{-1,0,0},
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
    { -1, -1, 1}, // {1,-1,1},
    {1, 1, 1}}; //,{-1,-1,-1}};

  return countToken(x + vector[0][0], y + vector[0][1], h + vector[0][2], vector[0][0], vector[0][1], vector[0][2], player) + countToken(x - vector[0][0], y - vector[0][1], h - vector[0][2], -vector[0][0], -vector[0][1],- vector[0][2], player) >= 3 ||
         countToken(x + vector[1][0], y + vector[1][1], h + vector[1][2], vector[1][0], vector[1][1], vector[1][2], player) + countToken(x - vector[1][0], y - vector[1][1], h - vector[1][2], -vector[1][0], -vector[1][1], -vector[1][2], player) >= 3 ||
         countToken(x + vector[2][0], y + vector[2][1], h + vector[2][2], vector[2][0], vector[2][1], vector[2][2], player) + countToken(x - vector[2][0], y - vector[2][1], h - vector[2][2], -vector[2][0], -vector[2][1], -vector[2][2], player) >= 3 ||
         countToken(x + vector[3][0], y + vector[3][1], h + vector[3][2], vector[3][0], vector[3][1], vector[3][2], player) + countToken(x - vector[3][0], y - vector[3][1], h - vector[3][2], -vector[3][0], -vector[3][1], -vector[3][2], player) >= 3 ||
         countToken(x + vector[4][0], y + vector[4][1], h + vector[4][2], vector[4][0], vector[4][1], vector[4][2], player) + countToken(x - vector[4][0], y - vector[4][1], h - vector[4][2], -vector[4][0], -vector[4][1], -vector[4][2], player) >= 3 ||
         countToken(x + vector[5][0], y + vector[5][1], h + vector[5][2], vector[5][0], vector[5][1], vector[5][2], player) + countToken(x - vector[5][0], y - vector[5][1], h - vector[5][2], -vector[5][0], -vector[5][1], -vector[5][2], player) >= 3 ||
         countToken(x + vector[6][0], y + vector[6][1], h + vector[6][2], vector[6][0], vector[6][1], vector[6][2], player) + countToken(x - vector[6][0], y - vector[6][1], h - vector[6][2], -vector[6][0], -vector[6][1], -vector[6][2], player) >= 3 ||
         countToken(x + vector[7][0], y + vector[7][1], h + vector[7][2], vector[7][0], vector[7][1], vector[7][2], player) + countToken(x - vector[7][0], y - vector[7][1], h - vector[7][2], -vector[7][0], -vector[7][1], -vector[7][2], player) >= 3 ||
         countToken(x + vector[8][0], y + vector[8][1], h + vector[8][2], vector[8][0], vector[8][1], vector[8][2], player) + countToken(x - vector[8][0], y - vector[8][1], h - vector[8][2], -vector[8][0], -vector[8][1], -vector[8][2], player) >= 3 ||
         countToken(x + vector[9][0], y + vector[9][1], h + vector[9][2], vector[9][0], vector[9][1], vector[9][2], player) + countToken(x - vector[9][0], y - vector[9][1], h - vector[9][2], -vector[9][0], -vector[9][1], -vector[9][2], player) >= 3 ||
         countToken(x + vector[10][0], y + vector[10][1], h + vector[10][2], vector[10][0], vector[10][1], vector[10][2], player) + countToken(x - vector[10][0], y - vector[10][1], h - vector[10][2], -vector[10][0], -vector[10][1], -vector[10][2], player) >= 3 ||
         countToken(x + vector[11][0], y + vector[11][1], h + vector[11][2], vector[11][0], vector[11][1], vector[11][2], player) + countToken(x - vector[11][0], y - vector[11][1], h - vector[11][2], -vector[11][0], -vector[11][1], -vector[11][2], player) >= 3 ||
         countToken(x + vector[12][0], y + vector[12][1], h + vector[12][2], vector[12][0], vector[12][1], vector[12][2], player) + countToken(x - vector[12][0], y - vector[12][1], h - vector[12][2], -vector[12][0], -vector[12][1], -vector[12][2], player) >= 3 ;
}

int countToken(uint8_t x, uint8_t y, uint8_t h, int delta_x, int delta_y, int delta_h, int player) { //, int** tokenRow, int count){
  if ((x >= 0 && x < 4) && (y >= 0 && y < 4) && (h >= 0 && h < 4)) { //if token is in the bounderies of the game
    int newField = ((x * 4 + y) * 4 + h);
    if (gameState[newField] == player) {
      Serial.print("x: ");
      Serial.println(x);
      Serial.print("y: ");
      Serial.println(y);
      Serial.print("h: ");
      Serial.println(h);
      Serial.println("");
      return countToken(x + delta_x, y + delta_y, h + delta_h, delta_x, delta_y, delta_h, player) + 1;
    } else {
      return 0; // newField and player are not identical
    }
  } else {
    Serial.println("Search out of boundary! no more tokens!");
    Serial.print("x: ");
    Serial.println(x);
    Serial.print("y: ");
    Serial.println(y);
    Serial.print("h: ");
    Serial.println(h);
    Serial.println("");
    return 0;
  }
}


//-------------------------------------------------------------------------------------------------------------------
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
    state = "select";
  }

  if (state == "select") {

    //Button matrix
    char button = customKeypad.getKey();
    if (button) {
      switch (button) {
        case '0':
          //          for (int i = 0; i < NUM_LEDS; i++) {
          //            leds[i] = CRGB::Black;
          //          }
          //          for (int i = 0; i < 8; i++) {
          //            leds[i] = CHSV(255, 255, 255);
          //          };
          //          FastLED.show();
          animateGameToken(0, 0, alternate);
          break;
        case '1':
          animateGameToken(0, 1, alternate);
          break;
        case '2':
          animateGameToken(0, 2, alternate);
          break;
        case '3':
          animateGameToken(0, 3, alternate);
          break;

        case '4':
          animateGameToken(1, 0, alternate);
          break;
        case '5':
          animateGameToken(1, 1, alternate);
          break;
        case '6':
          animateGameToken(1, 2, alternate);
          break;
        case '7':
          animateGameToken(1, 3, alternate);
          break;

        case '8':
          animateGameToken(2, 0, alternate);
          break;
        case '9':
          animateGameToken(2, 1, alternate);
          break;
        case 'A':
          animateGameToken(2, 2, alternate);
          break;
        case 'B':
          animateGameToken(2, 3, alternate);
          break;

        case 'C':
          animateGameToken(3, 0, alternate);
          break;
        case 'D':
          animateGameToken(3, 1, alternate);
          break;
        case 'E':
          animateGameToken(3, 2, alternate);
          break;
        case 'F':
          animateGameToken(3, 3, alternate);
          break;
      }
    }
  // read the state of the switch/button:
  currentStateCB1 = digitalRead(CASE_BUTTON_1);

  if(lastStateCB1 == LOW && currentStateCB1 == HIGH){
    Serial.println("The state changed from LOW to HIGH");
    Serial.println("---------------------------------------------------------");
    for (int i = 0; i < NUM_TOWER; i++) {
      Serial.println(towerHeight[i]);
    }
    Serial.println("---------------------------------------------------------");

    for (int i = 0; i < NUM_TOWER * 4; i++) {
      Serial.println(gameState[i]);
    }
    setupGame();
  }

  // save the the last state
  lastStateCB1 = currentStateCB1;
    
  }

  //TODO: Update winning state

  if (state == "winner") {
    resetAllLeds();
    setupGame();
    state = "select";
  }


}
