#include "Configuration.h"
#if defined(HB_SSD1963)
#include "ultralcd.h"
#include "Marlin.h"

#include <UTFT.h>
#include <UTouch.h>
#include <UTFT_Buttons.h>

// Declare which fonts we will be using
extern uint8_t SmallFont[];
extern uint8_t BigFont[];
extern uint8_t Dingbats1_XL[];

UTFT          myGLCD(20, 42, 41, 39, 38);
UTouch        myTouch(43, 44, 45, 46, 47);
UTFT_Buttons  myButtons(&myGLCD, &myTouch);

#define STEPS_REV    200
#define MICROSTEPPING 32
#define STIGNING       4
#define STEPS_MM       ((STEPS_REV*MICROSTEPPING)/STIGNING)

typedef struct Motor {
  byte EnablePin;
  byte StepPin;
  byte DirectionPin;
} Motor;

Motor Motors[7] = {
  { .EnablePin=61, .StepPin= 6, .DirectionPin= 7},
  { .EnablePin=60, .StepPin=15, .DirectionPin=14},
  { .EnablePin=59, .StepPin=17, .DirectionPin=16},
  { .EnablePin=58, .StepPin=19, .DirectionPin=18},
  { .EnablePin=12, .StepPin=63, .DirectionPin=62},
  { .EnablePin= 9, .StepPin=65, .DirectionPin=64},
  { .EnablePin= 3, .StepPin=67, .DirectionPin=66}
};

const byte X_LimPin = 49, Y1_LimPin = 50, Y2_LimPin = 51, Z_LimPin = 2;
const byte A_motor = 0, B_motor = 1, C_motor = 2, D_motor = 3, X_motor = 4, Y1_motor = 5, Y2_motor = 6;

const byte LaserPin = 4, Laser2Pin = 4, MosFet1Pin = 11, NosFet2Pin = 10;
int numberOfSteps = 100;
byte ledPin = 13;
int pulseWidthMicros = 10;  // microseconds
int microsbetweenSteps = 500; // microeconds

void lcd_main_static();
void lcd_main_dynamic();

void lcd_init() 
{
  pinMode(40, OUTPUT);
  digitalWrite(40, HIGH);

  myGLCD.InitLCD();
  myGLCD.clrScr();
  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRect(0, 0, 799, 479);
  myGLCD.setFont(BigFont);

  myTouch.InitTouch();
  myTouch.setPrecision(PREC_MEDIUM);

  myButtons.setTextFont(BigFont);
  myButtons.setSymbolFont(Dingbats1_XL);

  lcd_main_static();
}

void stepSomeUp(byte stepPin, byte dirPin, boolean up);
void stepAllUp(boolean up);

int motorAUp, motorADown, motorBUp, motorBDown, motorCUp, motorCDown, motorDUp, motorDDown;
int motorAllUp, motorAllDown;

void lcd_main_static()
{
  const int bW = 100, dW = 800;
  const int bH = 80, dH = 480;
  const int dist = 10;

  motorBUp     = myButtons.addButton(dist,           dist,       bW, bH, " B+ ");
  motorBDown   = myButtons.addButton(2 * dist + bW,    dist,       bW, bH, " B- ");
  motorAUp     = myButtons.addButton(dist,           dH - dist - bH, bW, bH, " A+ ");
  motorADown   = myButtons.addButton(2 * dist + bW,    dH - dist - bH, bW, bH, " A- ");
  motorCUp     = myButtons.addButton(dW - 2 * (dist + bW), dist,       bW, bH, " C+ ");
  motorCDown   = myButtons.addButton(dW - dist - bW,     dist,       bW, bH, " C- ");
  motorDUp     = myButtons.addButton(dW - 2 * (dist + bW), dH - dist - bH, bW, bH, " D+ ");
  motorDDown   = myButtons.addButton(dW - dist - bW,     dH - dist - bH, bW, bH, " D- ");
  motorAllUp   = myButtons.addButton(dW / 2 - bW - dist / 2, dH / 2 - bH / 2,  bW, bH, " All+ ");
  motorAllDown = myButtons.addButton(dW / 2 + dist / 2,    dH / 2 - bH / 2,  bW, bH, " All- ");
  myButtons.drawButtons();

  myGLCD.print("You pressed:", 400, 200);
  myGLCD.setColor(VGA_BLACK);
  myGLCD.setBackColor(VGA_WHITE);
  myGLCD.print(" ---    ", 500, 240);

}

