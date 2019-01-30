// Freddie He, 1560733
// Lab 2, Section C

// Create an IntervalTimer object 
IntervalTimer myTimer;

const int ledPin_red = 20;  // the pin with red LED
const int ledPin_green = 21;  // the pin with green LED
const int ledPin_blue = 22;  // the pin with blus LED

void setup() {
  pinMode(ledPin_red, OUTPUT);
  pinMode(ledPin_green, OUTPUT);
  pinMode(ledPin_blue, OUTPUT);
  Serial.begin(9600);
  myTimer.begin(blinkLED, 10000); // blinkLED to run every 0.01 seconds
  digitalWrite(ledPin_red, 1);
  digitalWrite(ledPin_green, 1);
  digitalWrite(ledPin_blue, 1);
}

// The interrupt will blink the LED, and keep
// track of how many times it has blinked.
volatile int bright_level = 255; //0: brightest, 255: off
volatile int fade = 0; // 0: fades up, 1: fades down
volatile int led = 0; // 0: red, 1: green, 2: blue

// functions called by IntervalTimer should be short, run as quickly as
// possible, and should avoid calling other functions if possible.
void blinkLED() {
  if (led == 0) {
    digitalWrite(ledPin_green, 1);
    digitalWrite(ledPin_blue, 1);
    analogWrite(ledPin_red, bright_level);
    
  } else if (led == 1) {
    digitalWrite(ledPin_red, 1);
    analogWrite(ledPin_green, bright_level);
    digitalWrite(ledPin_blue, 1);
  } else {
    digitalWrite(ledPin_red, 1);
    digitalWrite(ledPin_green, 1);
    analogWrite(ledPin_blue, bright_level);
  }
  
  if (bright_level == 255 and fade == 1) {
    fade = 0;
    led = (led + 1) % 3; // switch to next LED
  } else if (bright_level == 0) {
    fade = 1;
  }
  if (fade == 0) {
    bright_level = bright_level - 1;
  } else{
    bright_level = bright_level + 1;
  }
}

// The main program will print the brightness
// to the Arduino Serial Monitor
void loop() {
  unsigned long blinkCopy;  // holds a copy of the brightness_level

  // to read a variable which the interrupt code writes, we
  // must temporarily disable interrupts, to be sure it will
  // not change while we are reading.  To minimize the time
  // with interrupts off, just quickly make a copy, and then
  // use the copy while allowing the interrupt to keep working.
  noInterrupts();
  blinkCopy = bright_level;
  interrupts();

  Serial.print("blinkCount = ");
  Serial.println(blinkCopy);
  delay(10);
}
