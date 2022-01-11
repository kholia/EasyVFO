// https://github.com/afarhan/ubitx/blob/master/ubitx_20/ubitx_20.ino

#include <si5351.h> // Install https://github.com/etherkit/Si5351Arduino
#include <Wire.h>
// #include <EEPROM.h>

// Display
#include <LiquidCrystal.h> // Install https://github.com/arduino-libraries/LiquidCrystal

// LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
LiquidCrystal lcd(8, 9, 10, 11, 12, 13);

Si5351 si5351;

// Frequency configurations
volatile unsigned long frequency = 150000000UL; // 150 MHz variable (VFO)
volatile unsigned long fixed_frequency = 116000000UL; // 116 MHz fixed for 2m upconverter operation
unsigned long iffreq = 0; // set the IF frequency in Hz

int ret = 0;

// Raduino
#define ENC_A   (A0)
#define ENC_B   (A1)
#define FBUTTON (A2)
#define PTT     (A3)

// Note: A5, A4 are wired to the Si5351 as I2C interface

void initPorts()
{
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  pinMode(FBUTTON, INPUT_PULLUP);
  pinMode(PTT, INPUT_PULLUP);
}


// Returns true if the button is pressed
int btnDown()
{
  if (digitalRead(FBUTTON) == HIGH)
    return 0;
  else
    return 1;
}

char c[30], b[30];
char printBuff[2][17];  // mirrors what is showing on the two lines of the display

// The generic routine to display one line on the LCD
void printLine(int linenmbr, char *c) {
  if (strcmp(c, printBuff[linenmbr])) {     // only refresh the display when there was a change
    lcd.setCursor(0, linenmbr);             // place the cursor at the beginning of the selected line
    lcd.print(c);
    strcpy(printBuff[linenmbr], c);

    for (byte i = strlen(c); i < 16; i++) { // add white spaces until the end of the 16 characters line is reached
      lcd.print(' ');
    }
  }
}

//  short cut to print to the first line
void printLine1(char *c) {
  printLine(1, c);
}
//  short cut to print to the second line
void printLine2(char *c) {
  printLine(0, c);
}

int enc_prev_state = 3;

byte enc_state (void) {
  return (analogRead(ENC_A) > 500 ? 1 : 0) + (analogRead(ENC_B) > 500 ? 2 : 0);
}

int enc_read(void) {
  int result = 0;
  byte newState;
  int enc_speed = 0;

  unsigned long stop_by = millis() + 50;

  while (millis() < stop_by) { // check if the previous state was stable
    newState = enc_state(); // Get current state

    if (newState != enc_prev_state)
      delay (1);

    if (enc_state() != newState || newState == enc_prev_state)
      continue;
    // these transitions point to the encoder being rotated anti-clockwise
    if ((enc_prev_state == 0 && newState == 2) ||
        (enc_prev_state == 2 && newState == 3) ||
        (enc_prev_state == 3 && newState == 1) ||
        (enc_prev_state == 1 && newState == 0)) {
      result--;
    }
    // these transitions point to the encoder being rotated clockwise
    if ((enc_prev_state == 0 && newState == 1) ||
        (enc_prev_state == 1 && newState == 3) ||
        (enc_prev_state == 3 && newState == 2) ||
        (enc_prev_state == 2 && newState == 0)) {
      result++;
    }
    enc_prev_state = newState; // Record state for next pulse interpretation
    enc_speed++;
    delay(1);
  }
  return (result);
}

void checkButton() {
  // only if the button is pressed
  if (!btnDown())
    return;
  delay(50);
  if (!btnDown()) // debounce
    return;

  // doMenu(); // NOTE
  Serial.println("Button pressed!");
  // wait for the button to go up again
  while (btnDown())
    delay(10);
  delay(50); // debounce
}

void setFrequency(unsigned long f) {
  frequency = f;
  si5351.set_freq(frequency * 100ULL, SI5351_CLK0);
}

/*
   The tuning jumps by 50 Hz on each step when you tune slowly
   As you spin the encoder faster, the jump size also increases
   This way, you can quickly move to another band by just spinning the
   tuning knob
*/

void doTuning() {
  int s;
  // unsigned long prev_freq;

  s = enc_read();
  if (s) {
    // prev_freq = frequency;

    if (s > 10)
      frequency += 200000l;
    if (s > 7)
      frequency += 10000l;
    else if (s > 4)
      frequency += 1000l;
    else if (s > 2)
      frequency += 500;
    else if (s > 0)
      frequency +=  50l;
    else if (s > -2)
      frequency -= 50l;
    else if (s > -4)
      frequency -= 500l;
    else if (s > -7)
      frequency -= 1000l;
    else if (s > -9)
      frequency -= 10000l;
    else
      frequency -= 200000l;

    setFrequency(frequency);
    updateDisplay();

    Serial.println(frequency);
  }
}

void updateDisplay() {
  memset(c, 0, sizeof(c));
  memset(b, 0, sizeof(b));

  ultoa(frequency, b, DEC);

  printLine(0, b);
}

void setup()
{
  Serial.begin(9600);

  lcd.begin(16, 2);

  // Print a debug message
  printLine2((char *)"Easy VFO!");
  delay(1000);

  updateDisplay();

  initPorts();

  // Si5351 calibration factor, adjust this to get exactly 10MHz. Increasing
  // this value will decreases the frequency and vice versa. Tip: Use
  // calibration sketch to find value.
  long cal = 115000; // CHANGE ME PLEASE!

  ret = si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, cal);

  si5351.output_enable(SI5351_CLK0, 1);  // 1 means Enable
  si5351.output_enable(SI5351_CLK1, 1);
  si5351.output_enable(SI5351_CLK2, 1);
  si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA);  // Output current 2/4/6/8 mA
  si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_8MA);  // Output current 2/4/6/8 mA
  si5351.drive_strength(SI5351_CLK2, SI5351_DRIVE_8MA);  // Output current 2/4/6/8 mA
  si5351.set_freq(frequency * 100ULL, SI5351_CLK0);
  si5351.set_freq(fixed_frequency * 100ULL, SI5351_CLK2);
  si5351.set_freq((frequency + iffreq) * 100ULL, SI5351_CLK1);
  // si5351.set_freq(iffreq * 100ULL, SI5351_CLK2);
}

void loop()
{
  // Serial.println(ret);

  checkButton();

  doTuning();
}
