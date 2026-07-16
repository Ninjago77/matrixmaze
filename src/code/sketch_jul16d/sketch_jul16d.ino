#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Adafruit_NeoPixel.h>

// --- MATRIX SETTINGS ---
#define NUM_LEDS_PER_MATRIX 64
#define MATRIX_WIDTH 8
#define MATRIX_HEIGHT 8
#define GLOBAL_WIDTH 16
#define GLOBAL_HEIGHT 24
#define BRIGHTNESS 60
const bool ZIGZAG_LAYOUT = true;

// Safe ESP32-C3 Pins
#define PIN_A 10
#define PIN_B 7
#define PIN_C 3
#define PIN_D 6 
#define PIN_E 4
#define PIN_F 5

Adafruit_NeoPixel matrices[6] = {
  Adafruit_NeoPixel(NUM_LEDS_PER_MATRIX, PIN_A, NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(NUM_LEDS_PER_MATRIX, PIN_B, NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(NUM_LEDS_PER_MATRIX, PIN_C, NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(NUM_LEDS_PER_MATRIX, PIN_D, NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(NUM_LEDS_PER_MATRIX, PIN_E, NEO_GRB + NEO_KHZ800),
  Adafruit_NeoPixel(NUM_LEDS_PER_MATRIX, PIN_F, NEO_GRB + NEO_KHZ800)
};

// --- DATA FROM CONTROLLER ---
volatile float c_accX = 0, c_accY = 0, c_accZ = 0;
volatile float c_gyroX = 0, c_gyroY = 0, c_gyroZ = 0;

typedef struct struct_message {
  float accX; float accY; float accZ;
  float gyroX; float gyroY; float gyroZ;
} struct_message;

// V3 ESP-NOW Receive Callback
void OnDataRecv(const esp_now_recv_info_t * esp_now_info, const uint8_t *incomingData, int len) {
  struct_message myData;
  memcpy(&myData, incomingData, sizeof(myData));
  
  // Depending on how you hold the controller, you might need to swap X and Y here.
  // We invert them here if needed to make "tilt right = move right"
  c_accX = myData.accX; 
  c_accY = myData.accY; 
  c_gyroZ = myData.gyroZ;
}

// --- PHYSICS VARIABLES ---
float ballX = 8.0;
float ballY = 12.0;
float velX = 0.0;
float velY = 0.0;

void setup() {
  Serial.begin(115200);

  // Init Matrices
  for (int i = 0; i < 6; i++) {
    matrices[i].begin();
    matrices[i].setBrightness(BRIGHTNESS);
    matrices[i].clear();
    matrices[i].show();
  }

  // Init ESP-NOW
  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  // 1. UPDATE PHYSICS
  // Apply acceleration (tilt) to velocity
  velX += c_accX * 0.15; // Sensitivity X
  velY += c_accY * 0.15; // Sensitivity Y

  // Apply friction (air drag)
  velX *= 0.94;
  velY *= 0.94;

  // Move Ball
  ballX += velX;
  ballY += velY;

  // Wall Collisions (Bounce)
  if (ballX < 0) { ballX = 0; velX = -velX * 0.6; }
  if (ballX > GLOBAL_WIDTH - 1) { ballX = GLOBAL_WIDTH - 1; velX = -velX * 0.6; }
  if (ballY < 0) { ballY = 0; velY = -velY * 0.6; }
  if (ballY > GLOBAL_HEIGHT - 1) { ballY = GLOBAL_HEIGHT - 1; velY = -velY * 0.6; }

  // 2. FADE SCREEN (Creates motion blur/trails instead of hard clearing)
  fadeTrails();

  // 3. DRAW 3D PARALLAX FRAME
  draw3DFrame();

  // 4. DRAW BALL
  drawBall();

  // 5. RENDER
  showAll();

  delay(20); // 50 Frames per second
}


// ==========================================
// RENDERING & FUSION ENGINE
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
    if (local_y % 2 == 0) pixel_idx = (local_y * MATRIX_WIDTH) + local_x;
    else pixel_idx = (local_y * MATRIX_WIDTH) + (7 - local_x);
  } else {
    pixel_idx = (local_y * MATRIX_WIDTH) + local_x;
  }
  matrices[matrix_idx].setPixelColor(pixel_idx, color);
}

void showAll() {
  for (int i = 0; i < 6; i++) matrices[i].show();
}

// Lowers the brightness of every pixel by 25% to create comet trails
void fadeTrails() {
  for (int m = 0; m < 6; m++) {
    for (int i = 0; i < NUM_LEDS_PER_MATRIX; i++) {
      uint32_t c = matrices[m].getPixelColor(i);
      uint8_t r = (c >> 16) * 0.75;
      uint8_t g = (c >> 8)  * 0.75;
      uint8_t b = (c)       * 0.75;
      matrices[m].setPixelColor(i, matrices[m].Color(r, g, b));
    }
  }
}

// Bresenham's Line Algorithm
void drawLine(int x0, int y0, int x1, int y1, uint32_t color) {
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy, e2;
  while (true) {
    drawPixel(x0, y0, color);
    if (x0 == x1 && y0 == y1) break;
    e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

void draw3DFrame() {
  // Map Frame color to YAW (Twisting the controller changes the tunnel color)
  // Hue is 0 to 65535.
  uint16_t frameHue = abs(c_gyroZ) * 200; 
  uint32_t frameColor = matrices[0].ColorHSV(frameHue, 255, 120); // 120 is dim brightness

  // Calculate inner box parallax offsets (Opposite of tilt)
  int offsetX = constrain((int)(c_accX * -6.0), -5, 5); 
  int offsetY = constrain((int)(c_accY * -8.0), -6, 6);

  // Inner Box Corners
  int ix0 = 4 + offsetX, iy0 = 8 + offsetY;   // Top-Left
  int ix1 = 11 + offsetX, iy1 = 8 + offsetY;  // Top-Right
  int ix2 = 11 + offsetX, iy2 = 15 + offsetY; // Bottom-Right
  int ix3 = 4 + offsetX, iy3 = 15 + offsetY;  // Bottom-Left

  // Draw Inner Box
  drawLine(ix0, iy0, ix1, iy1, frameColor);
  drawLine(ix1, iy1, ix2, iy2, frameColor);
  drawLine(ix2, iy2, ix3, iy3, frameColor);
  drawLine(ix3, iy3, ix0, iy0, frameColor);

  // Draw connecting 3D corners to outer edge of matrix
  drawLine(0, 0, ix0, iy0, frameColor);                           // Top Left
  drawLine(GLOBAL_WIDTH - 1, 0, ix1, iy1, frameColor);            // Top Right
  drawLine(GLOBAL_WIDTH - 1, GLOBAL_HEIGHT - 1, ix2, iy2, frameColor); // Bottom Right
  drawLine(0, GLOBAL_HEIGHT - 1, ix3, iy3, frameColor);           // Bottom Left
}

void drawBall() {
  // Calculate ball speed
  float speed = sqrt((velX * velX) + (velY * velY));
  
  // Map speed to color (Fast = Red [0], Slow = Blue/Cyan [40000])
  uint16_t ballHue = map(constrain(speed * 100, 0, 150), 0, 150, 40000, 0);
  uint32_t ballColor = matrices[0].ColorHSV(ballHue, 255, 255); // Max brightness

  // Draw a 2x2 cluster so the ball is nice and thick
  int bx = (int)ballX;
  int by = (int)ballY;
  
  drawPixel(bx, by, ballColor);
  drawPixel(bx + 1, by, ballColor);
  drawPixel(bx, by + 1, ballColor);
  drawPixel(bx + 1, by + 1, ballColor);
}