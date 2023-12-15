#include <FastLED.h>
#include <EEPROM.h>

#define LED_PIN     6
#define NUM_LEDS    150
#define LED_TYPE    WS2811
#define COLOR_ORDER RGB

//this code is made for an arduino. It controls a LED-Controller that consists of:
//-- a BIG Buzzer, that's the main button people should press
//-- 4 Dip-Switches, 3 of them are used to input a 3-bit number and the last one is used to switch between run- and setting-mode
//-- a potentiometer to control the brightness in run-mode and change settings in setting-mode

int* readFromEEPROM() {
    static int data[8];  // Making it static to retain its value after the function returns
    for (int i = 0; i < 8; i++) {
        data[i] = EEPROM.read(i);
    }
    return data;
}

CRGB leds[NUM_LEDS];
// CRGB leds[readFromEEPROM()[NUMBER_LEDS]]; Initialize led array with nubmer of elements from eeprom

#define BUTTON_PIN 8
#define POT_PIN    A3
#define DIP1       2
#define DIP2       3
#define DIP3       4
#define DIP4       5

enum LEDSetting {
    COLOR_RED,
    COLOR_GREEN,
    COLOR_BLUE,
    SPEED,
    NUMBER_LEDS,
    RESERVED1,
    RESERVED2,
    RESERVED3
    // Add other settings as needed
};

class LEDSettingsManager {

  //Settings:
  // 0 Color Red - 0=Random Color, 1-254 = fixed color, 255 = white
  // 1 Color Green - 0..255
  // 2 Color Blue - 0..255
  // 3 Speed - Speed from 0 (slow) to 255 (fast)
  // 4 Number Leds - 50..255, when this is changed, the device needs to be reset!
  //rest is reserved
private:
    byte setts[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    int displayStartIndex = 0; // Start index for display settings
    int displayEndIndex = 44; // End index for display settings
    bool toggle = false; //used to blink the LEDs

public:
    LEDSettingsManager() {
        // Initialize settings array
      read_from_eeprom();
    }

    void read_from_eeprom() {
      for (int i = 0; i < 8; i++) {
          setts[i] = EEPROM.read(i);
      }
    }

    bool is_dark(){
      return (setts[COLOR_RED] <= 5 && setts[COLOR_GREEN] <= 5 && setts[COLOR_BLUE] <= 5);
    }

    CRGB get_color(){
      return CRGB(setts[COLOR_RED], setts[COLOR_GREEN], setts[COLOR_BLUE]);
    }

    byte get_speed(){
      return setts[SPEED];
    }

    void display_setting(LEDSetting setting_index, bool blinking=false) {
        // Show current setting value as solid LEDs
        int value = map(setts[setting_index], 0, 255, displayStartIndex, displayEndIndex);
        CRGB bar_color = CRGB::Red;
        if(setting_index == NUMBER_LEDS){
          //changing Number of LEDs
          value = setts[NUMBER_LEDS]; //use value unscaled, so the length of the LED strip can be adjusted from 0 .. 255
          bar_color = CRGB::Blue;
        }
        if(setting_index >= 0 && setting_index <= 2){
          //setting color, use current color to display
          if(setts[0] <= 5 && setts[1] <= 5 && setts[2] <= 5){
            //all Colors low, choose random
            bar_color = CRGB(random(255), random(255), random(255));
          } else{
            bar_color = get_color();
          }
        }

        //Display LED-Bar
        for (int i = 0; i < NUM_LEDS; i++) {
            if (i >= (displayStartIndex) && i <= value) {
                leds[i] = toggle || !blinking ? bar_color : CRGB::Black;
            } else {
                leds[i] = CRGB::Black;
            }
        }          
        
        toggle = !toggle;
        delay(map(setts[SPEED], 0, 255, 100, 50)); //blink with current speed setting
        // Note: FastLED.show() should be called externally after this function
    }

    void set_setting(byte setting_index, byte value) {
        if(setting_index == 5 && value < 30){
          //limit number of LEDs to 
          value = 30;
        }
        setts[setting_index] = value;
        display_setting(setting_index, true); // Update display to show new setting
        // Note: FastLED.show() should be called externally after this function
    }

    byte get_setting(byte setting_index) {
        return setts[setting_index];
    }

    byte* get_settings() {
        return setts;
    }

    void write_to_eeprom(){
      //writes the current settings to EEPROM
      for (int i = 0; i < 8; i++) {
        EEPROM.write(i, setts[i]);
      }
      Serial.println("written to EEPROM");
    }

    // Additional methods and properties can be added as needed
};

class LEDMode {
public:
    virtual void update(LEDSettingsManager settings) = 0;
};

class RunningDotMode : public LEDMode {
private:
    bool oldButtonState = HIGH;

public:
    void update(LEDSettingsManager settings) override {
        bool newButtonState = digitalRead(BUTTON_PIN);
        if (newButtonState == LOW && oldButtonState == HIGH) {
            CRGB color = settings.is_dark() ? CRGB(random(255), random(255), random(255)) : settings.get_color(); // Random or white color if no color selected
            leds[0] = color;
        }
        oldButtonState = newButtonState;

        for (int i = NUM_LEDS - 1; i > 0; i--) {
            leds[i] = leds[i - 1];
        }
        leds[0] = CRGB::Black;

        delay(map(settings.get_speed(), 0, 255, 300, 3)); // Speed of shifting
    }
};

class LightSwitchMode : public LEDMode {
private:
    bool isOn = false;
    bool oldButtonState = HIGH;

public:
    void update(LEDSettingsManager settings) override {
        bool newButtonState = digitalRead(BUTTON_PIN);
        if (newButtonState == LOW && oldButtonState == HIGH) {
            isOn = !isOn;
            CRGB color = isOn ? (settings.is_dark() ? CRGB(random(255), 255, 255) : settings.get_color()) : CRGB::Black; // Random or white color based on parameter
            fill_solid(leds, NUM_LEDS, color);
            delay(20); // Debounce delay
        } else {
          //avoid super fast looping
          delay(20);
        }
        oldButtonState = newButtonState;
    }
};

class GradualFillMode : public LEDMode {
private:
    int fillLevel = 0;
    bool filling = false;

public:
    void update(LEDSettingsManager settings) override {
        if (digitalRead(BUTTON_PIN) == LOW) {
            filling = true;
        } else {
            filling = false;
        }

        if (filling && fillLevel < NUM_LEDS) {
            fillLevel++;
        } else if (!filling && fillLevel > 0) {
            fillLevel--;
        }

        CRGB color = calculateUniformColor(fillLevel);
        for (int i = 0; i < NUM_LEDS; i++) {
            leds[i] = i < fillLevel ? color : CRGB::Black;
        }
        delay(10);
    }

