#include <Wire.h>

#define I2C_SDA_PIN 5
#define I2C_SCL_PIN 6
#define BUTTON_PIN  7   // Button connected to GPIO 7 and GND
#define BMI160_ADDR 0x68 

// Track the previous state of the button to detect the exact moment it is pressed
bool lastButtonState = HIGH; 

void writeRegister(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(BMI160_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

bool readRegisters(uint8_t reg, uint8_t *buffer, uint8_t length) {
  Wire.beginTransmission(BMI160_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return false; 
  }
  uint8_t bytesRead = Wire.requestFrom(BMI160_ADDR, length);
  if (bytesRead != length) {
    return false; 
  }
  for (uint8_t i = 0; i < length; i++) {
    buffer[i] = Wire.read();
  }
  return true;
}

// Function to handle the actual physical reset of the BMI160
void resetBMI160() {
  Serial.println("\n====================================");
  Serial.println("🔄 RESET BUTTON PRESSED! Resetting BMI160...");
  Serial.println("====================================");
  
  writeRegister(0x7E, 0xB6); // Send Soft Reset Command
  delay(15);                 // Wait for the chip to reboot
  
  writeRegister(0x7E, 0x11); // Turn Accel back to Normal Mode
  delay(15);
  
  writeRegister(0x7E, 0x15); // Turn Gyro back to Normal Mode
  delay(100);                // Wait for the sensors to stabilize
  
  Serial.println("Sensor re-initialization complete!\n");
}

void setup() {
  Serial.begin(115200);
  delay(3000); 
  Serial.println("\n--- BMI160 Test with Reset Button ---");

  // Configure the button pin with the internal pull-up resistor
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Initialize custom I2C pins
  Wire.setPins(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.begin();

  // Verify the sensor is connected
  uint8_t chipID = 0;
  if (!readRegisters(0x00, &chipID, 1)) {
    Serial.println("ERROR: Could not communicate with the sensor.");
    while (1) delay(100);
  }

  Serial.print("Sensor Found! Chip ID: 0x");
  Serial.println(chipID, HEX);

  // Initial power-up sequence
  resetBMI160();
}

void loop() {
  // 1. Read the physical button state
  bool currentButtonState = digitalRead(BUTTON_PIN);

  // Detect transition from HIGH (not pressed) to LOW (pressed)
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    resetBMI160(); 
    delay(200); // Simple debounce delay to prevent double-triggering
  }
  lastButtonState = currentButtonState;

  // 2. Read and print the sensor data
  uint8_t dataBuffer[12];
  if (readRegisters(0x0C, dataBuffer, 12)) {
    int16_t rawGyroX  = (int16_t)(dataBuffer[0]  | (dataBuffer[1]  << 8));
    int16_t rawGyroY  = (int16_t)(dataBuffer[2]  | (dataBuffer[3]  << 8));
    int16_t rawGyroZ  = (int16_t)(dataBuffer[4]  | (dataBuffer[5]  << 8));
    int16_t rawAccelX = (int16_t)(dataBuffer[6]  | (dataBuffer[7]  << 8));
    int16_t rawAccelY = (int16_t)(dataBuffer[8]  | (dataBuffer[9]  << 8));
    int16_t rawAccelZ = (int16_t)(dataBuffer[10] | (dataBuffer[11] << 8));

    float accX = rawAccelX / 16384.0;
    float accY = rawAccelY / 16384.0;
    float accZ = rawAccelZ / 16384.0;

    float gyroX = rawGyroX / 16.4;
    float gyroY = rawGyroY / 16.4;
    float gyroZ = rawGyroZ / 16.4;

    Serial.printf("Accel [X: %5.2fG, Y: %5.2fG, Z: %5.2fG] | Gyro [X: %7.1f dps, Y: %7.1f dps, Z: %7.1f dps]\n", 
                  accX, accY, accZ, gyroX, gyroY, gyroZ);
  } else {
    Serial.println("Failed to read data registers. Try pressing the reset button!");
  }

  delay(100); 
}