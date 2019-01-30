//Yueyang Cheng
//lab#7
/* ADCpdbDMA
PDB triggers the ADC which requests the DMA to move the data to a buffer
*/
#define PDB_CH0C1_TOS 0x0100
#define PDB_CH0C1_EN 0x01
#include <SD.h>
#define TFT_DC  9
#define TFT_CS 10
#define CS_PIN  4

#include "SPI.h"
#include "ILI9341_t3.h"
#include <font_Arial.h> // from ILI9341_t3
#include <XPT2046_Touchscreen.h>

XPT2046_Touchscreen ts(CS_PIN);
#define TIRQ_PIN  2

ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC);
volatile int counter;
volatile bool START;

uint8_t ledOn = 0;
uint16_t samples[7500];

uint16_t arr[7500];
int bigcounter = -1;

void setup() {
  Serial.begin(9600);
  while (!Serial); // wait until the serial debug window is opened

  adcInit();
  pdbInit();
  dmaInit();
  START = false;
  tft.setRotation(1);
  tft.fillScreen(ILI9341_WHITE);
  drawGrid();
  counter = 0;
}

void drawGrid() {
  //vertical lines
  for(int i = 0; i < 32; i++) {
    tft.drawLine(0+10*i, 40, 0+10*i, 239, ILI9341_RED);
    if (i % 5 == 1){
       tft.drawLine(10*i-1, 40, 10*i-1, 239, ILI9341_RED);
    }    
  }
  //horizontal lines
  for (int i = 4; i < 24; i++) {
    tft.drawLine(0, 0+10*i, 319, 0+10*i, ILI9341_RED);
    if (i % 5 == 4){
       tft.drawLine(0, 10*i-1, 319, 10*i-1, ILI9341_RED);
    } 
  }
}

void loop() {   
  // calculate the variance, if the variance is samll, which means the
  // data is stabilized
  noInterrupts();
  float variance = 0;
  if(bigcounter > 100) {
    variance = calVar();
  }
  interrupts();

  if ((variance < 5000 && variance > 0)) {
    START = true;
  }
  drawECG();
    // if signal is not stabilized, screen print text
  if (!START){
    tft.setCursor(80, 10);
    tft.setTextColor(ILI9341_BLACK);  tft.setTextSize(2);
    tft.println("Calibrating!!"); 
  }
  
  delay(4);
}

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
  float variance = sum1 / 100.0;
  return variance;
}

#define NZEROS 4
#define NPOLES 4
#define GAIN   1.412950429e+02

static float xv[NZEROS+1], yv[NPOLES+1];

static float filterloop(int rawData)
  { for (;;)
      { xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; 
        xv[4] = rawData / GAIN;
        yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; 
        yv[4] =   (xv[0] + xv[4]) + 4 * (xv[1] + xv[3]) + 6 * xv[2]
                     + ( -0.1518009223 * yv[0]) + (  0.8831957792 * yv[1])
                     + ( -2.0212349314 * yv[2]) + (  2.1766018485 * yv[3]);
        return yv[4];
      }
  }

int previous = 140;
void drawECG() { 
  int val1 = (int)filterloop(samples[bigcounter]);
  samples[bigcounter] = val1;
  //derivative filter
  float val2 = (1/8.0)*(samples[bigcounter]+2*samples[bigcounter-1]-2*samples[bigcounter-3]-samples[bigcounter-4]);
  arr[bigcounter] = (int)val2;

  //squaring
  arr[bigcounter] *= arr[bigcounter];

  // moving-window integratior
  
  
  if (START) {  
    int current;  
//    Serial.println("START DRAWING");
    Serial.print("arr data:");
    Serial.println(arr[bigcounter]);
    current = map(arr[bigcounter],0,40960,40,240);  
//    Serial.print("current data:");
//    Serial.println(current);
    tft.drawLine((counter+1)%320, 40, (counter+1)%320, 239, ILI9341_WHITE);
    
    //horizontal lines
    for(int i = 4; i < 24; i++) {
      tft.drawPixel((counter+1)%320, 10*i, ILI9341_RED);
      if (i % 5 == 4){
         tft.drawPixel((counter+1)%320, 10*i-1, ILI9341_RED);
      }
    }
    //vertical lines
    if (counter%320%10 == 0 || counter%320%50 == 9) {
      tft.drawLine(counter%320, 40, counter%320, 239, ILI9341_RED); 
    }
    if (counter == 0 || counter % 320 == 319) {
      tft.drawPixel(counter % 320, current, ILI9341_BLACK);
    } else {
      tft.drawLine(counter%320, previous, (counter+1)%320, current, ILI9341_BLACK);
    }
    counter++;

    previous = current;
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

void adc0_isr() {
  if(bigcounter < 7500){
    bigcounter++;
  }
}

void pdb_isr() {
  // Clear interrupt flag
  PDB0_SC &= ~PDB_SC_PDBIF;
}

void dma_ch1_isr() {
  // Clear interrupt request for channel 1
  DMA_CINT = 1;
}
