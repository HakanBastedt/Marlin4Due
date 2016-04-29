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
extern uint8_t SixteenSegment40x60[];
extern uint8_t Dingbats1_XL[];

UTFT          myGLCD(20, 42, 41, 39, 38);
UTouch        myTouch(43, 44, 45, 46, 47);
UTFT_Buttons  LCD_main_Buttons(&myGLCD, &myTouch);
UTFT_Buttons  LCD_table_Buttons(&myGLCD, &myTouch);
UTFT_Buttons  LCD_align_Buttons(&myGLCD, &myTouch);

#define STEPS_REV    200
#define MICROSTEPPING 32
#define STIGNING       4
#define STEPS_MM       ((STEPS_REV*MICROSTEPPING)/STIGNING)
#define LASER_NOZZLE_DISTANCE 6.0 // Focus distance
#define LASER_Z_COMP  1.0         // Distance between z probe on-point and nozzle

typedef struct Motor {
  byte EnablePin;
  byte StepPin;
  byte DirectionPin;
} Motor;

Motor Motors[7] = {
  { .EnablePin=A_ENABLE_PIN,  .StepPin=A_STEP_PIN,  .DirectionPin=A_DIR_PIN},
  { .EnablePin=B_ENABLE_PIN,  .StepPin=B_STEP_PIN,  .DirectionPin=B_DIR_PIN},
  { .EnablePin=C_ENABLE_PIN,  .StepPin=C_STEP_PIN,  .DirectionPin=C_DIR_PIN},
  { .EnablePin=D_ENABLE_PIN,  .StepPin=D_STEP_PIN,  .DirectionPin=D_DIR_PIN},
  { .EnablePin=X_ENABLE_PIN,  .StepPin=X_STEP_PIN,  .DirectionPin=X_DIR_PIN},
  { .EnablePin=Y_ENABLE_PIN,  .StepPin=Y_STEP_PIN,  .DirectionPin=Y_DIR_PIN},
  { .EnablePin=Y2_ENABLE_PIN, .StepPin=Y2_STEP_PIN, .DirectionPin=Y2_DIR_PIN}
};
struct {
  int homeAxis;
  int gotoTable;
  int gotoAlign;
  bool buttonsDone;
} LCD_main;

// Check for exceeding MAX_BUTTONS in UTFT_Buttons.h 30.
struct {
  int motorAUp, motorADown, motorBUp, motorBDown, motorCUp, motorCDown, motorDUp, motorDDown;
  int cornerA, cornerB, cornerC, cornerD, midPoint; 
  int motorAllUp, motorAllDown;
  int microStepsB, smallStepsB, largeStepsB;
  int measureA, measureB, measureC, measureD, level;
  float distA, distB, distC, distD;
  int gotoMain;
  float stepSize;
  bool buttonsDone;
} LCD_table;

struct {
  int corner1, corner2, corner3, corner4, Fire; 
  int gotoMain;
  bool buttonsDone;
} LCD_align;

const byte X_LimPin = X_MAX_PIN, Y1_LimPin = Y_MIN_PIN, Y2_LimPin = Y2_MIN_PIN;
const byte A_motor = 0, B_motor = 1, C_motor = 2, D_motor = 3, X_motor = 4, Y1_motor = 5, Y2_motor = 6;

const byte LaserPin = 4, Laser2Pin = 4, MosFet1Pin = 11, NosFet2Pin = 10;
bool LCD_doingRaster = false;

byte lcd_main_static();
void lcd_main_dynamic();
byte lcd_table_static();
void lcd_table_dynamic();
byte lcd_align_static();
void lcd_align_dynamic();

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

  LCD_main_Buttons.setTextFont(BigFont);
  LCD_main_Buttons.setSymbolFont(Dingbats1_XL);

  LCD_table_Buttons.setTextFont(BigFont);
  LCD_table_Buttons.setSymbolFont(Dingbats1_XL);

  LCD_align_Buttons.setTextFont(BigFont);
  LCD_align_Buttons.setSymbolFont(Dingbats1_XL);

  LCD_table.stepSize = 1;
  LCD_table.buttonsDone = false;
  LCD_main.buttonsDone = false;
  LCD_align.buttonsDone = false;

  lcd_main_static();
}

