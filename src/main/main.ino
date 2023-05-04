/*
Код для mega 2560
Раздельное сверление и управление шаговиками
*/

#include "GyverStepper2.h"
#include "GyverButton.h"
#include "GyverTimers.h"
// #include <LiquidCrystal.h>
#include <LiquidCrystal_I2C.h>
#include "EepromCell.h"
#include <EncButton2.h>

// LiquidCrystal lcd(7, 6, 5, 4, 3, 2);
LiquidCrystal_I2C lcd(0x3F, 16, 2);

GStepper2<STEPPER2WIRE> stepperL(3200, 8, 10, 9);    // Левый мотор
GStepper2<STEPPER2WIRE> stepperR(3200, 18, 19, 17);  // Правый мотор

#define BTNL 15      // PIN - для левой кнопки
#define BTNE 40      // PIN - для ОК
#define BTNR 11      // PIN - для правой кнопки
#define ENDCAP_L 14  // PIN - для левого концевика
#define ENDCAP_R 16  // PIN - для правого коцневика
// #define SpindleS 20  // PIN - для реле шпинделя---
#define SpindleLEFT 30   // PIN - для реле шпинделя
#define SpindleRIGHT 31  // PIN - для реле шпинделя
#define LedStart 25      // PIN - Подсветка START
#define LedStop 24       // PIN - Подсветка STOP
#define LockMenu 12      // PIN - Свитч для блокировки части меню

GButton StartButton(BTNL);
GButton Enter(BTNE);
GButton StopButton(BTNR);

EncButton2<EB_ENCBTN> enc(INPUT_PULLUP, 17, 22, 23);  // энкодер с кнопкой

EepromCell Acceleration(500, 0);           // Ускорения
EepromCell MaxSpeed(500, 5);               // Скорость
EepromCell depthL(100, 10);                // Глубина сверления левый
EepromCell depthR(100, 15);                // Глубина сверления правый
EepromCell MaxReturnSpeed(500, 20);        // Скорость возврата после сверления
EepromCell Tmr_err(10000, 24);             // Время поиска концевика до ошибки
EepromCell SpindleBlock(1, 30);            // Включение концевика
EepromCell parkingErrorDistance(200, 45);  // Дистанция отжатия концевика мотора
EepromCell parkingSpeed(100, 50);          // Скорость парковки
EepromCell disSteps(10, 55);               // Кол-во шагов на один миллиметр
EepromCell offsetDrilingLeft(0, 60);
EepromCell offsetDrilingRight(0, 65);

bool stepperBreakeFlag;
bool stepperEndCapL;
bool stepperEndCapR;
bool EndCapStatL;
bool EndCapStatR;
bool SpindleLeft;   // Включение шпинделя
bool SpindleRight;  // Включение шпинделя
bool SpindleStat;
bool dirBoolL;
bool dirBoolR;

bool leftEngineParking = 0;
bool rightEngineParking = 0;
bool leftEngineDoneDrilling = 0;
bool rightEngineDoneDrilling = 0;
bool stopFlag = 0;
bool stopParkingMode = 0;


bool StartFlag = false;

char WorkID;

uint32_t LcdRef;  // Таймеры
uint32_t Tmr;     // Таймеры
uint32_t BtnTmr;  // Таймеры
bool buttonBlink = 0;

char const n = 5;
int steps[n] = { 10, 50, 100, 500, 1000 };
int stepIndex;
int ErrNum;
char const NumMenu = 12;
int NumMenuLock;
int CurrentMenu = 0;
const char *NameMenu[NumMenu] = {
  "Depth L",
  "Depth R",
  "Acceleration",
  "Speed",
  "Offset L",
  "Offset R",
  "Return speed",
  "Turning spindle",
  "Error timer set",
  "Parking speed",
  "Error dis. park.",
  "= Steps / Dis."
};
char const NumStat = 10;
const char *Stat[NumStat] = {
  "P L",   // 0
  "P R",   // 1
  "DRIL",  // 2
  "BK L",  // 3
  "PB L",  // 4
  "DR R",  // 5
  "BK R",  // 6
  "DONE",  // 7
  "ERR",   // 8
  "ERR"    // 9
};
char const OnOff = 2;
const char *StatBool[OnOff] = {
  "OFF",
  "ON"
};
enum Mode {
  Drilling,
  Wait,
  Error,
  Menu
};
enum ModeMenu {
  DepthL,
  DepthR,
  Accel,
  Speed,
  OffsetLeft,
  OffsetRight,
  ReSpeed,
  SpinldeB,
  TimerError,
  ParkingSpeed,
  ParkingErrorDistance,
  DisSteps,
  NoMode
};

