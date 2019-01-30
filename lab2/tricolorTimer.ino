// Freddie He, 1560733
// Lab 2, Section B

// Create an IntervalTimer object 
IntervalTimer myTimer;

const int ledPin_red = 20;  // the pin with red LED
const int ledPin_green = 21;  // the pin with green LED
const int ledPin_blue = 22;  // the pin with blue LED

void setup() {
  pinMode(ledPin_red, OUTPUT);
  pinMode(ledPin_green, OUTPUT);
  pinMode(ledPin_blue, OUTPUT);
  Serial.begin(9600);
  myTimer.begin(blinkLED, 1000000);  // blinkLED to run every 0.1 seconds
}

// The interrupt will blink the LED, and keep
// track of which color is turne on,
volatile int ledState_red = 1; // 0: on, 1:off
volatile int ledState_green = 1;
volatile int ledState_blue = 1;

volatile unsigned long blinkCount = 0; // use volatile for shared variables

// functions called by IntervalTimer should be short, run as quickly as
// possible, and should avoid calling other functions if possible.
void blinkLED() {
  if (ledState_red == 0) {
    ledState_red = 1;
    ledState_green = 0;
    ledState_blue = 1;
    blinkCount = 0;  // reset when red LED turns on
  } else if (ledState_green == 0) {
    ledState_red = 1;
    ledState_green = 1;
    ledState_blue = 0;
    blinkCount = blinkCount + 1;  // increase when green LED turns on
  } else {
    ledState_red = 0;
    ledState_green = 1;
    ledState_blue = 1;
    blinkCount = blinkCount + 1;  // increase when blue LED turns on
  }
  digitalWrite(ledPin_red, ledState_red);
  digitalWrite(ledPin_green, ledState_green);
  digitalWrite(ledPin_blue, ledState_blue);
}

// The main program will print the blink count
// to the Arduino Serial Monitor
void loop() {
  unsigned long blinkCopy;  // holds a copy of the blinkCount

  // to read a variable which the interrupt code writes, we
  // must temporarily disable interrupts, to be sure it will
  // not change while we are reading.  To minimize the time
  // with interrupts off, just quickly make a copy, and then
  // use the copy while allowing the interrupt to keep working.
  noInterrupts();
  blinkCopy = blinkCount;
  interrupts();

  Serial.print("blinkCount = ");
  Serial.println(blinkCopy);
  delay(1000);
}
