//Yueyang Cheng(1533989)
//lab#5 part 4
// this code is designed to sweep the sine wave in different frequency
#include <application.h>
#include <spark_wiring_i2c.h>

// LIS3L02DQ I2C address is 29
#define Addr 29

int xAccl = 0, yAccl =  0, zAccl = 0;
void setup()
{

  // Initialise I2C communication as MASTER
  Wire.begin();
  // Initialize serial communication, set baud rate = 9600
  Serial.begin(9600);

  // Start I2C Transmission
  Wire.beginTransmission(Addr);
  // Select control register 1
  Wire.write(0x20);
  // Enable X, Y, Z axis, power on mode, data output rate 50Hz
  Wire.write(0x27);
  // Stop I2C Transmission
  Wire.endTransmission();

  delay(300);
}

void loop()
{
  unsigned int data[6];
  for (int i = 0; i < 6; i++)
  {
    // Start I2C Transmission
    Wire.beginTransmission(Addr);
    // Select data register
    Wire.write((40 + i));
    // Stop I2C Transmission
    Wire.endTransmission();

    // Request 1 byte of data
    Wire.requestFrom(Addr, 1);

    // Read 6 bytes of data
    // xAccl lsb, xAccl msb, yAccl lsb, yAccl msb, zAccl lsb, zAccl msb
    if (Wire.available() == 1)
    {
      data[i] = Wire.read();
    }
    delay(300);
  }
}