Mode mode = Wait;
ModeMenu modemenu = NoMode;

void setup() {

  // Serial.begin(9600);

  // lcd.begin(16, 2);
  lcd.init();
  lcd.backlight();


  pinMode(ENDCAP_L, INPUT_PULLUP);
  pinMode(ENDCAP_R, INPUT_PULLUP);
  pinMode(LockMenu, INPUT_PULLUP);
  pinMode(SpindleLEFT, OUTPUT);
  pinMode(SpindleRIGHT, OUTPUT);
  pinMode(LedStart, OUTPUT);
  pinMode(LedStop, OUTPUT);

  //  Acceleration.setValue(0);
  //  MaxSpeed.setValue(0);
  //  MaxReturnSpeed.setValue(0);
  //  depthL.setValue(0);
  //  depthR.setValue(0);
  //  SpindleBlock.setValue(0);
  //  Tmr_err.setValue(0);
  //  parkingSpeed.setValue(0);
  //
  //  Acceleration.write();
  //  MaxSpeed.write();
  //  MaxReturnSpeed.write();
  //  depthL.write();
  //  depthR.write();
  //  SpindleBlock.write();
  //  Tmr_err.write();
  //  parkingSpeed.write();

  Acceleration.read();
  MaxSpeed.read();
  MaxReturnSpeed.read();
  depthL.read();
  depthR.read();
  SpindleBlock.read();
  Tmr_err.read();
  parkingSpeed.read();
  parkingErrorDistance.read();
  disSteps.read();
  offsetDrilingLeft.read();
  offsetDrilingRight.read();

  WorkID = 0;

  Tmr = millis();

  stepperBreakeFlag = false;
  Timer2.setPeriod(50);
  Timer2.enableISR();
}
ISR(TIMER2_A) {
  if (!stopFlag) {
    stepperL.tick();
    stepperR.tick();
    digitalWrite(LedStop, false);
  } else {
    digitalWrite(LedStop, true);
    digitalWrite(LedStart, false);
    digitalWrite(SpindleLEFT, true);
    digitalWrite(SpindleRIGHT, true);
  }
}
void drilling() {




  if (millis() - LcdRef >= 200) {

    LcdRef = millis();
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("L");

    lcd.setCursor(1, 0);
    lcd.print(stepperL.pos / disSteps.value() - offsetDrilingLeft.value());

    lcd.setCursor(6, 0);
    lcd.print("R");

    lcd.setCursor(7, 0);
    lcd.print(stepperR.pos / disSteps.value() - offsetDrilingRight.value());

    lcd.setCursor(0, 1);
    lcd.print("L");

    lcd.setCursor(1, 1);
    lcd.print(depthL.value());

    lcd.setCursor(6, 1);
    lcd.print("R");

    lcd.setCursor(7, 1);
    lcd.print(depthR.value());

    lcd.setCursor(12, 1);
    lcd.print(Stat[WorkID]);

    lcd.setCursor(14, 0);
    lcd.print(EndCapStatL);

    lcd.setCursor(15, 0);
    lcd.print(EndCapStatR);

    lcd.setCursor(12, 0);
    lcd.print(SpindleLeft);

    lcd.setCursor(13, 0);
    lcd.print(SpindleRight);
  }

  switch (WorkID) {
    case 0:  //  Парковка левого двигателя
      if (digitalRead(ENDCAP_L)) {
        WorkID = 1;
        Tmr = millis();
        stepperL.brake();
        stepperL.reset();
      } else {
        stepperL.enable();
        stepperL.setSpeed((parkingSpeed.value() * disSteps.value()));

        // if (millis() - Tmr >= Tmr_err.value()) {
        //   WorkID = 8;
        //   ErrNum = 0;
        // }
      }
      if (Enter.isSingle() || enc.held() || enc.click() || StopButton.isSingle()) {
        WorkID = 8;
        ErrNum = 9;
      }
      break;

    case 1:  //  Парковка правого двигателя
      if (digitalRead(ENDCAP_R)) {
        if (stopParkingMode == 0) {
          WorkID = 2;
        } else {
          stopParkingMode = 0;
          WorkID = 7;
        }
        stepperR.brake();
        stepperR.reset();
      } else {
        stepperR.enable();
        stepperR.setSpeed((parkingSpeed.value() * disSteps.value()));

        // if (millis() - Tmr >= Tmr_err.value()) {
        //   WorkID = 8;
        //   ErrNum = 1;
        // }
      }
      if (Enter.isSingle() || enc.held() || enc.click() || StopButton.isSingle()) {
        WorkID = 8;
        ErrNum = 9;
      }
      break;

    case 2:  //  Сверление

      digitalWrite(LedStart, true);
      if ((digitalRead(ENDCAP_L)) && (((stepperL.pos)) > (parkingErrorDistance.value() * disSteps.value()))) {
        WorkID = 8;
        ErrNum = 2;
      } else if ((digitalRead(ENDCAP_R)) && (((stepperR.pos)) > (parkingErrorDistance.value() * disSteps.value()))) {
        WorkID = 8;
        ErrNum = 3;
      }

      if (leftEngineDoneDrilling == 0) {
        SpindleLeft = true;
        stepperL.enable();
        stepperL.setAcceleration(Acceleration.value() * disSteps.value());

        if(stepperL.pos / disSteps.value() < offsetDrilingLeft.value()) {
          stepperL.setMaxSpeed(MaxReturnSpeed.value() * disSteps.value());
          stepperL.setTarget((depthL.value() * disSteps.value()) + offsetDrilingLeft.value());
        } else {
          stepperL.setMaxSpeed(MaxSpeed.value() * disSteps.value());
          stepperL.setTarget((depthL.value() * disSteps.value()) + offsetDrilingLeft.value());
        }

      }
      if (rightEngineDoneDrilling == 0) {
        SpindleRight = true;
        stepperR.enable();
        stepperR.setAcceleration(Acceleration.value() * disSteps.value());

        if(stepperR.pos / disSteps.value() < offsetDrilingRight.value()) {
          stepperR.setMaxSpeed(MaxReturnSpeed.value() * disSteps.value());
          stepperR.setTarget((depthR.value() * disSteps.value()) + offsetDrilingRight.value());
        } else {
          stepperR.setMaxSpeed(MaxSpeed.value() * disSteps.value());
          stepperR.setTarget((depthR.value() * disSteps.value()) + offsetDrilingRight.value());
        }
      }

      if ((stepperL.pos) >= (depthL.value() * disSteps.value()) + offsetDrilingLeft.value()) {
        leftEngineDoneDrilling = 1;
        SpindleLeft = false;
        Tmr = millis();
      }
      if ((stepperR.pos) >= (depthR.value() * disSteps.value()) + offsetDrilingRight.value()) {
        SpindleRight = false;
        rightEngineDoneDrilling = 1;
        Tmr = millis();
      }

      if (leftEngineDoneDrilling == 1 && leftEngineParking == 0) {  // Возврат левого

        stepperL.setAcceleration(Acceleration.value() * disSteps.value());
        stepperL.setMaxSpeed(MaxReturnSpeed.value() * disSteps.value());
        // stepperL.setTarget((depthL.value() * disSteps.value()));

        if ((stepperL.pos) <= ((depthL.value() * disSteps.value()) + offsetDrilingLeft.value())) {
          if (digitalRead(ENDCAP_L)) {
            leftEngineParking = 1;
            Tmr = millis();
            stepperL.brake();
            stepperL.reset();
          } else {
            stepperL.enable();
            stepperL.setSpeed((parkingSpeed.value() * disSteps.value()));
            // if (millis() - Tmr >= Tmr_err.value()) {
            //   WorkID = 8;
            //   ErrNum = 4;
            // }
          }
        }
      }

      if (rightEngineDoneDrilling == 1 && rightEngineParking == 0) {  // Возврат правого

        stepperR.setAcceleration(Acceleration.value() * disSteps.value());
        stepperR.setMaxSpeed(MaxReturnSpeed.value() * disSteps.value());
        // stepperR.setTarget((depthR.value() * disSteps.value()));

        if ((stepperR.pos) <= ((depthR.value() * disSteps.value()) + offsetDrilingRight.value())) {
          if (digitalRead(ENDCAP_R)) {
            rightEngineParking = 1;
            stepperR.setTarget(0);
            stepperR.brake();
            stepperR.reset();
          } else {
            stepperR.enable();
            stepperR.setSpeed((parkingSpeed.value() * disSteps.value()));
            // if (millis() - Tmr >= Tmr_err.value()) {
            //   WorkID = 8;
            //   ErrNum = 5;
            // }
          }
        }
      }

      if ((leftEngineParking == 1) && (rightEngineParking == 1)) {
        WorkID = 7;
        SpindleLeft = false;
        SpindleRight = false;
      }
      // if (enc.held() || enc.click()) {
      //   WorkID = 8;
      //   ErrNum = 9;
      // }
      stopFlag = StopButton.isClick();
      while (stopFlag) {
        StopButton.tick();
        StartButton.tick();
        if (StartButton.isHold()) {
          SpindleLeft = false;
          SpindleRight = false;
          stopParkingMode = 1;
          WorkID = 0;
          stopFlag = 0;
        }
        // stopFlag = !StartButton.isClick();
      }
      break;

    case 7:  //  Завершение операции
      stepperL.disable();
      stepperR.disable();
      stopParkingMode = 0;
      leftEngineDoneDrilling = 0;
      rightEngineDoneDrilling = 0;
      leftEngineParking = 0;
      rightEngineParking = 0;

      SpindleLeft = false;
      SpindleRight = false;

      if (WorkID == 7 && (StartButton.isClick() || enc.click())) {
        WorkID = 0;
        mode = Wait;
      }
      break;

    case 8:  //  Вывод ошибок
      stepperL.brake();
      stepperL.disable();
      stepperR.brake();
      stepperR.disable();
      SpindleLeft = false;
      SpindleRight = false;

      unsigned long ms = millis();
      if (ms - BtnTmr >= 500) {
        BtnTmr = ms;
        if (!buttonBlink) {
          buttonBlink = 1;
        } else
          buttonBlink = 0;
      }
      digitalWrite(LedStart, buttonBlink);

      lcd.setCursor(15, 1);
      lcd.print(ErrNum);

      if (enc.held()) {
        WorkID = 7;
        // mode = Wait;
      }
      break;
  }
}