void stepOne(int m, float mm);
void stepAll(float mm);
void stepAllOneUp(boolean up);

int LCD_menu = 0; // Start at main

byte lcd_main_static()
{
  const int bW = 100, dW = 800;
  const int bH = 80, dH = 480;
  const int dist = 10;

  if (!LCD_main.buttonsDone) {
    LCD_main.homeAxis  = LCD_main_Buttons.addButton(dW - (dist + bW),           dist, bW, bH, "HomeA");
    LCD_main.gotoTable = LCD_main_Buttons.addButton(dW/2 - bW - dist, dH - bH - dist, bW, bH, "Table");
    LCD_main.gotoAlign = LCD_main_Buttons.addButton(dW/2 + dist,      dH - bH - dist, bW, bH, "Align");
    LCD_main.buttonsDone = true;
  }
  myGLCD.setColor(200, 200, 200);
  myGLCD.fillRect(0, 0, 799, 479);
  myGLCD.setBackColor(200, 200, 200);
  myGLCD.setColor(50, 50, 50);
  myGLCD.setFont(SixteenSegment40x60);
  LCD_main_Buttons.drawButtons(); 
  return 0;
}


void lcd_main_dynamic()
{
  static uint8_t count = 0;
  if (count++ < 10)
    return;
  count=0;
  float x = current_position[X_AXIS];
  float y = current_position[Y_AXIS];
  myGLCD.printNumF(x, 2, 50+(x<10.0 ? 80 : (x < 100.0 ? 40: 0)), 100);
  myGLCD.printNumF(y, 2, 50+(y<10.0 ? 80 : (y < 100.0 ? 40: 0)), 200);
  while (myTouch.dataAvailable() == true)
  {
    int pressed_button = LCD_main_Buttons.checkButtons();

    if (pressed_button == LCD_main.homeAxis) {
      enqueuecommands_P(PSTR("G28"));      
    }
    if (pressed_button == LCD_main.gotoTable) {
      LCD_menu = lcd_table_static();
    }
    if (pressed_button == LCD_main.gotoAlign) {
      LCD_menu = lcd_align_static();
    }
  }
}

