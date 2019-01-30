// Yueyang Cheng, Yingge He
// group #24
// final project
/* This project is a real-time heart monitor which can detect abnormal rate
of heart contraction. System will records 30s data of patient's heart beat, and 
analyzes the data during the process. At the end, screen will shows a report page including 
Arrhythmia details. Bluetooth is applied in data transmission. The idea of this project is 
to achieve wireless clinical diagnosis. 
*/
#define PDB_CH0C1_TOS 0x0100
#define PDB_CH0C1_EN 0x01
#include <SD.h>
#define TFT_DC  9
#define TFT_CS 10
#define CS_PIN  4
#define sample_size  7500

#include <Arduino.h>
#include "SPI.h"

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"

#include "BluefruitConfig.h"

#if SOFTWARE_SERIAL_AVAILABLE
#include <SoftwareSerial.h>
#endif
// Create the bluefruit object, either software serial...uncomment these lines
/*
SoftwareSerial bluefruitSS = SoftwareSerial(BLUEFRUIT_SWUART_TXD_PIN, BLUEFRUIT_SWUART_RXD_PIN);

Adafruit_BluefruitLE_UART ble(bluefruitSS, BLUEFRUIT_UART_MODE_PIN,
                      BLUEFRUIT_UART_CTS_PIN, BLUEFRUIT_UART_RTS_PIN);
*/

/* ...or hardware serial, which does not need the RTS/CTS pins. Uncomment this line */
// Adafruit_BluefruitLE_UART ble(BLUEFRUIT_HWSERIAL_NAME, BLUEFRUIT_UART_MODE_PIN);

/* ...hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST */
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

/* ...software SPI, using SCK/MOSI/MISO user-defined SPI pins and then user selected CS/IRQ/RST */
//Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_SCK, BLUEFRUIT_SPI_MISO,
//                             BLUEFRUIT_SPI_MOSI, BLUEFRUIT_SPI_CS,
//                             BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);


// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

/* The service information */

int32_t hrmServiceId;
int32_t hrmMeasureCharId;
int32_t hrmLocationCharId;
/**************************************************************************/
/*!
    @brief  Sets up the HW an the BLE module (this function is called
            automatically on startup)
*/
/**************************************************************************/

#include "ILI9341_t3.h"
#include <font_Arial.h> // from ILI9341_t3
#include <XPT2046_Touchscreen.h>
XPT2046_Touchscreen ts(CS_PIN);
#define TIRQ_PIN  2

ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC);
volatile int counter;
volatile bool START;

volatile int start_point = 0;
uint16_t samples[sample_size+1500];

uint16_t arr[sample_size+1500];
uint8_t mapped[sample_size];
int bigcounter = -1;

const int numReadings = 30;
volatile int avg_BPM;
volatile int avg_QRS;
volatile int avg_RR;
volatile bool Bradycardia = false;
volatile bool Tachycardia = false;
volatile bool PVC = false;
volatile bool PAC = false;
volatile int PAC_counter = 0;

int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int average = 0;                // the average