void wait() {

  if (StartFlag == false) {
    lcd.setCursor(2, 0);
    lcd.print("Drilling MCU");
    lcd.setCursor(1, 1);
    lcd.print("Ver. 1.5.0 JD");
    delay(1500);
    StartFlag = true;
  } else {
    if (millis() - LcdRef >= 200) {
      LcdRef = millis();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("A");

      lcd.setCursor(1, 0);
      lcd.print(Acceleration.value());

      lcd.setCursor(6, 0);
      lcd.print("S");

      lcd.setCursor(7, 0);
      lcd.print(MaxSpeed.value());

      lcd.setCursor(0, 1);
      lcd.print("L");

      lcd.setCursor(1, 1);
      lcd.print(depthL.value());

      lcd.setCursor(6, 1);
      lcd.print("R");

      lcd.setCursor(7, 1);
      lcd.print(depthR.value());

      lcd.setCursor(12, 1);
      lcd.print("WAIT");

      lcd.setCursor(14, 0);
      lcd.print(EndCapStatL);

      lcd.setCursor(15, 0);
      lcd.print(EndCapStatR);

      lcd.setCursor(12, 0);
      lcd.print(SpindleLeft);

      lcd.setCursor(13, 0);
      lcd.print(SpindleRight);
    }

    stepperL.disable();
    stepperR.disable();
    SpindleLeft = false;
    SpindleRight = false;

    digitalWrite(LedStart, false);

    if (enc.click()) {
      mode = Menu;
    }
    if (StartButton.isHold()) {
      mode = Drilling;
      Tmr = millis();
    }
  }
}
void error() {
}
void menu() {

  switch (modemenu) {
    case DepthL:
      depthFL();
      break;
    case DepthR:
      depthFR();
      break;
    case Accel:
      accel();
      break;
    case Speed:
      speedF();
      break;
    case ReSpeed:
      reSpeed();
      break;
    case SpinldeB:
      spinldeB();
      break;
    case TimerError:
      timerError();
      break;
    case ParkingSpeed:
      parkingSpeedF();
      break;
    case ParkingErrorDistance:
      parkingErrDis();
      break;
    case DisSteps:
      dissteps();
      break;
    case NoMode:
      if (millis() - LcdRef >= 200) {
        LcdRef = millis();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(NameMenu[CurrentMenu]);

        switch (CurrentMenu) {
          case DepthL:
            lcd.setCursor(0, 1);
            lcd.print(depthL.value());

            lcd.setCursor(12, 1);
            lcd.print("mm");
            break;
          case DepthR:
            lcd.setCursor(0, 1);
            lcd.print(depthR.value());

            lcd.setCursor(12, 1);
            lcd.print("mm");
            break;
          case Accel:
            lcd.setCursor(0, 1);
            lcd.print(Acceleration.value());

            lcd.setCursor(12, 1);
            lcd.print("mm/s");
            break;
          case Speed:
            lcd.setCursor(0, 1);
            lcd.print(MaxSpeed.value());

            lcd.setCursor(12, 1);
            lcd.print("mm/s");
            break;
          case OffsetLeft:
            lcd.setCursor(0, 1);
            lcd.print(offsetDrilingLeft.value());

            lcd.setCursor(12, 1);
            lcd.print("mm");
            break;
          case OffsetRight:
            lcd.setCursor(0, 1);
            lcd.print(offsetDrilingRight.value());

            lcd.setCursor(12, 1);
            lcd.print("mm");
            break;
          case ReSpeed:
            lcd.setCursor(0, 1);
            lcd.print(MaxReturnSpeed.value());

            lcd.setCursor(12, 1);
            lcd.print("mm/s");
            break;
          case SpinldeB:
            lcd.setCursor(0, 1);
            lcd.print(StatBool[SpindleBlock.value()]);
            break;
          case TimerError:
            lcd.setCursor(0, 1);
            lcd.print(Tmr_err.value() / 1000);

            lcd.setCursor(12, 1);
            lcd.print("S");
            break;
          case ParkingSpeed:
            lcd.setCursor(0, 1);
            lcd.print(parkingSpeed.value());

            lcd.setCursor(12, 1);
            lcd.print("mm/s");
            break;
          case ParkingErrorDistance:
            lcd.setCursor(0, 1);
            lcd.print(parkingErrorDistance.value());

            lcd.setCursor(12, 1);
            lcd.print("mm");
            break;
          case DisSteps:
            lcd.setCursor(0, 1);
            lcd.print(disSteps.value());

            lcd.setCursor(12, 1);
            lcd.print("s/mm");
        }
      }
      if ((enc.right())) {
        if (CurrentMenu < NumMenuLock - 1)
          CurrentMenu = CurrentMenu + 1;
        else
          CurrentMenu = 0;
      } else if (enc.left()) {
        if (CurrentMenu != 0)
          CurrentMenu = CurrentMenu - 1;
        else
          CurrentMenu = NumMenuLock - 1;
      }
      if (Enter.isSingle() || (enc.click())) {
        modemenu = ModeMenu(CurrentMenu);
      }
      if (Enter.isDouble() || (enc.step())) {
        mode = Wait;
      }
      break;
  }
}
void depthFL() {

  if (millis() - LcdRef >= 200) {
    LcdRef = millis();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(depthL.value());
    lcd.setCursor(14, 0);
    lcd.print("mm");
    lcd.setCursor(10, 1);
    lcd.print(steps[stepIndex]);
    depthL.write();
  }
  if ((enc.right())) {
    depthL.setValue(depthL.value() + (steps[stepIndex]));
  } else if (enc.left()) {
    depthL.setValue(depthL.value() - (steps[stepIndex]));
  }
  if (depthL.value() < 0) depthL.setValue(0);
  if (Enter.isSingle() || (enc.click())) {
    if (stepIndex < n - 1)
      stepIndex = stepIndex + 1;
    else
      stepIndex = 0;
  }
  if (Enter.isHold() || (enc.hold())) {
    modemenu = NoMode;
  }
}
void depthFR() {

  if (millis() - LcdRef >= 200) {
    LcdRef = millis();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(depthR.value());
    lcd.setCursor(14, 0);
    lcd.print("mm");
    lcd.setCursor(10, 1);
    lcd.print(steps[stepIndex]);
    depthR.write();
  }
  if (enc.right()) {
    depthR.setValue(depthR.value() + (steps[stepIndex]));
  } else if (enc.left()) {
    depthR.setValue(depthR.value() - (steps[stepIndex]));
  }
  if (depthR.value() < 0) {
    depthR.setValue(0);
  }
  if (Enter.isSingle() || (enc.click())) {
    if (stepIndex < n - 1)
      stepIndex = stepIndex + 1;
    else
      stepIndex = 0;
  }
  if (Enter.isHold() || (enc.hold())) {
    modemenu = NoMode;
  }
}
void accel() {

  if (millis() - LcdRef >= 200) {
    LcdRef = millis();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(Acceleration.value());
    lcd.setCursor(12, 0);
    lcd.print("mm/s");
    lcd.setCursor(10, 1);
    lcd.print(steps[stepIndex]);
    Acceleration.write();
  }
  if ((enc.right())) {
    Acceleration.setValue(Acceleration.value() + steps[stepIndex]);
  } else if ((enc.left())) {
    Acceleration.setValue(Acceleration.value() - steps[stepIndex]);
  }
  if (Acceleration.value() < 10) Acceleration.setValue(10);
  if (Enter.isSingle() || (enc.click())) {
    if (stepIndex < n - 1)
      stepIndex = stepIndex + 1;
    else
      stepIndex = 0;
  }
  if (Enter.isHold() || (enc.hold())) {
    modemenu = NoMode;
  }
}
void speedF() {

  if (millis() - LcdRef >= 200) {
    LcdRef = millis();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(MaxSpeed.value());
    lcd.setCursor(12, 0);
    lcd.print("mm/s");
    lcd.setCursor(10, 1);
    lcd.print(steps[stepIndex]);
    MaxSpeed.write();
  }
  if ((enc.right())) {
    MaxSpeed.setValue(MaxSpeed.value() + (steps[stepIndex]));
  } else if ((enc.left())) {
    MaxSpeed.setValue(MaxSpeed.value() - (steps[stepIndex]));
  }
  if (MaxSpeed.value() < 10) MaxSpeed.setValue(10);
  if (Enter.isSingle() || (enc.click())) {
    if (stepIndex < n - 1)
      stepIndex = stepIndex + 1;
    else
      stepIndex = 0;
  }
  if (Enter.isHold() || (enc.hold())) {
    modemenu = NoMode;
  }
}
void offsetLeft() {

  if (millis() - LcdRef >= 200) {
    LcdRef = millis();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(offsetDrilingLeft.value());
    lcd.setCursor(12, 0);
    lcd.print("mm");
    lcd.setCursor(10, 1);
    lcd.print(steps[stepIndex]);
    offsetDrilingLeft.write();
  }
  if ((enc.right())) {
    offsetDrilingLeft.setValue(offsetDrilingLeft.value() + (steps[stepIndex]));
  } else if ((enc.left())) {
    offsetDrilingLeft.setValue(offsetDrilingLeft.value() - (steps[stepIndex]));
  }
  if (Enter.isSingle() || (enc.click())) {
    if (stepIndex < n - 1)
      stepIndex = stepIndex + 1;
    else
      stepIndex = 0;
  }
  if (Enter.isHold() || (enc.hold())) {
    modemenu = NoMode;
  }
}
void offsetRight() {

  if (millis() - LcdRef >= 200) {
    LcdRef = millis();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(offsetDrilingRight.value());
    lcd.setCursor(12, 0);
    lcd.print("mm");
    lcd.setCursor(10, 1);
    lcd.print(steps[stepIndex]);
    MaxSpeed.write();
  }
  if ((enc.right())) {
    offsetDrilingRight.setValue(offsetDrilingRight.value() + (steps[stepIndex]));
  } else if ((enc.left())) {
    offsetDrilingRight.setValue(offsetDrilingRight.value() - (steps[stepIndex]));
  }
  if (Enter.isSingle() || (enc.click())) {
    if (stepIndex < n - 1)
      stepIndex = stepIndex + 1;
    else
      stepIndex = 0;
  }
  if (Enter.isHold() || (enc.hold())) {
    modemenu = NoMode;
  }
}