byte lcd_table_static()
{
  const int bW = 70, dW = 800;
  const int bH = 60, dH = 480;
  const int dist = 6;

  if (!LCD_table.buttonsDone) {
    LCD_table.motorAUp     = LCD_table_Buttons.addButton(dist,                   dH - dist - bH,  bW, bH, "A+");
    LCD_table.motorADown   = LCD_table_Buttons.addButton(2 * dist + bW,          dH - dist - bH,  bW, bH, "A-");
    LCD_table.cornerA      = LCD_table_Buttons.addButton(3 * dist + 2 * bW,      dH - dist - bH,  bW, bH, "Hit");
    LCD_table.measureA     = LCD_table_Buttons.addButton(4 * dist + 3 * bW,      dH - dist - bH,  bW, bH, "M.A");
    LCD_table.motorBUp     = LCD_table_Buttons.addButton(dist,                   dist,            bW, bH, "B+");
    LCD_table.motorBDown   = LCD_table_Buttons.addButton(2 * dist + bW,          dist,            bW, bH, "B-");
    LCD_table.cornerB      = LCD_table_Buttons.addButton(3 * dist + 2 * bW,      dist,            bW, bH, "Hit");
    LCD_table.measureB     = LCD_table_Buttons.addButton(4 * dist + 3 * bW,      dist,            bW, bH, "M.B");
    LCD_table.measureC     = LCD_table_Buttons.addButton(dW - 4 * (dist + bW),   dist,            bW, bH, "M.C");
    LCD_table.cornerC      = LCD_table_Buttons.addButton(dW - 3 * (dist + bW),   dist,            bW, bH, "Hit");
    LCD_table.motorCUp     = LCD_table_Buttons.addButton(dW - 2 * (dist + bW),   dist,            bW, bH, "C+");
    LCD_table.motorCDown   = LCD_table_Buttons.addButton(dW - dist - bW,         dist,            bW, bH, "C-");
    LCD_table.measureD     = LCD_table_Buttons.addButton(dW - 4 * (dist + bW),   dH - dist - bH,  bW, bH, "M.D");
    LCD_table.cornerD      = LCD_table_Buttons.addButton(dW - 3 * (dist + bW),   dH - dist - bH,  bW, bH, "Hit");
    LCD_table.motorDUp     = LCD_table_Buttons.addButton(dW - 2 * (dist + bW),   dH - dist - bH,  bW, bH, "D+");
    LCD_table.motorDDown   = LCD_table_Buttons.addButton(dW - dist - bW,         dH - dist - bH,  bW, bH, "D-");
    LCD_table.motorAllUp   = LCD_table_Buttons.addButton(dW / 2 - bW - dist / 2, dH / 2 - bH / 2, bW, bH, "All+");
    LCD_table.motorAllDown = LCD_table_Buttons.addButton(dW / 2 + dist / 2,      dH / 2 - bH / 2, bW, bH, "All-");
    LCD_table.midPoint     = LCD_table_Buttons.addButton(dW / 2 - (bW+dist) / 2, dH / 2 - 3*bH/2, bW, bH, "MidP");
    LCD_table.level        = LCD_table_Buttons.addButton(dW / 2 + (bW+dist) / 2, dH / 2 - 3*bH/2, bW, bH, "Level");
    LCD_table.gotoMain     = LCD_table_Buttons.addButton(dW / 2 - dist - bW/2,   dH - dist - bH,  bW, bH, "Main");
    LCD_table.microStepsB  = LCD_table_Buttons.addButton(dW / 2 - 2*(dist + bW), dH/2 +bH/2+dist, bW, bH, "Micro");
    LCD_table.smallStepsB  = LCD_table_Buttons.addButton(dW / 2 - dist - bW,     dH/2 +bH/2+dist, bW, bH, "Small");
    LCD_table.largeStepsB  = LCD_table_Buttons.addButton(dW / 2 + dist,          dH/2 +bH/2+dist, bW, bH, "Large");
    LCD_table.buttonsDone = true;
  }
  LCD_table.stepSize = 1.0;
  myGLCD.setColor(0, 0, 100);
  myGLCD.fillRect(0, 0, 799, 479);
  myGLCD.setBackColor(0, 0, 100);
  myGLCD.setColor(255, 255, 0);
  LCD_table_Buttons.drawButtons();
  LCD_table.distA = LCD_table.distB = LCD_table.distC = LCD_table.distD = -1;
  
  return 1;
#if 0
  myGLCD.print("You pressed:", 400, 200);
  myGLCD.setColor(VGA_BLACK);
  myGLCD.setBackColor(VGA_WHITE);
  myGLCD.print(" ---    ", 500, 240);
#endif
}