    CRGB calculateUniformColor(int fillLevel) {
        float ratio = float(fillLevel) / float(NUM_LEDS);
        if (ratio < 0.33) {
            return CRGB::Green; // Green when few LEDs are lit
        } else if (ratio < 0.66) {
            return blend(CRGB::Green, CRGB::Red, (ratio - 0.33) * 6.0); // Blend to yellow and then to red
        } else {
            return CRGB::Red; // Red when many LEDs are lit
        }
    }
};

class WaveformMode : public LEDMode {
private:
    int fillLevel = 0;
    bool filling = false;
    float phaseShift = 0.0; // For sine wave generation

public:
    void update(LEDSettingsManager settings) override {
        bool buttonState = digitalRead(BUTTON_PIN);
        filling = (buttonState == LOW);

        if (filling && fillLevel < NUM_LEDS) {
            fillLevel++;
        } else if (!filling && fillLevel > 0) {
            fillLevel--;
        }

        for (int i = 0; i < NUM_LEDS; i++) {
            if (i < fillLevel) {
                float waveValue = sin(i * 0.2 + phaseShift); // Sine wave calculation
                waveValue = (waveValue + 1) / 2; // Normalize to 0-1

                CRGB color = settings.is_dark() ? CRGB(random(255), random(255), random(255)) : settings.get_color();
                leds[i] = color;
                leds[i].fadeToBlackBy(255 * (1 - waveValue));
            } else {
                leds[i] = CRGB::Black;
            }
        }

        phaseShift += 0.1; // Adjust for wave movement
        if (phaseShift > TWO_PI) {
            phaseShift -= TWO_PI;
        }

        delay(10); // Adjust delay for fill speed
    }
};

class TwinklesMode : public LEDMode {
private:
    // Twinkle parameters
    int twinkleSpeed = 2;
    int twinkleDensity = 1;
    CRGB backgroundColor = CRGB::Black;
    #define AUTO_SELECT_BACKGROUND_COLOR 0
    #define COOL_LIKE_INCANDESCENT 0

    CRGBPalette16 currentPalette;
    CRGBPalette16 targetPalette;

