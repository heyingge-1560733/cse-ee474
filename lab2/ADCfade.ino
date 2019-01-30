// Freddie He, 1560733
// Lab 2, Section C

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
  myTimer.begin(blinkLED, 10000);  // blinkLED to run every 0.01 seconds
  digitalWrite(ledPin_red, 1);
  digitalWrite(ledPin_green, 1);
  digitalWrite(ledPin_blue, 1);
}

// The interrupt will blink the LED, and keep
// track of value of the counter
volatile int bright_level = 255;
volatile int fade = 0; // 0: fades up, 1: fades down
volatile int led = 0; // 0: on, 1: off
volatile int val; // analogRead from potentiometer
volatile int counter = 0;

// functions called by IntervalTimer should be short, run as quickly as
// possible, and should avoid calling other functions if possible.
void blinkLED() {
  if (counter == val / 100 + 1) {
    if (led == 0) {
      analogWrite(ledPin_red, bright_level);
      digitalWrite(ledPin_green, 1);
      digitalWrite(ledPin_blue, 1);
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
      led = (led + 1) % 3;
    } else if (bright_level == 0) {
      fade = 1;
    }
    if (fade == 0) {
      bright_level = bright_level - 1;
    } else{
      bright_level = bright_level + 1;
    } 
    counter = 0; // reset counter
  }
  if (counter > val / 100) {
    counter = 0;
  }
  counter += 1;
}

// The main program will print the value of the counter
// to the Arduino Serial Monitor
void loop() {
  unsigned long blinkCopy;  // holds a copy of the blinkCount
  val = analogRead(2);

  // to read a variable which the interrupt code writes, we
  // must temporarily disable interrupts, to be sure it will
  // not change while we are reading.  To minimize the time
  // with interrupts off, just quickly make a copy, and then
  // use the copy while allowing the interrupt to keep working.
  noInterrupts();
  blinkCopy = counter;
  interrupts();

  Serial.print("blinkCount = ");
  Serial.println(blinkCopy);
  delay(10);
}
