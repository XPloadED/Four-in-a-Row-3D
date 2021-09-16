#include <Keypad.h>
#include <FastLED.h>

#define NUM_LEDS 128
#define LED_PIN 2
#define NUM_TOWER 16

#define NUM_PLAYER 2

CRGB leds[NUM_LEDS];

const byte ROWS = 4; // four rows
const byte COLS = 4; // four columns

String state = "select";
uint8_t hue = 0;

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

//player settings -> color of players
CHSV players[NUM_PLAYER] = {CHSV(150, 255, 255), CHSV(255, 255, 255)};
// var for alternating player
int alternate = 0;

// Map the buttons to an array for the Keymap instance
char hexaKeys[ROWS][COLS] = {
  {'0', '1', '2', '3'},
  {'4', '5', '6', '7'},
  {'8', '9', 'A', 'B'},
  {'C', 'D', 'E', 'F'}
};

byte colPins[ROWS] = {33, 26, 27, 12};
byte rowPins[COLS] = {22, 21, 19, 18};

// Initialise the Keypad
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

void setup() {
  Serial.begin(9600);   // Initialise the serial monitor
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(50);

  setupGame();
}

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

//setup game settings
void setupGame(){
  for(int i = 0; i < NUM_TOWER; i++){
    towerHeight[i] = 0; //height per tower at startup is 0
  }
  for (int j = 0; j < NUM_TOWER * 4; j++){
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
int calcLedNumbersOfTowerXY(uint8_t x, uint8_t y){
  if (x%2 == 0){                // is x even?
    return ((x*32)+(y*8));      // even x column
  }else{
    return ((x*32+32)-((y+1)*8)); // odd x column
  }
}

//-------------------------------------------------------------------------------------------------------------------

// set the color of a led pair at the tower (x,y) with (0 <= x <= 3) and y(0 <= y <= 3) in the height h ((0 <= h <= 3) (from bottom) in a CHSV color depending on player (0 or 1)
void setLedPair(uint8_t x, uint8_t y, uint8_t h, int player){
    int firstLed = calcLedNumbersOfTowerXY(x,y); // first led number of the tower

    leds[firstLed + h] = players[player]; //  set color for led at tower (x,y) and height h
    leds[firstLed + 7 - h] = players[player]; // set color for mirrored led at tower (x,y) and height h
    FastLED.show();
}

//-------------------------------------------------------------------------------------------------------------------
// reset the color of a led pair at the tower (x,y) with (0 <= x <= 3) and y(0 <= y <= 3) in the height h ((0 <= h <= 3) (from bottom)
void resetLedPair(uint8_t x, uint8_t y, uint8_t h){
  int firstLed = calcLedNumbersOfTowerXY(x,y); // first led number of the tower

  leds[firstLed + h] = CRGB::Black; // reset color for led at tower (x,y) and height h
  leds[firstLed + 7 - h] = CRGB::Black; // reset color for mirrored led at tower (x,y) and height h
  FastLED.show();
}

//-------------------------------------------------------------------------------------------------------------------

void animateGameToken(uint8_t x, uint8_t y, uint8_t player){
  int towerID = (x*4 + y);
  int time = 100;

  if (towerHeight[towerID] < 4){
    int tokenHeight = 3; // starting height
    setLedPair(x, y, tokenHeight, player);
    while (tokenHeight > towerHeight[towerID]){
      delay(time-=20);
      resetLedPair(x, y, tokenHeight);
      tokenHeight -= 1;
      setLedPair(x, y, tokenHeight, player);
    }

    towerHeight[towerID] += 1;
    gameState[(towerID*4 + towerHeight[towerID] - 1)] = player; // 1d representation of game for storing the game status
    alternate = ((alternate + 1) % 2); //for testing only -> alternate player
    
  }else{
    Serial.println("Full! no more tokens!");
  }
}

//-------------------------------------------------------------------------------------------------------------------
// reset the color of all leds
void resetAllLeds(){
  for (int i = 0; i < 128; i++){
    leds[i] = CRGB::Black; // reset color of all led
  }
  FastLED.show();
}
//-------------------------------------------------------------------------------------------------------------------
// check for a winner

//TODO: Update winning function

//void checkWinner(uint8_t x, uint8_t y, uint8_t h ){
//  if ((x < 0 or x > 3) and (y < 0 or y > 3) and ( < 0 or h > 3)){
//    
//  }
//}
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
            animateGameToken(0,0,alternate);     
          break;
        case '1':
//          for (int i = 0; i < NUM_LEDS; i++) {
//            leds[i] = CRGB::Black;
//          }
//          for (int i = 8; i < 16; i++) {
//            leds[i] = CHSV(255, 255, 255);
//          };
//          FastLED.show();
            animateGameToken(0,1,alternate);
          break;
        case '2':
//          for (int i = 0; i < NUM_LEDS; i++) {
//            leds[i] = CRGB::Black;
//          }
//          for (int i = 16; i < 24; i++) {
//            leds[i] = CHSV(255, 255, 255);
//          };
//          FastLED.show();
            animateGameToken(0,2,alternate);
          break;
        case '3':
//          for (int i = 0; i < NUM_LEDS; i++) {
//            leds[i] = CRGB::Black;
//          }
//          for (int i = 24; i < 32; i++) {
//            leds[i] = CHSV(255, 255, 255);
//          };
//          FastLED.show();
            animateGameToken(0,3,alternate);
          break;


        case '4':
//          for (int i = 0; i < NUM_LEDS; i++) {
//            leds[i] = CRGB::Black;
//          }
//          for (int i = 56; i < 64; i++) {
//            leds[i] = CHSV(255, 255, 255);
//          };
//          FastLED.show();
            animateGameToken(1,0,alternate);
          break;
        case '5':
//          for (int i = 0; i < NUM_LEDS; i++) {
//            leds[i] = CRGB::Black;
//          }
//          for (int i = 48; i < 56; i++) {
//            leds[i] = CHSV(255, 255, 255);
//          };
//          FastLED.show();
            animateGameToken(1,1,alternate);
          break;
        case '6':
//          for (int i = 0; i < NUM_LEDS; i++) {
//            leds[i] = CRGB::Black;
//          }
//          for (int i = 40; i < 48; i++) {
//            leds[i] = CHSV(255, 255, 255);
//          };
//          FastLED.show();
            animateGameToken(1,2,alternate);
          break;
        case '7':
//          for (int i = 0; i < NUM_LEDS; i++) {
//            leds[i] = CRGB::Black;
//          }
//          for (int i = 32; i < 40; i++) {
//            leds[i] = CHSV(255, 255, 255);
//          };
//          FastLED.show();
            animateGameToken(1,3,alternate);
          break;


        case '8':
//          for (int i = 0; i < NUM_LEDS; i++) {
//            leds[i] = CRGB::Black;
//          }
//          for (int i = 64; i < 72; i++) {
//            leds[i] = CHSV(255, 255, 255);
//          };
//          FastLED.show();
            animateGameToken(2,0,alternate);
          break;
        case '9':
//          for (int i = 0; i < NUM_LEDS; i++) {
//            leds[i] = CRGB::Black;
//          }
//          for (int i = 72; i < 80; i++) {
//            leds[i] = CHSV(255, 255, 255);
//          };
//          FastLED.show();
          animateGameToken(2,1,alternate);
          break;
        case 'A':
//          for (int i = 0; i < NUM_LEDS; i++) {
//            leds[i] = CRGB::Black;
//          }
//          for (int i = 80; i < 88; i++) {
//            leds[i] = CHSV(255, 255, 255);
//          };
//          FastLED.show();
          animateGameToken(2,2,alternate);
          break;
        case 'B':
//          for (int i = 0; i < NUM_LEDS; i++) {
//            leds[i] = CRGB::Black;
//          }
//          for (int i = 88; i < 96; i++) {
//            leds[i] = CHSV(255, 255, 255);
//          };
//          FastLED.show();
          animateGameToken(2,3,alternate);
          break;


        case 'C':
//          for (int i = 0; i < NUM_LEDS; i++) {
//            leds[i] = CRGB::Black;
//          }
//          for (int i = 120; i < 128; i++) {
//            leds[i] = CHSV(255, 255, 255);
//          };
//          FastLED.show();
          animateGameToken(3,0,alternate);
          break;
        case 'D':
//          for (int i = 0; i < NUM_LEDS; i++) {
//            leds[i] = CRGB::Black;
//          }
//          for (int i = 112; i < 120; i++) {
//            leds[i] = CHSV(255, 255, 255);
//          };
//          FastLED.show();
          animateGameToken(3,1,alternate);
          break;

        case 'E':

//          for (int i = 0; i < NUM_LEDS; i++) {
//            leds[i] = CRGB::Black;
//          }
//          for (int i = 104; i < 112; i++) {
//            leds[i] = CHSV(255, 255, 255);
//          };
//          FastLED.show();
          animateGameToken(3,2,alternate);
          break;
        case 'F':
          for (int i = 0; i < NUM_LEDS; i++) {
            leds[i] = CRGB::Black;
          }
          for (int i = 96; i < 104; i++) {
            leds[i] = CHSV(255, 255, 255);
          };
          FastLED.show();

          Serial.println("---------------------------------------------------------");
          for (int i = 0; i < NUM_TOWER; i++){
            Serial.println(towerHeight[i]);
          }
          Serial.println("---------------------------------------------------------");

          for (int i = 0; i < NUM_TOWER*4; i++){
            Serial.println(gameState[i]);
          }
          setupGame();
          break;
      }
    }
  }

//TODO: Update winning state

//  if (state == "winner"){
//    
//  }


}
