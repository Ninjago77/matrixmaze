#include <Adafruit_Protomatter.h>

// ==========================================
// 1. PIN CONFIGURATION
// ==========================================
uint8_t rgbPins[]  = {0, 1, 2, 3, 4, 5};  // R1, G1, B1, R2, G2, B2
uint8_t addrPins[] = {7, 11, 6, 12};     // A, B, C, D

uint8_t clockPin = 8;   // CLK
uint8_t oePin    = 9;   // OE
uint8_t latchPin = 13;  // LAT

// ==========================================
// 2. 64x64 CONSTRUCTOR
// ==========================================
Adafruit_Protomatter matrix(
  64,          // Width of a single panel row (64)
  4,           // Bit depth (4 provides 4096 colors)
  1,           // Parallel chains output
  rgbPins,     // RGB data pins array
  4,           // Number of address lines
  addrPins,    // Address pins array
  clockPin, 
  latchPin, 
  oePin, 
  true,        // Double-buffering enabled (prevents tearing)
  2            // Vertical Tiling
);

void setup() {
  Serial.begin(115200);

  // Initialize the matrix first
  ProtomatterStatus status = matrix.begin();
  if (status != PROTOMATTER_OK) {
    for (;;); // Halt on error
  }

  // CORRECT FIX: Adjust pixel clock duty cycle timing for fast microcontrollers.
  // This stretches the clock signal so stubborn matrix shift registers can read it.
  matrix.setDuty(2); 

  // Draw the static graphics ONCE in setup because they don't change
  drawStaticScene();
  matrix.show();

}

void loop() {
  // CRITICAL FIX: Loop runs completely open with no heavy delays.
  // Protomatter relies on background interrupts. Stalling kills the display.
  delay(10); 
}

void drawStaticScene() {
  matrix.fillScreen(matrix.color565(0, 0, 0));
  
  // Red border around the 64x64 square
  matrix.drawRect(0, 0, 64, 64, matrix.color565(255, 0, 0));

  // Green diagonal line
  matrix.drawLine(0, 0, 63, 63, matrix.color565(0, 255, 0));

  // Blue circle in the center
  matrix.drawCircle(32, 32, 12, matrix.color565(0, 0, 255));

  // White text across the seam
  matrix.setTextSize(1);
  matrix.setTextColor(matrix.color565(255, 255, 255));
  matrix.setCursor(14, 28); 
  matrix.print("SQUARE");
  
  matrix.show();
}