    static const CRGB RedGreenWhite_p[16];
    static const CRGB Holly_p[16];
    static const CRGB RedWhite_p[16];
    static const CRGB BlueWhite_p[16];
    static const CRGB FairyLight_p[16];
    static const CRGB Snow_p[16];
    static const CRGB RetroC9_p[16];
    static const CRGB Ice_p[16];

public:
    TwinklesMode() {
        // Initialize with a default palette
        currentPalette = RainbowColors_p; // Replace with your default palette
        targetPalette = currentPalette;
    }

    const CRGB* getPalette(int index){
      switch(index){
        case 0: return RedGreenWhite_p;
        case 1: return Holly_p;
        case 2: return RedWhite_p;
        case 3: return BlueWhite_p;
        case 4: return FairyLight_p;
        case 5: return Snow_p;
        case 6: return RetroC9_p;
        case 7: return Ice_p;
      }
    }

    void update(LEDSettingsManager settings) override {
        CRGBSet ledSet(leds, NUM_LEDS);
        drawTwinkles(ledSet);
    }

    void drawTwinkles(CRGBSet& L) {
      // "PRNG16" is the pseudorandom number generator
      // It MUST be reset to the same starting value each time
      // this function is called, so that the sequence of 'random'
      // numbers that it generates is (paradoxically) stable.
      uint16_t PRNG16 = 11337;
      
      uint32_t clock32 = millis();

      // Set up the background color, "bg".
      // if AUTO_SELECT_BACKGROUND_COLOR == 1, and the first two colors of
      // the current palette are identical, then a deeply faded version of
      // that color is used for the background color
      CRGB bg;
      if( (AUTO_SELECT_BACKGROUND_COLOR == 1) &&
          (currentPalette[0] == currentPalette[1] )) {
        bg = currentPalette[0];
        uint8_t bglight = bg.getAverageLight();
        if( bglight > 64) {
          bg.nscale8_video( 16); // very bright, so scale to 1/16th
        } else if( bglight > 16) {
          bg.nscale8_video( 64); // not that bright, so scale to 1/4th
        } else {
          bg.nscale8_video( 86); // dim, scale to 1/3rd.
        }
      } else {
        bg = backgroundColor; // just use the explicitly defined background color
      }

      uint8_t backgroundBrightness = bg.getAverageLight();
      
      for( CRGB& pixel: L) {
        PRNG16 = (uint16_t)(PRNG16 * 2053) + 1384; // next 'random' number
        uint16_t myclockoffset16= PRNG16; // use that number as clock offset
        PRNG16 = (uint16_t)(PRNG16 * 2053) + 1384; // next 'random' number
        // use that number as clock speed adjustment factor (in 8ths, from 8/8ths to 23/8ths)
        uint8_t myspeedmultiplierQ5_3 =  ((((PRNG16 & 0xFF)>>4) + (PRNG16 & 0x0F)) & 0x0F) + 0x08;
        uint32_t myclock30 = (uint32_t)((clock32 * myspeedmultiplierQ5_3) >> 3) + myclockoffset16;
        uint8_t  myunique8 = PRNG16 >> 8; // get 'salt' value for this pixel

        // We now have the adjusted 'clock' for this pixel, now we call
        // the function that computes what color the pixel should be based
        // on the "brightness = f( time )" idea.
        CRGB c = computeOneTwinkle( myclock30, myunique8);

        uint8_t cbright = c.getAverageLight();
        int16_t deltabright = cbright - backgroundBrightness;
        if( deltabright >= 32 || (!bg)) {
          // If the new pixel is significantly brighter than the background color, 
          // use the new color.
          pixel = c;
        } else if( deltabright > 0 ) {
          // If the new pixel is just slightly brighter than the background color,
          // mix a blend of the new color and the background color
          pixel = blend( bg, c, deltabright * 8);
        } else { 
          // if the new pixel is not at all brighter than the background color,
          // just use the background color.
          pixel = bg;
        }
      }
    }