void lcd_table_dynamic()
{
  const float x1=108, y1=70, x2=708, y2=550;
  const float xmA=45, ymA=140, xmB=45, ymB=740, xmC=920-45, ymC=740, xmD=920-45, ymD=140;
  float fmA, fmB, fmC, fmD;
  pinMode(LASER_BED_Z_PIN, INPUT);
  if (digitalRead(LASER_BED_Z_PIN)) 
    myGLCD.setColor(0, 255, 0);
  else
    myGLCD.setColor(100, 100, 100);

  myGLCD.fillRect(400-20, 0, 400+20, 41);

  while (myTouch.dataAvailable() == true)
  {
    int pressed_button = LCD_table_Buttons.checkButtons();

    if (pressed_button == LCD_table.motorAUp) {
      stepOne(A_motor, LCD_table.stepSize);
    }
    if (pressed_button == LCD_table.motorADown) {
      stepOne(A_motor, -1*LCD_table.stepSize);
    }
    if (pressed_button == LCD_table.motorBUp) {
      stepOne(B_motor, LCD_table.stepSize);
    }
    if (pressed_button == LCD_table.motorBDown) {
      stepOne(B_motor, -1*LCD_table.stepSize);
    }
    if (pressed_button == LCD_table.motorCUp) {
      stepOne(C_motor, LCD_table.stepSize);
    }
    if (pressed_button == LCD_table.motorCDown) {
      stepOne(C_motor, -1*LCD_table.stepSize);
    }
    if (pressed_button == LCD_table.motorDUp) {
      stepOne(D_motor, LCD_table.stepSize);
    }
    if (pressed_button == LCD_table.motorDDown) {
      stepOne(D_motor, -1*LCD_table.stepSize);
    }
    if (pressed_button == LCD_table.motorAllUp) {
      stepAll(LCD_table.stepSize);
    }
    if (pressed_button == LCD_table.motorAllDown) {
      stepAll(-1*LCD_table.stepSize);
    }
    if (pressed_button == LCD_table.microStepsB) {
      LCD_table.stepSize = 0.1;
    }
    if (pressed_button == LCD_table.smallStepsB) {
      LCD_table.stepSize = 1.0;
    }
    if (pressed_button == LCD_table.largeStepsB) {
      LCD_table.stepSize = 10.0;
    }
    if (pressed_button == LCD_table.cornerA) {
      enqueuecommands_P(PSTR("G0 X0 Y0 F10000"));
    }
    if (pressed_button == LCD_table.cornerB) {
      enqueuecommands_P(PSTR("G0 X0 Y550 F10000"));
    }
    if (pressed_button == LCD_table.cornerC) {
      enqueuecommands_P(PSTR("G0 X600 Y550 F10000"));
    }
    if (pressed_button == LCD_table.cornerD) {
      enqueuecommands_P(PSTR("G0 X600 Y0 F10000"));
    }
    if (pressed_button == LCD_table.midPoint) {
      enqueuecommands_P(PSTR("G0 X300 Y275 F10000"));
    }
    if (pressed_button == LCD_table.measureA ||
	pressed_button == LCD_table.measureB ||
	pressed_button == LCD_table.measureC ||
	pressed_button == LCD_table.measureD) {
      float dist;
      int i;
      for (i=0; i < 20*STEPS_MM; i++) {
	stepAllOneUp(true); // Up
	if (digitalRead(LASER_BED_Z_PIN))
	  break;
      }
      dist = i / float(STEPS_MM) + LASER_Z_COMP; //
      for (; i > 0; i--) {
	stepAllOneUp(false); // Down
      }
      if (pressed_button == LCD_table.measureA) {
	LCD_table.distA = dist;
	myGLCD.setColor(0, 255, 0);
	myGLCD.setFont(SixteenSegment40x60);
	myGLCD.print("A:", 0, 200);
      }
      if (pressed_button == LCD_table.measureB) {
	LCD_table.distB = dist;
	myGLCD.setColor(0, 255, 0);
	myGLCD.setFont(SixteenSegment40x60);
	myGLCD.print("B:", 0, 200);
      }
      if (pressed_button == LCD_table.measureC) {
	LCD_table.distC = dist;
	myGLCD.setColor(0, 255, 0);
	myGLCD.setFont(SixteenSegment40x60);
	myGLCD.print("C:", 0, 200);
      }
      if (pressed_button == LCD_table.measureD) {
	LCD_table.distD = dist;
	myGLCD.setFont(SixteenSegment40x60);
	myGLCD.setColor(0, 255, 0);
	myGLCD.print("D:", 0, 200);
      }
      myGLCD.setFont(SixteenSegment40x60);
      myGLCD.printNumF(dist, 2, 70, 100);
      myGLCD.setFont(BigFont);
      float mean = 0.25* (LCD_table.distA + LCD_table.distB + LCD_table.distC + LCD_table.distD);
      myGLCD.setColor(0, 255, 0);
      myGLCD.printNumF(LCD_table.distA, 2, 600, 130);
      myGLCD.printNumF(LCD_table.distB, 2, 600, 160);
      myGLCD.printNumF(LCD_table.distC, 2, 600, 190);
      myGLCD.printNumF(LCD_table.distD, 2, 600, 220);
      myGLCD.printNumF(mean, 2, 600, 100);
      myGLCD.printNumF(mean-LASER_NOZZLE_DISTANCE, 2, 700, 100);
      myGLCD.printNumF(LCD_table.distA - mean, 2, 700, 130);
      myGLCD.printNumF(LCD_table.distB - mean, 2, 700, 160);
      myGLCD.printNumF(LCD_table.distC - mean, 2, 700, 190);
      myGLCD.printNumF(LCD_table.distD - mean, 2, 700, 220);
      float fQ11 = LCD_table.distA;
      float fQ12 = LCD_table.distB;
      float fQ22 = LCD_table.distC; 
      float fQ21 = LCD_table.distD; // Bilinear interpolation
      fmA = 1.0/((x2-x1)*(y2-y1))*(fQ11*(x2-xmA)*(y2-ymA)+fQ21*(xmA-x1)*(y2-ymA)+
				   fQ12*(x2-xmA)*(ymA-y1)+fQ22*(xmA-x1)*(ymA-y1));
      fmB = 1.0/((x2-x1)*(y2-y1))*(fQ11*(x2-xmB)*(y2-ymB)+fQ21*(xmB-x1)*(y2-ymB)+
				   fQ12*(x2-xmB)*(ymB-y1)+fQ22*(xmB-x1)*(ymB-y1));
      fmC = 1.0/((x2-x1)*(y2-y1))*(fQ11*(x2-xmC)*(y2-ymC)+fQ21*(xmC-x1)*(y2-ymC)+
				   fQ12*(x2-xmC)*(ymC-y1)+fQ22*(xmC-x1)*(ymC-y1));
      fmD = 1.0/((x2-x1)*(y2-y1))*(fQ11*(x2-xmD)*(y2-ymD)+fQ21*(xmD-x1)*(y2-ymD)+
				   fQ12*(x2-xmD)*(ymD-y1)+fQ22*(xmD-x1)*(ymD-y1));
      float mean2 = 0.25*(fmA+fmB+fmC+fmD);
      myGLCD.printNumF(mean2, 2, 600, 260);
      myGLCD.printNumF(fmA, 2, 600, 280);
      myGLCD.printNumF(fmB, 2, 600, 300);
      myGLCD.printNumF(fmC, 2, 600, 320);
      myGLCD.printNumF(fmD, 2, 600, 340);
      myGLCD.printNumF(mean2-LASER_NOZZLE_DISTANCE, 2, 700, 260);
      myGLCD.printNumF(fmA - mean2, 2, 700, 280);
      myGLCD.printNumF(fmB - mean2, 2, 700, 300);
      myGLCD.printNumF(fmC - mean2, 2, 700, 320);
      myGLCD.printNumF(fmD - mean2, 2, 700, 340);
      
    }

    if (pressed_button == LCD_table.level) {
      if (LCD_table.distA > 0.0 && LCD_table.distB > 0.0 && 
	  LCD_table.distC > 0.0 && LCD_table.distD > 0.0) {
	float fQ11 = LCD_table.distA;
	float fQ12 = LCD_table.distB;
	float fQ22 = LCD_table.distC; 
	float fQ21 = LCD_table.distD; // Bilinear interpolation
	fmA = 1.0/((x2-x1)*(y2-y1))*(fQ11*(x2-xmA)*(y2-ymA)+fQ21*(xmA-x1)*(y2-ymA)+
				     fQ12*(x2-xmA)*(ymA-y1)+fQ22*(xmA-x1)*(ymA-y1));
	fmB = 1.0/((x2-x1)*(y2-y1))*(fQ11*(x2-xmB)*(y2-ymB)+fQ21*(xmB-x1)*(y2-ymB)+
				     fQ12*(x2-xmB)*(ymB-y1)+fQ22*(xmB-x1)*(ymB-y1));
	fmC = 1.0/((x2-x1)*(y2-y1))*(fQ11*(x2-xmC)*(y2-ymC)+fQ21*(xmC-x1)*(y2-ymC)+
				     fQ12*(x2-xmC)*(ymC-y1)+fQ22*(xmC-x1)*(ymC-y1));
	fmD = 1.0/((x2-x1)*(y2-y1))*(fQ11*(x2-xmD)*(y2-ymD)+fQ21*(xmD-x1)*(y2-ymD)+
				     fQ12*(x2-xmD)*(ymD-y1)+fQ22*(xmD-x1)*(ymD-y1));
	float mean2 = 0.25*(fmA+fmB+fmC+fmD);
	stepAll(mean2 - LASER_NOZZLE_DISTANCE);
	stepOne(A_motor, fmA - mean2);
	stepOne(B_motor, fmB - mean2);
	stepOne(C_motor, fmC - mean2);
	stepOne(D_motor, fmD - mean2);
      } else { // Just measure at current point and set focus distance
	float dist;
	int i;
	for (i=0; i < 20*STEPS_MM; i++) {
	  stepAllOneUp(true); // Up
	  if (digitalRead(LASER_BED_Z_PIN))
	    break;
	}
	dist = i / float(STEPS_MM) + LASER_Z_COMP; //
	stepAll(-LASER_NOZZLE_DISTANCE + LASER_Z_COMP);
      }
    }

    if (pressed_button == LCD_table.gotoMain) {
      LCD_menu = lcd_main_static();
    }
  }
}


