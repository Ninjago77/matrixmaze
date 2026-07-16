#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <ESP32-VirtualMatrixPanel-I2S-DMA.h>

// ==========================================
// 1. HARDWARE PINOUT
// ==========================================
#define R1_PIN  1
#define G1_PIN  2
#define B1_PIN  3
#define R2_PIN  4
#define G2_PIN  5
#define B2_PIN  6
#define A_PIN   7
#define B_PIN   8
#define C_PIN   9
#define D_PIN   10
#define E_PIN   -1 
#define LAT_PIN 12
#define OE_PIN  13
#define CLK_PIN 14

// ==========================================
// 2. PANEL CONFIGURATION
// ==========================================
#define PANEL_RES_X 64 // Width of ONE panel
#define PANEL_RES_Y 32 // Height of ONE panel
#define NUM_PANELS  2  // 2 panels total in the chain

// Virtual Matrix: 2 panels tall, 1 panel wide
#define VIRTUAL_ROWS 2 
#define VIRTUAL_COLS 1 

MatrixPanel_I2S_DMA *dma_display = nullptr;
VirtualMatrixPanel  *virtualDisp = nullptr;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Configure DMA Matrix
  HUB75_I2S_CFG mxconfig(PANEL_RES_X, PANEL_RES_Y, NUM_PANELS);
  
  mxconfig.gpio.r1  = R1_PIN;
  mxconfig.gpio.g1  = G1_PIN;
  mxconfig.gpio.b1  = B1_PIN;
  mxconfig.gpio.r2  = R2_PIN;
  mxconfig.gpio.g2  = G2_PIN;
  mxconfig.gpio.b2  = B2_PIN;
  mxconfig.gpio.a   = A_PIN;
  mxconfig.gpio.b   = B_PIN;
  mxconfig.gpio.c   = C_PIN;
  mxconfig.gpio.d   = D_PIN;
  mxconfig.gpio.e   = E_PIN;
  mxconfig.gpio.lat = LAT_PIN;
  mxconfig.gpio.oe  = OE_PIN;
  mxconfig.gpio.clk = CLK_PIN;
  
  mxconfig.clkphase = false;
  
  // Slow down the clock from 20MHz to 10MHz (Highly recommended for breadboards)
  mxconfig.i2sspeed = HUB75_I2S_CFG::HZ_10M;
  
  // Give the panel a tiny bit more time to switch rows before pushing color
  mxconfig.latch_blanking = 4;

  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
  dma_display->setBrightness8(90); // 0-255

  // Initialize Virtual Display (Top-to-Bottom chaining)
  virtualDisp = new VirtualMatrixPanel((*dma_display), VIRTUAL_ROWS, VIRTUAL_COLS, PANEL_RES_X, PANEL_RES_Y, CHAIN_TOP_LEFT_DOWN);
}

void loop() {
  // --- TEST 1: Identify Panels ---
  virtualDisp->fillScreen(virtualDisp->color565(0, 0, 0)); // Clear to Black
  
  // Fill Top Panel (Panel 1) with Dark Red
  virtualDisp->fillRect(0, 0, 64, 32, virtualDisp->color565(100, 0, 0));
  
  // Fill Bottom Panel (Panel 2) with Dark Blue
  virtualDisp->fillRect(0, 32, 64, 32, virtualDisp->color565(0, 0, 100));

  // Write Text to Top Panel
  virtualDisp->setTextSize(1);
  virtualDisp->setTextColor(virtualDisp->color565(255, 255, 255)); // White Text
  virtualDisp->setCursor(8, 12);
  virtualDisp->print("TOP PANEL");

  // Write Text to Bottom Panel
  virtualDisp->setTextColor(virtualDisp->color565(255, 255, 0)); // Yellow Text
  virtualDisp->setCursor(2, 44);
  virtualDisp->print("BOTTOM PANEL");

  delay(2000); // Hold for 2 seconds so you can read it

  // --- TEST 2: The Scanning Line (Tests seamless boundary) ---
  virtualDisp->fillScreen(virtualDisp->color565(0, 0, 0));
  
  virtualDisp->setCursor(16, 28);
  virtualDisp->setTextColor(virtualDisp->color565(0, 255, 255));
  virtualDisp->print("SCANNING");

  // Draw a horizontal green line that sweeps from Y=0 to Y=63
  for (int y = 0; y < 64; y++) {
    virtualDisp->drawLine(0, y, 63, y, virtualDisp->color565(0, 255, 0));
    delay(30);
    // Erase the line behind it so it looks like it's moving
    virtualDisp->drawLine(0, y, 63, y, virtualDisp->color565(0, 0, 0));
  }
}