    CRGB computeOneTwinkle(uint32_t ms, uint8_t salt) {
      //  This function takes a time in pseudo-milliseconds,
      //  figures out brightness = f( time ), and also hue = f( time )
      //  The 'low digits' of the millisecond time are used as 
      //  input to the brightness wave function.  
      //  The 'high digits' are used to select a color, so that the color
      //  does not change over the course of the fade-in, fade-out
      //  of one cycle of the brightness wave function.
      //  The 'high digits' are also used to determine whether this pixel
      //  should light at all during this cycle, based on the TWINKLE_DENSITY.
      uint16_t ticks = ms >> (8-twinkleSpeed);
      uint8_t fastcycle8 = ticks;
      uint16_t slowcycle16 = (ticks >> 8) + salt;
      slowcycle16 += sin8( slowcycle16);
      slowcycle16 =  (slowcycle16 * 2053) + 1384;
      uint8_t slowcycle8 = (slowcycle16 & 0xFF) + (slowcycle16 >> 8);
      
      uint8_t bright = 0;
      if( ((slowcycle8 & 0x0E)/2) < twinkleDensity) {
        bright = attackDecayWave8( fastcycle8);
      }

      uint8_t hue = slowcycle8 - salt;
      CRGB c;
      if( bright > 0) {
        c = ColorFromPalette( currentPalette, hue, bright, NOBLEND);
        if( COOL_LIKE_INCANDESCENT == 1 ) {
          coolLikeIncandescent( c, fastcycle8);
        }
      } else {
        c = CRGB::Black;
      }
      return c; 
    }

    uint8_t attackDecayWave8(uint8_t i) {
      // This function is like 'triwave8', which produces a 
      // symmetrical up-and-down triangle sawtooth waveform, except that this
      // function produces a triangle wave with a faster attack and a slower decay:
      //
      //     / \ 
      //    /     \ 
      //   /         \ 
      //  /             \ 
      //
      if( i < 86) {
        return i * 3;
      } else {
        i -= 86;
        return 255 - (i + (i/2));
      }
    }


    // This function takes a pixel, and if its in the 'fading down'
    // part of the cycle, it adjusts the color a little bit like the 
    // way that incandescent bulbs fade toward 'red' as they dim.
    void coolLikeIncandescent(CRGB& c, uint8_t phase) {
      if( phase < 128) return;

      uint8_t cooling = (phase - 128) >> 4;
      c.g = qsub8( c.g, cooling);
      c.b = qsub8( c.b, cooling * 2);
    }