void reSpeed() {

  if (millis() - LcdRef >= 200) {
    LcdRef = millis();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(MaxReturnSpeed.value());
    lcd.setCursor(12, 0);
    lcd.print("mm/s");
    lcd.setCursor(10, 1);
    lcd.print(steps[stepIndex]);
    MaxReturnSpeed.write();
  }
  if ((enc.right())) {
    MaxReturnSpeed.setValue(MaxReturnSpeed.value() + (steps[stepIndex]));
  } else if ((enc.left())) {
    MaxReturnSpeed.setValue(MaxReturnSpeed.value() - (steps[stepIndex]));
  }
  if (MaxReturnSpeed.value() < 10) MaxReturnSpeed.setValue(10);
  if (Enter.isSingle() || (enc.click())) {
    if (stepIndex < n - 1)
      stepIndex = stepIndex + 1;
    else
      stepIndex = 0;
  }
  if (Enter.isHold() || (enc.hold())) {
    modemenu = NoMode;
  }
}
void spinldeB() {

  if (millis() - LcdRef >= 200) {
    LcdRef = millis();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(StatBool[SpindleBlock.value()]);
    SpindleBlock.write();
  }
  if ((enc.right())) {
    SpindleBlock.setValue(false);
  } else if ((enc.left())) {
    SpindleBlock.setValue(true);
  }

  if (Enter.isHold() || (enc.hold())) {
    modemenu = NoMode;
  }
}
void timerError() {

  if (millis() - LcdRef >= 200) {
    LcdRef = millis();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(Tmr_err.value() / 1000);
    lcd.setCursor(12, 0);
    lcd.print("S");
    lcd.setCursor(10, 1);
    lcd.print((steps[stepIndex] / 100));
    Tmr_err.write();
  }
  if ((enc.right())) {
    Tmr_err.setValue(Tmr_err.value() + (steps[stepIndex] * 10));
  } else if ((enc.left())) {
    Tmr_err.setValue(Tmr_err.value() - (steps[stepIndex] * 10));
  }
  if (Tmr_err.value() < 1000) Tmr_err.setValue(1000);
  if (stepIndex < 2) stepIndex = 2;
  if (Enter.isSingle() || (enc.click())) {
    if (stepIndex < n - 1)
      stepIndex = stepIndex + 1;
    else
      stepIndex = 2;
  }
  if (Enter.isHold() || (enc.hold())) {
    modemenu = NoMode;
  }
}
void parkingSpeedF() {

  if (millis() - LcdRef >= 200) {
    LcdRef = millis();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(parkingSpeed.value());
    lcd.setCursor(12, 0);
    lcd.print("mm/s");
    lcd.setCursor(10, 1);
    lcd.print((steps[stepIndex]));
    parkingSpeed.write();
  }
  if ((enc.right())) {
    parkingSpeed.setValue(parkingSpeed.value() + (steps[stepIndex]));
  } else if ((enc.left())) {
    parkingSpeed.setValue(parkingSpeed.value() - (steps[stepIndex]));
  }
  if (parkingSpeed.value() < 10) parkingSpeed.setValue(10);
  if (Enter.isSingle() || (enc.click())) {
    if (stepIndex < n - 1)
      stepIndex = stepIndex + 1;
    else
      stepIndex = 0;
  }
  if (Enter.isHold() || (enc.hold())) {
    modemenu = NoMode;
  }
}
// void dirLF() {