int inputPin = A0;
int counter_100;
void setup() {
  Serial.begin(9600);
  while (!Serial); // wait until the serial debug window is opened
/**************************************************************************/
 //Bluetooth setup
  boolean success;
  
  Serial.println(F("Adafruit Bluefruit Heart Rate Monitor (HRM) Example"));
  Serial.println(F("---------------------------------------------------"));

  randomSeed(micros());

  /* Initialise the module */
  Serial.print(F("Initialising the Bluefruit LE module: "));

  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println( F("OK!") );

  /* Perform a factory reset to make sure everything is in a known state */
  Serial.println(F("Performing a factory reset: "));
  if (! ble.factoryReset() ){
       error(F("Couldn't factory reset"));
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);

  Serial.println("Requesting Bluefruit info:");
  /* Print Bluefruit information */
  ble.info();

  // this line is particularly required for Flora, but is a good idea
  // anyways for the super long lines ahead!
  // ble.setInterCharWriteDelay(5); // 5 ms

  /* Change the device name to make it easier to find */
  Serial.println(F("Setting device name to 'CYY HRM': "));

  if (! ble.sendCommandCheckOK(F("AT+GAPDEVNAME=CYY HRM")) ) {
    error(F("Could not set device name?"));
  }

  /* Add the Heart Rate Service definition */
  /* Service ID should be 1 */
  Serial.println(F("Adding the Heart Rate Service definition (UUID = 0x180D): "));
  success = ble.sendCommandWithIntReply( F("AT+GATTADDSERVICE=UUID=0x180D"), &hrmServiceId);
  if (! success) {
    error(F("Could not add HRM service"));
  }

  /* Add the Heart Rate Measurement characteristic */
  /* Chars ID for Measurement should be 1 */
  Serial.println(F("Adding the Heart Rate Measurement characteristic (UUID = 0x2A37): "));
  success = ble.sendCommandWithIntReply( F("AT+GATTADDCHAR=UUID=0x2A37, PROPERTIES=0x10, MIN_LEN=2, MAX_LEN=3, VALUE=00-40"), &hrmMeasureCharId);
    if (! success) {
    error(F("Could not add HRM characteristic"));
  }

  /* Add the Body Sensor Location characteristic */
  /* Chars ID for Body should be 2 */
  Serial.println(F("Adding the Body Sensor Location characteristic (UUID = 0x2A38): "));
  success = ble.sendCommandWithIntReply( F("AT+GATTADDCHAR=UUID=0x2A38, PROPERTIES=0x02, MIN_LEN=1, VALUE=3"), &hrmLocationCharId);
    if (! success) {
    error(F("Could not add BSL characteristic"));
  }

  /* Add the Heart Rate Service to the advertising data (needed for Nordic apps to detect the service) */
  Serial.print(F("Adding Heart Rate Service UUID to the advertising payload: "));
  ble.sendCommandCheckOK( F("AT+GAPSETADVDATA=02-01-06-05-02-0d-18-0a-18") );

  /* Reset the device for the new service setting changes to take effect */
  Serial.print(F("Performing a SW reset (service changes require a reset): "));
  ble.reset();

  Serial.println();
/**************************************************************************/

  adcInit();
  pdbInit();
  dmaInit();
  START = false;
  avg_BPM = 75;
  avg_QRS = 60;
  avg_RR = 700;
  counter_100 = 0;
  counter = 0;
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
  display_UI("calibrate");
}

// this methods draw the grid in background setup, only operate during initializing 
void drawSmallGrid() {
  //vertical lines
  for(int i = 0; i < 31; i++) {
    tft.drawLine(14+10*i, 20, 14+10*i, 139, ILI9341_WHITE);
    if (i % 5 == 0){
       tft.drawLine(14+10*i - 1, 20, 10*i+14-1, 139, ILI9341_WHITE);
    }
  }
  //horizontal lines
  for (int i = 2; i < 15; i++) {
    tft.drawLine(5, 10*i , 314, 10*i, ILI9341_WHITE);
    if (i % 5 == 3){
       tft.drawLine(5, 10*i-1, 314, 10*i-1, ILI9341_WHITE);
    } 
  }
}

// this methods draw the grid in review ECG wave page
void drawGrid(int color) {
  //vertical lines
  for(int i = 0; i < 32; i++) {
    tft.drawLine(0+10*i, 40, 0+10*i, 239, color);
    if (i % 5 == 1){
       tft.drawLine(10*i-1, 40, 10*i-1, 239, color);
    }
  }
  //horizontal lines
  for (int i = 4; i < 24; i++) {
    tft.drawLine(0, 0+10*i, 319, 0+10*i, color);
    if (i % 5 == 4){
       tft.drawLine(0, 10*i-1, 319, 10*i-1, color);
    } 
  }
}

void loop() {
  unsigned long start_time = micros();
 
  // calculate the variance, if the variance is samll, which means the
  // data is stabilized
  if (!START) {
    noInterrupts();
    float var = 0;
    if(bigcounter > 100) {
      var = calVar();
    }
    interrupts();
    if ((var < 6500 && var > 2000) || bigcounter >= 1500) {
      START = true;
      start_point = bigcounter;
    }
    display_UI("calibrate");
  }
  
  if (counter == 0 && START){
    display_UI("background");
  }

  drawECG();
  
  //after 30s, system finished collecting data, and enter menu page
  if (bigcounter >= (sample_size+start_point)){
    display_UI("menu");    
  }

  // determine Arrhythmia  
  if (avg_BPM < 60) {
    Bradycardia = true; 
  } else if (avg_BPM > 100) {
    Tachycardia = true;
  } else {
    if (avg_QRS > 120) {
      PVC = true;
    }
  }
  if (PAC_counter > 10){
    PAC = true;
  }
  
  // make sure systems meet the frequency requirement (4ms)
  unsigned long end_time = micros();
  while (end_time - start_time < 4000) {
    end_time = micros();
  }
}