    // Additional methods as needed
};

// A mostly red palette with green accents and white trim.
// "CRGB::Gray" is used as white to keep the brightness more uniform.
const CRGB TwinklesMode::RedGreenWhite_p[16] =
{  CRGB::Red, CRGB::Red, CRGB::Red, CRGB::Red, 
  CRGB::Red, CRGB::Red, CRGB::Red, CRGB::Red, 
  CRGB::Red, CRGB::Red, CRGB::Gray, CRGB::Gray, 
  CRGB::Green, CRGB::Green, CRGB::Green, CRGB::Green };

// A mostly (dark) green palette with red berries.
#define Holly_Green 0x00580c
#define Holly_Red   0xB00402
const CRGB TwinklesMode::Holly_p[16] =
{  Holly_Green, Holly_Green, Holly_Green, Holly_Green, 
  Holly_Green, Holly_Green, Holly_Green, Holly_Green, 
  Holly_Green, Holly_Green, Holly_Green, Holly_Green, 
  Holly_Green, Holly_Green, Holly_Green, Holly_Red 
};

// A red and white striped palette
// "CRGB::Gray" is used as white to keep the brightness more uniform.
const CRGB TwinklesMode::RedWhite_p[16] =
{  CRGB::Red,  CRGB::Red,  CRGB::Red,  CRGB::Red, 
  CRGB::Gray, CRGB::Gray, CRGB::Gray, CRGB::Gray,
  CRGB::Red,  CRGB::Red,  CRGB::Red,  CRGB::Red, 
  CRGB::Gray, CRGB::Gray, CRGB::Gray, CRGB::Gray };

// A mostly blue palette with white accents.
// "CRGB::Gray" is used as white to keep the brightness more uniform.
const CRGB TwinklesMode::BlueWhite_p[16] =
{  CRGB::Blue, CRGB::Blue, CRGB::Blue, CRGB::Blue, 
  CRGB::Blue, CRGB::Blue, CRGB::Blue, CRGB::Blue, 
  CRGB::Blue, CRGB::Blue, CRGB::Blue, CRGB::Blue, 
  CRGB::Blue, CRGB::Gray, CRGB::Gray, CRGB::Gray };

// A pure "fairy light" palette with some brightness variations
#define HALFFAIRY ((CRGB::FairyLight & 0xFEFEFE) / 2)
#define QUARTERFAIRY ((CRGB::FairyLight & 0xFCFCFC) / 4)
const CRGB TwinklesMode::FairyLight_p[16] =
{  CRGB::FairyLight, CRGB::FairyLight, CRGB::FairyLight, CRGB::FairyLight, 
  HALFFAIRY,        HALFFAIRY,        CRGB::FairyLight, CRGB::FairyLight, 
  QUARTERFAIRY,     QUARTERFAIRY,     CRGB::FairyLight, CRGB::FairyLight, 
  CRGB::FairyLight, CRGB::FairyLight, CRGB::FairyLight, CRGB::FairyLight };

// A palette of soft snowflakes with the occasional bright one
const CRGB TwinklesMode::Snow_p[16] =
{  0x304048, 0x304048, 0x304048, 0x304048,
  0x304048, 0x304048, 0x304048, 0x304048,
  0x304048, 0x304048, 0x304048, 0x304048,
  0x304048, 0x304048, 0x304048, 0xE0F0FF };

// A palette reminiscent of large 'old-school' C9-size tree lights
// in the five classic colors: red, orange, green, blue, and white.
#define C9_Red    0xB80400
#define C9_Orange 0x902C02
#define C9_Green  0x046002
#define C9_Blue   0x070758
#define C9_White  0x606820
const CRGB TwinklesMode::RetroC9_p[16] =
{  C9_Red,    C9_Orange, C9_Red,    C9_Orange,
  C9_Orange, C9_Red,    C9_Orange, C9_Red,
  C9_Green,  C9_Green,  C9_Green,  C9_Green,
  C9_Blue,   C9_Blue,   C9_Blue,
  C9_White
};

// A cold, icy pale blue palette
#define Ice_Blue1 0x0C1040
#define Ice_Blue2 0x182080
#define Ice_Blue3 0x5080C0
const CRGB TwinklesMode::Ice_p[16] =
{
  Ice_Blue1, Ice_Blue1, Ice_Blue1, Ice_Blue1,
  Ice_Blue1, Ice_Blue1, Ice_Blue1, Ice_Blue1,
  Ice_Blue1, Ice_Blue1, Ice_Blue1, Ice_Blue1,
  Ice_Blue2, Ice_Blue2, Ice_Blue2, Ice_Blue3
};


LEDMode* currentMode;
RunningDotMode runningDotMode;
LightSwitchMode lightSwitchMode;
GradualFillMode gradualFillMode;
WaveformMode waveformMode;
TwinklesMode twinklesMode;

LEDSettingsManager settings;

bool prev_button_state = false;
bool settings_modified = false;

void setup() {
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    settings = LEDSettingsManager();
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(POT_PIN, INPUT);
    pinMode(DIP1, INPUT_PULLUP);
    pinMode(DIP2, INPUT_PULLUP);
    pinMode(DIP3, INPUT_PULLUP);
    pinMode(DIP4, INPUT_PULLUP);
    Serial.begin(9600);
    currentMode = &runningDotMode; // Default mode
}

void loop() {
  //read dip 4 as setting_mode. If the dip is active, settings can be edited.
  bool settings_mode = !digitalRead(DIP4);
  //read dip 1-3 as 3-bit number 0..7
  byte dip_number = 0;
  dip_number |= !digitalRead(DIP1) << 0;
  dip_number |= !digitalRead(DIP2) << 1;
  dip_number |= !digitalRead(DIP3) << 2;
  if(settings_mode){
    //SETTINGS MODE
    //display current setting value
    bool button_value = !digitalRead(BUTTON_PIN);
    if(button_value){
      //write value continuously and display while editing
      //remember if number LEDs was edited (needs to be written to eeprom)
      settings.set_setting(static_cast<LEDSetting>(dip_number), map(analogRead(POT_PIN), 1023, 0, 0, 255));
      settings_modified = true;
    }
    else {
      settings.display_setting(static_cast<LEDSetting>(dip_number), false);
    }
  }
  else  {
    //RUN MODE
    if(settings_modified){
      //if returning from settings, and settings have been modified, write new settings to eeprom
      settings_modified = false;
      settings.write_to_eeprom();
    }
    FastLED.setBrightness(map(analogRead(POT_PIN), 1023, 0, 5, 255)); // Control brightness with potentiometer
    switch (dip_number) {
        case 0: currentMode = &runningDotMode; break;
        case 1: currentMode = &lightSwitchMode; break;
        case 2: currentMode = &gradualFillMode; break;
        case 3: currentMode = &waveformMode; break;
        case 4: currentMode = &twinklesMode; break;
        // Additional cases for other modes
    }
    currentMode->update(settings);    
  }
  FastLED.show();
}