//   if (millis() - LcdRef >= 200) {
//     LcdRef = millis();
//     lcd.clear();
//     lcd.setCursor(0, 0);
//     lcd.print(StatBool[dirBoolL]);
//     dirL.write();
//   }
//   if ((enc.right())) {
//     dirL.setValue(1);
//   } else if ((enc.left())) {
//     dirL.setValue(-1);
//   }

//   if (Enter.isHold() || (enc.hold())) {
//     modemenu = NoMode;
//   }
// }
// void dirRF() {

//   if (millis() - LcdRef >= 200) {
//     LcdRef = millis();
//     lcd.clear();
//     lcd.setCursor(0, 0);
//     lcd.print(StatBool[dirBoolR]);
//     dirR.write();
//   }
//   if ((enc.right())) {
//     dirR.setValue(1);
//   } else if ((enc.left())) {
//     dirR.setValue(-1);
//   }

//   if (Enter.isHold() || (enc.hold())) {
//     modemenu = NoMode;
//   }
// }
void parkingErrDis() {

  if (millis() - LcdRef >= 200) {
    LcdRef = millis();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(parkingErrorDistance.value());
    lcd.setCursor(12, 0);
    lcd.print("mm");
    lcd.setCursor(10, 1);
    lcd.print((steps[stepIndex]));
    parkingErrorDistance.write();
  }
  if ((enc.right())) {
    parkingErrorDistance.setValue(parkingErrorDistance.value() + (steps[stepIndex]));
  } else if ((enc.left())) {
    parkingErrorDistance.setValue(parkingErrorDistance.value() - (steps[stepIndex]));
  }
  if (parkingErrorDistance.value() < 1) parkingErrorDistance.setValue(1);
  if (Enter.isSingle() || (enc.click())) {
    if (stepIndex < n - 1)
      stepIndex = stepIndex + 1;
    else
      stepIndex = 0;
  }
  if (Enter.isHold() || (enc.hold())) {
    modemenu = NoMode;
  }
}
void dissteps() {

  if (millis() - LcdRef >= 200) {
    LcdRef = millis();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(disSteps.value());
    lcd.setCursor(12, 0);
    lcd.print("s/mm");
    lcd.setCursor(10, 1);
    lcd.print((steps[stepIndex] / 10));
    disSteps.write();
  }
  if ((enc.right())) {
    disSteps.setValue(disSteps.value() + (steps[stepIndex] / 10));
  } else if ((enc.left())) {
    disSteps.setValue(disSteps.value() - (steps[stepIndex] / 10));
  }
  if (disSteps.value() < 1) disSteps.setValue(1);
  if (Enter.isSingle() || (enc.click())) {
    if (stepIndex < n - 1)
      stepIndex = stepIndex + 1;
    else
      stepIndex = 0;
  }
  if (Enter.isHold() || (enc.hold())) {
    modemenu = NoMode;
  }
}
void loop() {

  if (digitalRead(LockMenu)) {
    NumMenuLock = 6;
  } else {
    NumMenuLock = 12;
  }
  StartButton.tick();
  StopButton.tick();
  enc.tick();

  if ((digitalRead(ENDCAP_L) != 0)) {
    EndCapStatL = true;
  } else {
    EndCapStatL = false;
  }

  if ((digitalRead(ENDCAP_R) != 0)) {
    EndCapStatR = true;
  } else {
    EndCapStatR = false;
  }

  if ((SpindleRight == true) && (SpindleBlock.value() == true)) {
    digitalWrite(SpindleRIGHT, false);
    SpindleStat = true;
  } else {
    SpindleStat = false;
    digitalWrite(SpindleRIGHT, true);
  }
  if ((SpindleLeft == true) && (SpindleBlock.value() == true)) {
    digitalWrite(SpindleLEFT, false);
    SpindleStat = true;
  } else {
    SpindleStat = false;
    digitalWrite(SpindleLEFT, true);
  }

  // if (StartButton.isClick()) Serial.println("Button 1");
  // if (StopButton.isClick()) Serial.println("Button 2");
  //if (LimitSwitch.isClick()) Serial.println("Button 3");
  //Serial.println(CurrentMenu);

  switch (mode) {
    case Drilling:
      drilling();
      break;
    case Wait:
      wait();
      break;
    case Error:
      error();
      break;
    case Menu:
      menu();
      break;
  }
}