int previous = 140;

// use collected data to draw ECG wave 
void drawECG() {
  // if time less than 30s, keep getting data and draw
  if (bigcounter < (sample_size+start_point)) {
    int val1 = (int)filterloop(samples[bigcounter]);
    samples[bigcounter]  = val1;
    //derivative filter
    float val2 = (1/8.0)*(samples[bigcounter]+2*samples[bigcounter-1]-2*samples[bigcounter-3]-samples[bigcounter-4]);
    arr[bigcounter] = (int)val2;
  
    //squaring
    arr[bigcounter] *= arr[bigcounter];
  
    // moving-window integration
    // subtract the last reading:
    total = total - readings[readIndex];
    // read from the sensor:
    readings[readIndex] = arr[bigcounter];
    // add the reading to the total:
    total = total + readings[readIndex];
    // advance to the next position in the array:
    readIndex = readIndex + 1;
  
    // if we're at the end of the array...
    if (readIndex >= numReadings) {
      // ...wrap around to the beginning:
      readIndex = 0;
    }
  
   // calculate the average:
    average = total / numReadings;
   // send it to the computer as ASCII digits
    arr[bigcounter] = average;
  
    if (START) {
      peakDetect();
      int current;
      current = map(samples[bigcounter],0,4096,21,139);
      
      // save data for review
      int temp = map(samples[bigcounter],0,4096,40,239);
      mapped[counter] = temp;
      
      //make drawing process more smooth withouot keep flashing screen
      tft.drawLine(5+(counter+1)%310, 20, 5+(counter+1)%310, 139, ILI9341_DARKGREY);
      //horizontal lines
      for(int i = 2; i < 14; i++) {
        tft.drawPixel(5+(counter+1)%310, 10*i, ILI9341_WHITE);
        if (i % 5 == 3){
           tft.drawPixel(5+(counter+1)%310, 10*i-1, ILI9341_WHITE);
        }
      }
      //vertical lines
      if ((counter+1)%310%10 == 9 || (5+(counter+1)%310)%50 == 13){
        tft.drawLine(5+(counter+1)%310, 20, 5+(counter+1)%310, 139, ILI9341_WHITE);
      }
      if (counter == 0 || counter % 310 == 309) {
        tft.drawPixel(5+counter % 310, current, ILI9341_GREEN);
        tft.drawPixel(5+counter % 310, current - 1, ILI9341_GREEN);
      } else {
        tft.drawLine(5+counter%310, previous - 1, 5+(counter+1)%310, current - 1, ILI9341_GREEN);
        tft.drawLine(5+counter%310, previous, 5+(counter+1)%310, current, ILI9341_GREEN);
      }
      counter++;
      
      display_UI("countdown");
      
      previous = current;
    }
  }
}

static const uint8_t channel2sc1a[] = {
  5, 14, 8, 9, 13, 12, 6, 7, 15, 4,
  0, 19, 3, 21, 26, 22
};

/*
  ADC_CFG1_ADIV(2)         Divide ratio = 4 (F_BUS = 48 MHz => ADCK = 12 MHz)
  ADC_CFG1_MODE(2)         Single ended 10 bit mode
  ADC_CFG1_ADLSMP          Long sample time
*/
#define ADC_CONFIG1 (ADC_CFG1_ADIV(1) | ADC_CFG1_MODE(1) | ADC_CFG1_ADLSMP)

/*
  ADC_CFG2_MUXSEL          Select channels ADxxb
  ADC_CFG2_ADLSTS(3)       Shortest long sample time
*/
#define ADC_CONFIG2 (ADC_CFG2_MUXSEL | ADC_CFG2_ADLSTS(3))

