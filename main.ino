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

void loop() {

  //StartUp routine
  if(state == "startup"){
    
    /*for(int i = 0, b = 1; i < NUM_LEDS; i++){
      leds[i] = CRGB::Red;
      leds[b * 7 - (i % 7)] = CRGB::Red;
      FastLED.show();
      delay(1000);
      
      leds[i]= CRGB::Black;
      leds[b * 7 - (i % 7)] = CRGB::Black;

      if((i+1)%4 == 0){
        i = (b * 8) - 1;
        b++;
        }
      }*/

    for(int i = 0, b = 1; i < NUM_LEDS; i++){
      leds[i] = CRGB::Red;
      FastLED.show();
      delay(40);
      
      leds[i]= CRGB::Black;
    }
      
    for(int i = NUM_LEDS - 1; i >= 0; i--){
      leds[i] = CRGB::Red;
      FastLED.show();
      delay(40);
      leds[i]= CRGB::Black;
      leds[i+8]= CRGB::Black;
      FastLED.show();
      }

    // End of the startup routine, later switch to gamemode [singleplayer/multiplayer/atmospheric lamp]
    // For now leads into the tower select state, in which the player can select the a tower through the button matrix
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
