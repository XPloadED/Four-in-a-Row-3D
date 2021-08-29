#include <Keypad.h>
#include <FastLED.h>

#define NUM_LEDS 128
#define LED_PIN 2

CRGB leds[NUM_LEDS];

const byte ROWS = 4; // four rows
const byte COLS = 4; // four columns

String state = "startup";
uint8_t hue = 0;

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
}

// plays the stone drop down animation until it reaches the bottom spot
void playStoneAnimation(int towerID, int stackHeight, bool startup, CHSV Player){
  // find led spot of the top of the tower and initialice the mirror stone
  int runindex = (towerID * 8) + 3;
  int mirrorstone = runindex + 1;
  int deltaDuration = 100;
  
  if(stackHeight < 3){
    //Display the stone for deltaduration
    leds[runindex] = Player;
    leds[mirrorstone] = Player;
    FastLED.show();
    delay(deltaDuration);
    if(not startup){
      leds[runindex] = CRGB::Black;
      leds[mirrorstone] = CRGB::Black;
      FastLED.show();
      }
    
  }

  // while the tower has empty spaces, repeat until it reaches the state bottom
  while(stackHeight < 3){
    runindex = runindex - 1;
    mirrorstone = mirrorstone + 1;
    leds[runindex] = Player;
    leds[mirrorstone] = Player;
    FastLED.show();
    delay(deltaDuration = deltaDuration - 20);
    
    stackHeight++;
        
    if(not startup){
      leds[runindex] = CRGB::Black;
      leds[mirrorstone] = CRGB::Black;
      FastLED.show();
    if(stackHeight == 3){
      leds[runindex] = Player;
      leds[mirrorstone] = Player;
      FastLED.show();
      }
      }
    }
  }

void loop() {

  //StartUp routine
  if(state == "startup"){
  
   
      for(int towerID = 0; towerID < 16; towerID ++){
        playStoneAnimation(towerID, random(0,3), true, CHSV(random8(), 255, 255));
        }
      for(int towerID = 15; towerID >= 0; towerID --){
        playStoneAnimation(random(0,15), random(0,3), false, CHSV(random8(), 255, 255));
        }
      for(int towerID = 0; towerID < 16; towerID ++){
        playStoneAnimation(towerID, random(0,3), true, CHSV(random8(), 255, 255));
        }
        

    // End of the startup routine, later switch to gamemode [singleplayer/multiplayer/atmospheric lamp]
    // For now leads into the tower select state, in which the player can select a tower through the button matrix
    state = "select";
    }
  
if(state == "select"){
 char button = customKeypad.getKey();
  if (button) {
      switch (button){
        case '0': 
            for(int i = 0; i < NUM_LEDS; i++){
              leds[i] = CRGB::Black;
            }
            for(int i = 0; i < 8; i++){
              leds[i] = CHSV(255, 255, 255); 
            };
            FastLED.show();
            break;
        case '1':
            for(int i = 0; i < NUM_LEDS; i++){
              leds[i] = CRGB::Black;
            }
            for(int i = 8; i < 16; i++){
              leds[i] = CHSV(255, 255, 255); 
              };
              FastLED.show();
            break;
        case '2': 
            for(int i = 0; i < NUM_LEDS; i++){
              leds[i] = CRGB::Black;
            }
            for(int i = 16; i < 24; i++){
              leds[i] = CHSV(255, 255, 255); 
              };
              FastLED.show();
            break;
        case '3': 
            for(int i = 0; i < NUM_LEDS; i++){
              leds[i] = CRGB::Black;
            }
            for(int i = 24; i < 32; i++){
              leds[i] = CHSV(255, 255, 255); 
              };
              FastLED.show();
            break;

            
        case '4': 
            for(int i = 0; i < NUM_LEDS; i++){
              leds[i] = CRGB::Black;
            }
            for(int i = 56; i < 64; i++){
              leds[i] = CHSV(255, 255, 255); 
              };
              FastLED.show();
            break;
        case '5': 
            for(int i = 0; i < NUM_LEDS; i++){
              leds[i] = CRGB::Black;
            }
            for(int i = 48; i < 56; i++){
              leds[i] = CHSV(255, 255, 255); 
              };
              FastLED.show();
            break;
        case '6': 
            for(int i = 0; i < NUM_LEDS; i++){
              leds[i] = CRGB::Black;
            }
            for(int i = 40; i < 48; i++){
              leds[i] = CHSV(255, 255, 255); 
              };
              FastLED.show();
            break;
        case '7': 
            for(int i = 0; i < NUM_LEDS; i++){
              leds[i] = CRGB::Black;
            }
            for(int i = 32; i < 40; i++){
              leds[i] = CHSV(255, 255, 255); 
              };
              FastLED.show();
            break;

            
        case '8': 
            for(int i = 0; i < NUM_LEDS; i++){
              leds[i] = CRGB::Black;
            }
            for(int i = 64; i < 72; i++){
              leds[i] = CHSV(255, 255, 255); 
              };
              FastLED.show();
            break;            
        case '9': 
            for(int i = 0; i < NUM_LEDS; i++){
              leds[i] = CRGB::Black;
            }
            for(int i = 72; i < 80; i++){
              leds[i] = CHSV(255, 255, 255); 
              };
              FastLED.show();
            break;            
        case 'A': 

            for(int i = 0; i < NUM_LEDS; i++){
              leds[i] = CRGB::Black;
            }
            for(int i = 80; i < 88; i++){
              leds[i] = CHSV(255, 255, 255); 
              };
              FastLED.show();
            break;
        case 'B': 

            for(int i = 0; i < NUM_LEDS; i++){
              leds[i] = CRGB::Black;
            }
            for(int i = 88; i < 96; i++){
              leds[i] = CHSV(255, 255, 255); 
              };
              FastLED.show();
            break;

            
        case 'C': 
            for(int i = 0; i < NUM_LEDS; i++){
              leds[i] = CRGB::Black;
            }
            for(int i = 120; i < 128; i++){
              leds[i] = CHSV(255, 255, 255); 
              };
              FastLED.show();
            break;
        case 'D': 
            for(int i = 0; i < NUM_LEDS; i++){
              leds[i] = CRGB::Black;
            }
            for(int i = 112; i < 120; i++){
              leds[i] = CHSV(255, 255, 255); 
              };
              FastLED.show();
            break;
            
        case 'E': 

            for(int i = 0; i < NUM_LEDS; i++){
              leds[i] = CRGB::Black;
            }
            for(int i = 104; i < 112; i++){
              leds[i] = CHSV(255, 255, 255); 
              };
              FastLED.show();
            break;
        case 'F': 
            for(int i = 0; i < NUM_LEDS; i++){
              leds[i] = CRGB::Black;
            }
            for(int i = 96; i < 104; i++){
              leds[i] = CHSV(255, 255, 255); 
              };
              FastLED.show();
            break;            
      }
    }
  }


 

}