void adcInit() {
  ADC0_CFG1 = ADC_CONFIG1;
  ADC0_CFG2 = ADC_CONFIG2;
  // Voltage ref vcc, hardware trigger, DMA
  ADC0_SC2 = ADC_SC2_REFSEL(0) | ADC_SC2_ADTRG | ADC_SC2_DMAEN;

  // Enable averaging, 4 samples
  ADC0_SC3 = ADC_SC3_AVGE | ADC_SC3_AVGS(0);

  adcCalibrate();
  Serial.println("calibrated");

  // Enable ADC interrupt, configure pin
  ADC0_SC1A = ADC_SC1_AIEN | channel2sc1a[0];
  NVIC_ENABLE_IRQ(IRQ_ADC0);
}

void adcCalibrate() {
  uint16_t sum;

  // Begin calibration
  ADC0_SC3 = ADC_SC3_CAL;
  // Wait for calibration
  while (ADC0_SC3 & ADC_SC3_CAL);

  // Plus side gain
  sum = ADC0_CLPS + ADC0_CLP4 + ADC0_CLP3 + ADC0_CLP2 + ADC0_CLP1 + ADC0_CLP0;
  sum = (sum / 2) | 0x8000;
  ADC0_PG = sum;

  // Minus side gain (not used in single-ended mode)
  sum = ADC0_CLMS + ADC0_CLM4 + ADC0_CLM3 + ADC0_CLM2 + ADC0_CLM1 + ADC0_CLM0;
  sum = (sum / 2) | 0x8000;
  ADC0_MG = sum;
}

/*
  PDB_SC_TRGSEL(15)        Select software trigger
  PDB_SC_PDBEN             PDB enable
  PDB_SC_PDBIE             Interrupt enable
  PDB_SC_CONT              Continuous mode
  PDB_SC_PRESCALER(7)      Prescaler = 128
  PDB_SC_MULT(1)           Prescaler multiplication factor = 10
*/
#define PDB_CONFIG (PDB_SC_TRGSEL(15) | PDB_SC_PDBEN | PDB_SC_PDBIE \
  | PDB_SC_CONT | PDB_SC_PRESCALER(7) | PDB_SC_MULT(1))

// 48 MHz / 128 / 10 / 1 Hz = 37500
#define PDB_PERIOD (F_BUS / 128 / 10 / 250)

void pdbInit() {
  pinMode(13, OUTPUT);
  tft.begin();
  ts.begin();
  
  // Enable PDB clock
  SIM_SCGC6 |= SIM_SCGC6_PDB;
  // Timer period
  PDB0_MOD = PDB_PERIOD;
  // Interrupt delay
  PDB0_IDLY = 0;
  // Enable pre-trigger
  PDB0_CH0C1 = PDB_CH0C1_TOS | PDB_CH0C1_EN;
  // PDB0_CH0DLY0 = 0;
  PDB0_SC = PDB_CONFIG | PDB_SC_LDOK;
  // Software trigger (reset and restart counter)
  PDB0_SC |= PDB_SC_SWTRIG;
  // Enable interrupt request
  NVIC_ENABLE_IRQ(IRQ_PDB);
}

void dmaInit() {
  // Enable DMA, DMAMUX clocks
  SIM_SCGC7 |= SIM_SCGC7_DMA;
  SIM_SCGC6 |= SIM_SCGC6_DMAMUX;

  // Use default configuration
  DMA_CR = 0;

  // Source address
  DMA_TCD1_SADDR = &ADC0_RA;
  // Don't change source address
  DMA_TCD1_SOFF = 0;
  DMA_TCD1_SLAST = 0;
  // Destination address
  DMA_TCD1_DADDR = samples;
  // Destination offset (2 byte)
  DMA_TCD1_DOFF = 2;
  // Restore destination address after major loop
  DMA_TCD1_DLASTSGA = -sizeof(samples);
  // Source and destination size 16 bit
  DMA_TCD1_ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
  // Number of bytes to transfer (in each service request)
  DMA_TCD1_NBYTES_MLNO = 2;
  // Set loop counts
  DMA_TCD1_CITER_ELINKNO = sizeof(samples) / 2;
  DMA_TCD1_BITER_ELINKNO = sizeof(samples) / 2;
  // Enable interrupt (end-of-major loop)
  DMA_TCD1_CSR = DMA_TCD_CSR_INTMAJOR;

  // Set ADC as source (CH 1), enable DMA MUX
  DMAMUX0_CHCFG1 = DMAMUX_DISABLE;
  DMAMUX0_CHCFG1 = DMAMUX_SOURCE_ADC0 | DMAMUX_ENABLE;

  // Enable request input signal for channel 1
  DMA_SERQ = 1;

  // Enable interrupt request
  NVIC_ENABLE_IRQ(IRQ_DMA_CH1);
}

