#include <Wire.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// --- PIN DEFINITIONS ---
#define I2C_SDA_PIN 5
#define I2C_SCL_PIN 6
#define BUTTON_PIN  7
#define BMI160_ADDR 0x68

// --- ESP-NOW DATA STRUCTURE ---
typedef struct struct_message {
  float accX;
  float accY;
  float accZ;
  float gyroX;
  float gyroY;
  float gyroZ;
} struct_message;

struct_message myData;
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
esp_now_peer_info_t peerInfo;

// V3 ESP-NOW Callback
void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
  // We keep this empty so it doesn't spam the serial monitor at 50fps
}

// --- BMI160 HARDWARE FUNCTIONS ---
void writeRegister(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(BMI160_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

bool readRegisters(uint8_t reg, uint8_t *buffer, uint8_t length) {
  Wire.beginTransmission(BMI160_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false; 
  if (Wire.requestFrom((uint16_t)BMI160_ADDR, (uint8_t)length) != length) return false;
  for (uint8_t i = 0; i < length; i++) buffer[i] = Wire.read();
  return true;
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // 1. Initialize BMI160
  Wire.setPins(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.begin();
  delay(100);

  // Wake up BMI160
  writeRegister(0x7E, 0xB6); delay(15); // Soft Reset
  writeRegister(0x7E, 0x11); delay(15); // Accel Normal
  writeRegister(0x7E, 0x15); delay(100); // Gyro Normal

  // 2. Initialize ESP-NOW
  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_register_send_cb(OnDataSent);
  
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
}

void loop() {
  uint8_t dataBuffer[12];
  
  if (readRegisters(0x0C, dataBuffer, 12)) {
    // Process Raw Data
    int16_t rawGyroX  = (int16_t)(dataBuffer[0]  | (dataBuffer[1]  << 8));
    int16_t rawGyroY  = (int16_t)(dataBuffer[2]  | (dataBuffer[3]  << 8));
    int16_t rawGyroZ  = (int16_t)(dataBuffer[4]  | (dataBuffer[5]  << 8));
    int16_t rawAccelX = (int16_t)(dataBuffer[6]  | (dataBuffer[7]  << 8));
    int16_t rawAccelY = (int16_t)(dataBuffer[8]  | (dataBuffer[9]  << 8));
    int16_t rawAccelZ = (int16_t)(dataBuffer[10] | (dataBuffer[11] << 8));

    // Convert and Load into Struct
    myData.accX = rawAccelX / 16384.0;
    myData.accY = rawAccelY / 16384.0;
    myData.accZ = rawAccelZ / 16384.0;
    myData.gyroX = rawGyroX / 16.4;
    myData.gyroY = rawGyroY / 16.4;
    myData.gyroZ = rawGyroZ / 16.4;

    // Broadcast Data
    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
  }

  // Run at roughly 50 fps for smooth physics
  delay(20); 
}