byte lcd_align_static()
{
  const int bW = 100, dW = 800;
  const int bH = 80, dH = 480;
  const int dist = 10;
  if (!LCD_align.buttonsDone) {
    LCD_align.gotoMain = LCD_align_Buttons.addButton(dW / 2 - dist - bW/2,     dH - dist - bH,  bW, bH, " Main ");
    LCD_align.corner1  = LCD_align_Buttons.addButton(dist,                     dH - dist - bH,  bW, bH, "Go here");
    LCD_align.corner2  = LCD_align_Buttons.addButton(dist,                     dist,            bW, bH, "Go here");
    LCD_align.corner3  = LCD_align_Buttons.addButton(dW - 1 * (dist + bW),     dist,            bW, bH, "Go here");
    LCD_align.corner4  = LCD_align_Buttons.addButton(dW - 1 * (dist + bW),     dH - dist - bH,  bW, bH, "Go here");
    LCD_align.Fire     = LCD_align_Buttons.addButton(dW / 2 - bW/2 - dist / 2, dH / 2 - bH / 2, bW, bH, "Fire!");
    LCD_align.buttonsDone = true;
  }
  myGLCD.setColor(0, 100, 0);
  myGLCD.fillRect(0, 0, 799, 479);
  myGLCD.setBackColor(0, 100, 0);
  myGLCD.setColor(255, 0, 255);
  LCD_align_Buttons.drawButtons();  
  return 2;
}