// obtain data from DMA
void adc0_isr() {
  if (bigcounter - start_point  < 7500)
    bigcounter++;
}

// trigger ADC
void pdb_isr() {
  // Clear interrupt flag
  PDB0_SC &= ~PDB_SC_PDBIF;
}

void dma_ch1_isr() {
  // Clear interrupt request for channel 1
  DMA_CINT = 1;
}

// methods for calculating the varience of data
float calVar(){
  int sum = 0;
  for (int i = bigcounter; i > bigcounter - 100; i--)
    {
      sum = sum + samples[i];
    }
  float average = sum / 100.0;
  float sum1 = 0.0;
  for (int i = bigcounter; i > bigcounter - 100; i--) {
      sum1 = sum1 + pow((samples[i] - average), 2);
  }
  float var = sum1 / 100.0;
  return var;
}

volatile int page_num = 0;
volatile int index_offset = 0;
void display_UI(String stage){
  // if signal is not stabilized, screen print text
  if (stage == "calibrate"){
    if (counter_100 / 100 % 4  == 0){
      if (counter_100 % 100  == 0) {
        drawText("Calibrating...", 30, 100, ILI9341_BLACK, 3);
      } else {
        drawText("Calibrating", 30, 100, ILI9341_WHITE, 3);
      }
    } else if (counter_100 / 100 % 4  == 1) {
      if (counter_100 % 100  == 0) {
        drawText("Calibrating", 30, 100, ILI9341_BLACK, 3);
      } else {
        drawText("Calibrating.", 30, 100, ILI9341_WHITE, 3);
      }
    } else if (counter_100 / 100 % 4  == 2) {
      if (counter_100 % 100  == 0) {
      } else {
        drawText("Calibrating..", 30, 100, ILI9341_WHITE, 3);
      }
    } else if (counter_100 / 100 % 4  == 3) {
      if (counter_100 % 100  == 0) {
        drawText("Calibrating..", 30, 100, ILI9341_BLACK, 3);
      } else {
        drawText("Calibrating...", 30, 100, ILI9341_WHITE, 3);
      }
    }
    counter_100++;
  }

  // display useful information during 30s measuring process
  if (stage == "background") {
      tft.fillRoundRect(3, 3, 320-3-3, 147 - 3, 10, ILI9341_LIGHTGREY);
      tft.fillRoundRect(5, 5, 320-5-5, 147 - 7, 10, ILI9341_DARKGREY);
      drawText("ECG Filtered Signal", 110, 10, ILI9341_WHITE, 1);
      drawSmallGrid();
      tft.drawLine(5,150,319-5,150, ILI9341_WHITE);
      tft.drawLine(120,155,120,234, ILI9341_WHITE);
      drawText("Heart Rate (BPM)", 10, 155, ILI9341_WHITE, 1);
      tft.fillRoundRect(15, 165, 75, 65, 5, ILI9341_WHITE);
      drawText("ECG Analysis", 180, 155, ILI9341_WHITE, 1);
      drawText("ECG Status: ", 145, 175, ILI9341_WHITE, 1);
      drawText("QRS Int(ms): ", 145, 195, ILI9341_WHITE, 1);
      drawText("RR Int(ms): ", 145, 215, ILI9341_WHITE, 1);
  }
  // display calculated BPM and QRS data during 30s measuring process
  if (stage == "BPM&QRS") {
    tft.fillRoundRect(15, 165, 80, 70, 5, ILI9341_WHITE);
    drawText((String)avg_BPM, 38, 190, ILI9341_BLACK, 3);
    tft.fillRect(220, 175, 70, 70, ILI9341_BLACK);
    if (PVC) {
        drawText("PVC detected", 220, 175, ILI9341_WHITE, 1);  
    } else {
        drawText("PVC not detected", 220, 175, ILI9341_WHITE, 1);
    }
    drawText((String)avg_QRS, 220, 195, ILI9341_WHITE, 1);
    drawText((String)avg_RR, 220, 215, ILI9341_WHITE, 1);
  }
  // countdown from 30s to 0s, reminding user that don't leave finger from device
  if (stage == "countdown") {
    int count_250 = counter / 250;
    if (count_250 != (counter - 1) / 250) {
      tft.fillRoundRect(297,225, 18, 15, 2, ILI9341_WHITE);
      tft.setCursor(284, 5);
      drawText((String)(30 - count_250) + "s", 298, 230, ILI9341_BLACK, 1);
      
      // smartphone application displays the BPM data 
      int heart_rate = avg_BPM;

      Serial.print(F("Updating HRM value to "));
      Serial.print(heart_rate);
      Serial.println(F(" BPM"));
    
      /* Command is sent when \n (\r) or println is called */
      /* AT+GATTCHAR=CharacteristicID,value */
      ble.print( F("AT+GATTCHAR=") );
      ble.print( hrmMeasureCharId );
      ble.print( F(",00-") );
      ble.println(heart_rate, HEX);
    
      /* Check if command executed OK */
      if ( !ble.waitForOK() )
      {
        Serial.println(F("Failed to get response!"));
      }

    }
  }

  // give user option to choose between review ECG wave and review Arrhythmia report
  if (stage == "menu") {
      tft.fillScreen(ILI9341_BLACK);
      tft.fillRoundRect(15, 40, 125, 180, 5, ILI9341_GREEN);
      tft.fillRoundRect(180, 40, 125, 180, 5, ILI9341_GREEN);
      drawText("Review", 42, 100, ILI9341_BLACK, 2);
      drawText("ECG wave", 33, 140, ILI9341_BLACK, 2);
      drawText("Arrhythmia", 184, 100, ILI9341_BLACK, 2);
      drawText("Details", 200, 140, ILI9341_BLACK, 2);
      
      bool ispressed = false;
      while (stage == "menu") {
        TS_Point p = ts.getPoint();
        // we have some minimum pressure we consider 'valid'
        // pressure of 0 means no pressing!
        if (p.z > 100) {
          int x = map(p.x,0,4000,0,319);
          int y = map(p.y,0,4000,0,239);
          // only user clicks specific area of screen to enter desired section
          if (x >= 20 && x <= 140 && y >= 40 && y <= 220) {
            ispressed = !ispressed;
            int help_counter = 0;
            while (ispressed){
              p = ts.getPoint();
              x = map(p.x,0,4000,0,319);
              y = map(p.y,0,4000,0,239);
              if (p.z == 0 && x >= 20 && x <= 140 && y >= 40 && y <= 220 && help_counter > 10) {
                stage = "data";
                ispressed = false;
              } else if (p.z > 100 && x >= 20 && x <= 140 && y >= 40 && y <= 220) {
                help_counter++;
              } else {
                ispressed = false;
              }
            }
          } else if (x >= 180 && x <= 300 && y >= 40 && y <= 220) {
            ispressed = !ispressed;
            int help_counter = 0;
            while (ispressed){
              p = ts.getPoint();
              x = map(p.x,0,4000,0,319);
              y = map(p.y,0,4000,0,239);
              if (p.z == 0 && x >= 180 && x <= 300 && y >= 40 && y <= 220 && help_counter > 10) {
                stage = "report";
                ispressed = false;
              } else if (p.z > 100 && x >= 180 && x <= 300 && y >= 40 && y <= 220) {
                help_counter++;
              } else {
                ispressed = false;
              }
            }
          }
        }
      }
      tft.fillScreen(ILI9341_BLACK);
      display_UI(stage);
  }
  if(stage == "report"){
    tft.fillRoundRect(5, 5, 80, 30, 5, ILI9341_BLUE);
    drawText("Back", 23, 13, ILI9341_WHITE, 2);
    drawText("Report", 150, 13, ILI9341_WHITE, 2);
    drawText("- Average BPM: " + (String)avg_BPM, 20, 50, ILI9341_WHITE, 2);
    drawText("- Average QRS: " + (String)avg_QRS + " ms", 20, 80, ILI9341_WHITE, 2);
    if (Bradycardia) {
      drawText("- Caution!! ", 20, 110, ILI9341_WHITE, 2);
      drawText("  Bradycardia is detected", 20, 130, ILI9341_WHITE, 2);
    } 
    if (Tachycardia) {
      drawText("- Caution!! ", 20, 110, ILI9341_WHITE, 2);
      drawText("  Tachycardia is detected", 20, 130, ILI9341_WHITE, 2);
    } 
    if (!Bradycardia && !Tachycardia){
      drawText("- No Bradycardia and ", 20, 110, ILI9341_WHITE, 2);
      drawText("  Tachycardia is detected", 20, 130, ILI9341_WHITE, 2);
    }
    if (PVC) {
        drawText("- PVC detected!", 20, 160, ILI9341_WHITE, 2);  
    } else {
        drawText("- PVC not detected", 20, 160, ILI9341_WHITE, 2);
    }
    if (PAC) {
        drawText("- PAC detected!", 20, 190, ILI9341_WHITE, 2);  
    } else {
        drawText("- PAC not detected", 20, 190, ILI9341_WHITE, 2);
    }
    drawText("* See doctor for professional advice ", 20, 220, ILI9341_WHITE, 1);

    
    while (stage == "report") {
      TS_Point p = ts.getPoint();
      // we have some minimum pressure we consider 'valid'
      // pressure of 0 means no pressing!
      int x = map(p.x,0,4000,0,319);
      int y = map(p.y,0,4000,0,239);
      if (p.z > 100) {
        if (x >= 5 && x <= 85 && y >= 5 && y <= 35) {
          stage = "menu";
        }
      }
    }
    tft.fillScreen(ILI9341_BLACK);
    display_UI(stage);
  }
  
  if(stage == "data"){
    drawGrid(ILI9341_LIGHTGREY);
    tft.fillRoundRect(5, 5, 80, 30, 5, ILI9341_BLUE);
    drawText("Back", 23, 13, ILI9341_WHITE, 2);
    drawText("ECG Data", 120, 13, ILI9341_WHITE, 2);
	
	// draw current waveform
    for (int i = 1; i < 320; i++) {
      int index =  i + index_offset;
      int curr_temp = mapped[index];
      int prev_temp = mapped[index - 1];
      if (index < sample_size - 20) {
        tft.drawLine(i-1, prev_temp,  i, curr_temp, ILI9341_GREEN);
      }
    }
    
    while (true) {
      // System records the coordinate of point when finger start to touch screen,
      // and the coordinate of point when finger leave screen. If X increased from start to end, it means
      // user want scroll data to right. Oppositely, if X decreased, data is scrolled to the left. And system
      // calculate the difference between start and end in X-axis to decide how many data should be shifted. 
      bool isTouched = false;
      int initial_X = 0;
      while(!isTouched) {
        TS_Point p = ts.getPoint();
        int initial_Y = map(p.y,0,4000,0,239);
        isTouched = (p.z > 100 && initial_Y > 85);
        initial_X = map(p.x,0,4000,0,319);
        if (p.z > 100) {
          int x = map(p.x,0,4000,0,319);
          int y = map(p.y,0,4000,0,239);
          if (x >= 5 && x <= 85 && y >= 5 && y <= 50) { // back button
            display_UI("menu");
          }
        }
      }
      
      int final_X = initial_X;
	  
	  // detect movement of finger
      while (final_X - initial_X < 10 && final_X - initial_X > -10) {
        Serial.println("Yes");
        TS_Point p = ts.getPoint();
        final_X = map(p.x,0,4000,0,319);
        isTouched = !(p.z == 0);
        if (!isTouched) {
          final_X = initial_X;
          break;
        }
      }

      if (final_X != initial_X &&
          index_offset - 2*(final_X - initial_X) < sample_size - 320 &&
          index_offset - 2*(final_X - initial_X) > 0){
        drawGrid(ILI9341_DARKGREY);
        for (int i = 1; i < 320; i++) {
          int index =  i + index_offset;
          int curr_temp = mapped[index];
          int prev_temp = mapped[index - 1];
          if (index < sample_size - 20) {
            tft.drawLine(i-1, prev_temp,  i, curr_temp, ILI9341_BLACK);
          }
        }
        index_offset -= 2*(final_X - initial_X);
        break;
      }
    }
    Serial.println(stage);
    display_UI(stage);
  }
}


