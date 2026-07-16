#include <Adafruit_NeoPixel.h>

// --- HARDWARE SETTINGS ---
#define NUM_LEDS_PER_MATRIX 64
#define MATRIX_WIDTH        8
#define MATRIX_HEIGHT       8

// Your physical layout creates a 16x24 grid
#define GLOBAL_WIDTH        16
#define GLOBAL_HEIGHT       24
#define BRIGHTNESS          50

// Pin Definitions for ESP32-C3 (Safe Pins)
#define PIN_A 10
#define PIN_B 7
#define PIN_C 3
#define PIN_D 6 
#define PIN_E 4
#define PIN_F 5

// Toggle this to false if your matrices do not use serpentine/zigzag wiring
const bool ZIGZAG_LAYOUT = true; 

// Create an array of 6 Adafruit_NeoPixel objects
Adafruit_NeoPixel matrices[6] = {
  Adafruit_NeoPixel(NUM_LEDS_PER_MATRIX, PIN_A, NEO_GRB + NEO_KHZ800), // Index 0: Matrix A
  Adafruit_NeoPixel(NUM_LEDS_PER_MATRIX, PIN_B, NEO_GRB + NEO_KHZ800), // Index 1: Matrix B
  Adafruit_NeoPixel(NUM_LEDS_PER_MATRIX, PIN_C, NEO_GRB + NEO_KHZ800), // Index 2: Matrix C
  Adafruit_NeoPixel(NUM_LEDS_PER_MATRIX, PIN_D, NEO_GRB + NEO_KHZ800), // Index 3: Matrix D
  Adafruit_NeoPixel(NUM_LEDS_PER_MATRIX, PIN_E, NEO_GRB + NEO_KHZ800), // Index 4: Matrix E
  Adafruit_NeoPixel(NUM_LEDS_PER_MATRIX, PIN_F, NEO_GRB + NEO_KHZ800)  // Index 5: Matrix F
};

// ==========================================
// BITMAP FONTS (8x8 grids for letters A-F)
// 1 bit = LED ON, 0 bit = LED OFF
// ==========================================
const uint8_t font_A[8] = {0x3C, 0x42, 0x42, 0x7E, 0x42, 0x42, 0x42, 0x00};
const uint8_t font_B[8] = {0x7C, 0x42, 0x42, 0x7C, 0x42, 0x42, 0x7C, 0x00};
const uint8_t font_C[8] = {0x3C, 0x42, 0x40, 0x40, 0x40, 0x42, 0x3C, 0x00};
const uint8_t font_D[8] = {0x7C, 0x42, 0x42, 0x42, 0x42, 0x42, 0x7C, 0x00};
const uint8_t font_E[8] = {0x7E, 0x40, 0x40, 0x7C, 0x40, 0x40, 0x7E, 0x00};
const uint8_t font_F[8] = {0x7E, 0x40, 0x40, 0x7C, 0x40, 0x40, 0x40, 0x00};

void setup() {
  Serial.begin(115200);
  
  for(int i = 0; i < 6; i++) {
    matrices[i].begin();
    matrices[i].setBrightness(BRIGHTNESS);
    matrices[i].clear();
    matrices[i].show();
  }
  
  Serial.println("6-Matrix ESP32-C3 Setup Complete!");
}

void loop() {
  clearAll();
  
  // Display the identity matrix!
  identifyMatrices();
  
  showAll();
  
  // Hold the image for 10 seconds so you can inspect your wiring/orientation
  delay(10000); 
}

// ==========================================
// CORE FUSION ENGINE
// ==========================================

void drawPixel(int x, int y, uint32_t color) {
  if (x < 0 || x >= GLOBAL_WIDTH || y < 0 || y >= GLOBAL_HEIGHT) return;

  int matrix_col = x / MATRIX_WIDTH;  
  int matrix_row = y / MATRIX_HEIGHT; 
  int matrix_idx = (matrix_row * 2) + matrix_col;

  int local_x = x % MATRIX_WIDTH;
  int local_y = y % MATRIX_HEIGHT;

  int pixel_idx;
  if (ZIGZAG_LAYOUT) {
    if (local_y % 2 == 0) {
      pixel_idx = (local_y * MATRIX_WIDTH) + local_x;             
    } else {
      pixel_idx = (local_y * MATRIX_WIDTH) + (7 - local_x);       
    }
  } else {
    pixel_idx = (local_y * MATRIX_WIDTH) + local_x;               
  }

  matrices[matrix_idx].setPixelColor(pixel_idx, color);
}

void showAll() {
  for(int i = 0; i < 6; i++) matrices[i].show();
}

void clearAll() {
  for(int i = 0; i < 6; i++) matrices[i].clear();
}

// ==========================================
// IDENTIFICATION & DRAWING FUNCTIONS
// ==========================================

// Helper function to draw an 8x8 bitmap letter at a specific X,Y location
void drawBitmap(int startX, int startY, const uint8_t bitmap[8], uint32_t color) {
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      // Check if the specific bit in the byte is a 1
      if (bitmap[row] & (1 << (7 - col))) {
        drawPixel(startX + col, startY + row, color);
      }
    }
  }
}

// Draws the letters A-F and an orientation dot on each matrix
void identifyMatrices() {
  uint32_t colorRed   = matrices[0].Color(200, 0, 0);
  uint32_t colorGreen = matrices[0].Color(0, 200, 0);
  uint32_t colorBlue  = matrices[0].Color(0, 0, 200);
  uint32_t colorYel   = matrices[0].Color(200, 200, 0);
  uint32_t colorMag   = matrices[0].Color(200, 0, 200);
  uint32_t colorCyan  = matrices[0].Color(0, 200, 200);
  uint32_t colorWhite = matrices[0].Color(255, 255, 255);

  // Matrix A (Top-Left)
  drawBitmap(0, 0, font_A, colorRed);
  drawPixel(0, 0, colorWhite); // Orientation Dot: Top Left of Matrix A

  // Matrix B (Top-Right)
  drawBitmap(8, 0, font_B, colorGreen);
  drawPixel(8, 0, colorWhite); // Orientation Dot: Top Left of Matrix B

  // Matrix C (Middle-Left)
  drawBitmap(0, 8, font_C, colorBlue);
  drawPixel(0, 8, colorWhite); // Orientation Dot: Top Left of Matrix C

  // Matrix D (Middle-Right)
  drawBitmap(8, 8, font_D, colorYel);
  drawPixel(8, 8, colorWhite); // Orientation Dot: Top Left of Matrix D

  // Matrix E (Bottom-Left)
  drawBitmap(0, 16, font_E, colorMag);
  drawPixel(0, 16, colorWhite); // Orientation Dot: Top Left of Matrix E

  // Matrix F (Bottom-Right)
  drawBitmap(8, 16, font_F, colorCyan);
  drawPixel(8, 16, colorWhite); // Orientation Dot: Top Left of Matrix F
}