void lcd_align_dynamic()
{
  while (myTouch.dataAvailable() == true)
  {
    int pressed_button = LCD_align_Buttons.checkButtons();

    if (pressed_button == LCD_align.gotoMain) {
      LCD_menu = lcd_main_static();
    }
    if (pressed_button == LCD_align.corner1) {
      enqueuecommands_P(PSTR("G0 X0 Y0 F10000"));
    }
    if (pressed_button == LCD_align.corner2) {
      enqueuecommands_P(PSTR("G0 X0 Y550 F10000"));
    }
    if (pressed_button == LCD_align.corner3) {
      enqueuecommands_P(PSTR("G0 X600 Y550 F10000"));
    }
    if (pressed_button == LCD_align.corner4) {
      enqueuecommands_P(PSTR("G0 X600 Y0 F10000"));
    }
    if (pressed_button == LCD_align.Fire) {
      enqueuecommands_P(PSTR("M651"));
    }
  }
}

void lcd_update()
{
  extern bool laserUpdateLCD;
  if (!laserUpdateLCD || LCD_doingRaster)
    return;
  switch (LCD_menu) {
  case 0:
    lcd_main_dynamic();
    break;
  case 1:
    lcd_table_dynamic();
    break;
  case 2:
    lcd_align_dynamic();
    break;
  }
}