#define NZEROS 4
#define NPOLES 4
#define GAIN   1.412950429e+02

static float xv[NZEROS+1], yv[NPOLES+1];

// lowpass filter equation
static float filterloop(int rawData) {
  for (;;) {
    xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; 
    xv[4] = rawData / GAIN;
    yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; 
    yv[4] =   (xv[0] + xv[4]) + 4 * (xv[1] + xv[3]) + 6 * xv[2]
                 + ( -0.1518009223 * yv[0]) + (  0.8831957792 * yv[1])
                 + ( -2.0212349314 * yv[2]) + (  2.1766018485 * yv[3]);
    if (yv[4] > 4095) yv[4] = 4095;
    else if (yv[4] < 0) yv[4] = 0;
    return yv[4];
  }
}

int SPKI = 0;
int NPKI = 0;
int PEAKI = 0;
int threshold1 = 0;
int threshold2 = 0;
int detect_counter = 0;
int current_peak = 0;
int previous_peak = 0;
int heartBeat = 0;
int previous_RR = 0;
int RR_temp = 0;

void peakDetect(){
  if (detect_counter < 180){
    // find the local max inside of window size
    if (average > PEAKI) {
      PEAKI = average; 
      current_peak = bigcounter; 
    }
    detect_counter++;
  } else { // if it is signal peak
    if (PEAKI > threshold1) {
      SPKI = 0.125*PEAKI + 0.875*SPKI;
      heartBeat++;
      // calculate the BPM based on the time difference between last heart beat and current heart beat
      int BPM_temp = (60 / ((current_peak - previous_peak) * 0.004));
      int BPM;

      // RR is time difference between last Rwave and current Rwave
      RR_temp = (current_peak - previous_peak) * 4;
      int RR;

      // ignore the bias
      if (RR_temp > avg_RR * 1.5 || RR_temp < avg_RR * 0.5) {
          RR = avg_RR;
      } else {
          // wait stable average RR interval for decide if PAV happen
          if (heartBeat < 15){
              RR = RR_temp;
              avg_RR = (avg_RR * heartBeat + RR) / (heartBeat + 1);  
          // one of the condition exist, PAC is detected       
          } else if ((previous_RR < 0.9 * avg_RR) || (RR_temp > 1.1 * avg_RR) || (RR_temp/previous_RR > 1.2 && RR_temp/previous_RR < 1.8)) {
            PAC_counter++;
            avg_RR = avg_RR;
          } else {
            RR = RR_temp;
            avg_RR = (avg_RR * heartBeat + RR) / (heartBeat + 1);
          }
        }
      
      if (BPM_temp > avg_BPM * 3 || BPM_temp < avg_BPM * 0.4) {
        BPM = avg_BPM;
      } else {
        BPM = BPM_temp;
        avg_BPM = (avg_BPM * heartBeat + BPM) / (heartBeat + 1);
      }
      
    } else { // if it is noise peak
      NPKI = 0.125*PEAKI + 0.875*NPKI;
    }
    threshold1 = NPKI + 0.25*(SPKI - NPKI);
    threshold2 = 0.5*threshold1;
    detect_counter = 0;
    PEAKI = 0;
    previous_peak = current_peak;
    previous_RR = RR_temp;

   // in order to find QRS complext, find the start point of P wave rising edage, and calculate the time interval between the 
   // start point and peak, convert into second arcorrding to used scaling value
   int QRS_start = current_peak - 40;
   int mark_start = 0;
   for (int i = 0; i<40; i++) {
    int diff = map(arr[QRS_start + i],0,40000,40,240) - map(arr[QRS_start + i -1],0,40000,40,240);
    if (diff > 3) {
      mark_start = QRS_start + i;
      break;
    }
    }
   int QRS_temp = (current_peak - mark_start) * 4;
   int QRS;
  if (QRS_temp > avg_QRS * 3 || QRS_temp < avg_QRS * 0.5) {
    QRS = avg_QRS;
  } else {
    QRS = QRS_temp;
    avg_QRS = (avg_QRS * heartBeat + QRS) / (heartBeat + 1);
  }
  display_UI("BPM&QRS");
  }
}

// a helper function for tft.println()
void drawText(String txt, int x, int y, int color, float font_size) {
  tft.setCursor(x, y);
  tft.setTextSize(font_size);
  tft.setTextColor(color);
  tft.println(txt);
}