void lcd_main_dynamic()
{
  while (myTouch.dataAvailable() == true)
  {
    int pressed_button = myButtons.checkButtons();

    if (pressed_button == motorAUp) {
      myGLCD.print("A up", 500, 240);
      stepSomeUp(Motors[A_motor].StepPin, Motors[A_motor].DirectionPin, true);
    }
    if (pressed_button == motorADown) {
      myGLCD.print("A down", 500, 240);
      stepSomeUp(Motors[A_motor].StepPin, Motors[A_motor].DirectionPin, false);
    }
    if (pressed_button == motorBUp) {
      myGLCD.print("B up", 500, 240);
      stepSomeUp(Motors[B_motor].StepPin, Motors[B_motor].DirectionPin, true);
    }
    if (pressed_button == motorBDown) {
      myGLCD.print("B down", 500, 240);
      stepSomeUp(Motors[B_motor].StepPin, Motors[B_motor].DirectionPin, false);
    }
    if (pressed_button == motorCUp) {
      myGLCD.print("C up", 500, 240);
      stepSomeUp(Motors[C_motor].StepPin, Motors[C_motor].DirectionPin, true);
    }
    if (pressed_button == motorCDown) {
      myGLCD.print("C down", 500, 240);
      stepSomeUp(Motors[C_motor].StepPin, Motors[C_motor].DirectionPin, false);
    }
    if (pressed_button == motorDUp) {
      myGLCD.print("D up", 500, 240);
      stepSomeUp(Motors[D_motor].StepPin, Motors[D_motor].DirectionPin, true);
    }
    if (pressed_button == motorDDown) {
      myGLCD.print("D down", 500, 240);
      stepSomeUp(Motors[D_motor].StepPin, Motors[D_motor].DirectionPin, false);
    }
    if (pressed_button == motorAllUp) {
      myGLCD.print("All up", 500, 240);
      stepAllUp(true);
    }
    if (pressed_button == motorAllDown) {
      myGLCD.print("All down", 500, 240);
      stepAllUp(false);
    }
    if (pressed_button == -1)
      myGLCD.print("None    ", 500, 240);
  }
}

void lcd_update()
{
  extern bool laserUpdateLCD;
  if (!laserUpdateLCD)
    return;
  lcd_main_dynamic();
}

void stepSomeUp(byte stepPin, byte dirPin, boolean up)
{
#if 1
  digitalWrite(dirPin, up ? HIGH : LOW);
  for (int n = 0; n < STEPS_MM; n++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(pulseWidthMicros); // this line is probably unnecessary
    digitalWrite(stepPin, LOW);
    delayMicroseconds(microsbetweenSteps);
  }
#endif
}

void stepAllUp(boolean up)
{
#if 1
  digitalWrite(Motors[A_motor].DirectionPin, up ? HIGH : LOW);
  digitalWrite(Motors[B_motor].DirectionPin, up ? HIGH : LOW);
  digitalWrite(Motors[C_motor].DirectionPin, up ? HIGH : LOW);
  digitalWrite(Motors[D_motor].DirectionPin, up ? HIGH : LOW);
  for (int n = 0; n < STEPS_MM; n++) {
    for (uint8_t m = A_motor; m <= D_motor; m++)
      digitalWrite(Motors[m].StepPin, HIGH);
    delayMicroseconds(pulseWidthMicros); // this line is probably unnecessary
    for (uint8_t m = A_motor; m <= D_motor; m++)
      digitalWrite(Motors[m].StepPin, LOW);
    delayMicroseconds(microsbetweenSteps);
  }
#endif
}


void lcd_setstatus(const char* message, const bool persist){}
void lcd_setstatuspgm(const char* message, const uint8_t level){}
bool lcd_hasstatus(){}
void lcd_setalertstatuspgm(const char* message){}
void lcd_reset_alert_level(){}
void lcd_buttons_update(){}


#endif
