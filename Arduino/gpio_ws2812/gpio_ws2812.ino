#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <functional>

uint i = 0;

#define LED_PIN     25
#define LED_COUNT   1
#define MIN_BRIGHTNESS 30
#define MAX_BRIGHTNESS 255
#define FADE_STEPS 100         // Brightness steps for fading effect
#define FADE_DELAY 20          // Delay per step (ms), total time = FADE_STEPS * FADE_DELAY = 1000ms

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// Define five colors (brightness adjusted later)
const uint32_t baseColors[5] = {
  strip.Color(255, 0, 0),    // Red
  strip.Color(0, 255, 0),    // Green
  strip.Color(0, 0, 255),    // Blue
  strip.Color(255, 255, 255), // White
  strip.Color(0, 0, 0)       // Black
};

const char* colorNames[5] = {
  "red", "green", "blue", "white", "black"
};

uint8_t currentColorIndex = 0;  // Current color index
uint8_t brightness = MIN_BRIGHTNESS;  // Current brightness
uint8_t fadeStep = 0;  // Current fade step

uint8_t gpio[16] = {0,1,2,3,4,5,6,7,8,9,10,11,22,26,27,28};
uint8_t saveColorIndex = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);
    delay(1000);
    Serial.println("GPIO initialization");
    for(i = 0; i < 16; i++) {
        gpio_init(gpio[i]);
        gpio_set_dir(gpio[i], GPIO_OUT);
    }
    strip.begin();
    strip.clear();
    strip.show();
}

void loop() {
    // Display five colors in sequence
    for (uint8_t colorIndex = 0; colorIndex < 5; colorIndex++) {
        displayColor(colorIndex);
    }
    Serial.println("Color display complete, entering GPIO test mode");
    delay(1000);
    
    while (true) {
      Serial.println("GPIO HIGH");
      for(i = 0; i < 16; i++) {
          gpio_put(gpio[i], 1);
      }
      delay(2000);
      
      Serial.println("GPIO LOW");  
      for(i = 0; i < 16; i++) {
          gpio_put(gpio[i], 0);
      }
      delay(2000);
    }
}

// Display specified color (with fading effect, black is immediate)
void displayColor(uint8_t colorIndex) {
    Serial.print("Displaying color: ");
    Serial.println(colorNames[colorIndex]);
    
    if (colorIndex == 4) { // Black (turn off LED immediately)
        strip.clear();
        strip.show();
        delay(1000); // Hold black for 1 second
        return;
    }
    
    // For other colors, perform fading effect
    for (uint8_t step = 0; step <= FADE_STEPS; step++) {
        uint8_t brightness = map(step, 0, FADE_STEPS, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
        uint32_t currentColor = scaleColor(baseColors[colorIndex], brightness);
        
        for(int i = 0; i < strip.numPixels(); i++) {
            strip.setPixelColor(i, currentColor);
        }
        strip.show();
        delay(FADE_DELAY);
    }
}

// Helper function: Scale color brightness
uint32_t scaleColor(uint32_t color, uint8_t brightness) {
  uint8_t r = (color >> 16) & 0xFF;
  uint8_t g = (color >> 8) & 0xFF;
  uint8_t b = color & 0xFF;
  
  r = (r * brightness) / 255;
  g = (g * brightness) / 255;
  b = (b * brightness) / 255;
  
  return strip.Color(r, g, b);
}