int pulseWidthMicros = 5;  // microseconds
int microsbetweenSteps = 100; // microeconds

void stepOne(int m, float mm)
{
  boolean up = mm > 0.0;
  int pulses = STEPS_MM * fabs(mm);
  digitalWrite(Motors[m].DirectionPin, up ? HIGH : LOW);
  for (int n = 0; n < pulses; n++) {
    digitalWrite(Motors[m].StepPin, HIGH);
    delayMicroseconds(pulseWidthMicros); // this line is probably unnecessary
    digitalWrite(Motors[m].StepPin, LOW);
    delayMicroseconds(microsbetweenSteps);
  }
}

void stepOneOneUp(int m, boolean up) // Take one step up
{
  digitalWrite(Motors[m].DirectionPin, up ? HIGH : LOW);
  digitalWrite(Motors[m].StepPin, HIGH);
  delayMicroseconds(pulseWidthMicros); // this line is probably unnecessary
  digitalWrite(Motors[m].StepPin, LOW);
  delayMicroseconds(microsbetweenSteps);
}

void stepAllOneUp(boolean up)
{
  digitalWrite(Motors[A_motor].DirectionPin, up ? HIGH : LOW);
  digitalWrite(Motors[B_motor].DirectionPin, up ? HIGH : LOW);
  digitalWrite(Motors[C_motor].DirectionPin, up ? HIGH : LOW);
  digitalWrite(Motors[D_motor].DirectionPin, up ? HIGH : LOW);
  for (uint8_t m = A_motor; m <= D_motor; m++)
    digitalWrite(Motors[m].StepPin, HIGH);
  delayMicroseconds(pulseWidthMicros); // this line is probably unnecessary
  for (uint8_t m = A_motor; m <= D_motor; m++)
    digitalWrite(Motors[m].StepPin, LOW);
  delayMicroseconds(microsbetweenSteps);
}


void stepAll(float mm)
{
  boolean up = mm > 0.0;
  int pulses = STEPS_MM * fabs(mm);
  digitalWrite(Motors[A_motor].DirectionPin, up ? HIGH : LOW);
  digitalWrite(Motors[B_motor].DirectionPin, up ? HIGH : LOW);
  digitalWrite(Motors[C_motor].DirectionPin, up ? HIGH : LOW);
  digitalWrite(Motors[D_motor].DirectionPin, up ? HIGH : LOW);
  for (int n = 0; n < pulses; n++) {
    for (uint8_t m = A_motor; m <= D_motor; m++)
      digitalWrite(Motors[m].StepPin, HIGH);
    delayMicroseconds(pulseWidthMicros); // this line is probably unnecessary
    for (uint8_t m = A_motor; m <= D_motor; m++)
      digitalWrite(Motors[m].StepPin, LOW);
    delayMicroseconds(microsbetweenSteps);
  }
}

void lcd_setstatus(const char* message, const bool persist){}
void lcd_setstatuspgm(const char* message, const uint8_t level){}
bool lcd_hasstatus(){}
void lcd_setalertstatuspgm(const char* message){}
void lcd_reset_alert_level(){}
void lcd_buttons_update(){}


#endif
