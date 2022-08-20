/*
Код для mega 2560
Совместное сверление и управление шаговиками, с ожиданием при сверлении
Нет поддержки энкодера
*/
#include "GyverStepper2.h"
#include "GyverButton.h"
#include "GyverTimers.h"
#include <LiquidCrystal.h>
#include "EepromCell.h"

LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

GStepper2<STEPPER2WIRE> stepperL(3200, 8, 10, 9);   // Левый мотор
GStepper2<STEPPER2WIRE> stepperR(3200, 18, 19, 17); // Правый мотор

#define BTNL 15      // PIN - для левой кнопки
#define BTNE 12      // PIN - для ОК
#define BTNR 11      // PIN - для правой кнопки
#define ENDCAP_L 14  // PIN - для левого концевика
#define ENDCAP_R 16  // PIN - для правого коцневика
#define SpindleS 20  // PIN - для реле шпинделя

GButton Left(BTNL);
GButton Enter(BTNE);
GButton Right(BTNR);

EepromCell Acceleration(500, 0);              // Ускорения
EepromCell MaxSpeed(500, 5);                  // Скорость
EepromCell depthL(100, 10);                    // Глубина сверления левый
EepromCell depthR(100, 15);                   // Глубина сверления правый  
EepromCell MaxReturnSpeed(500, 20);           // Скорость возврата после сверления
EepromCell Tmr_err(10000, 24);                // Время поиска концевика до ошибки
EepromCell SpindleBlock(1, 30);               // Включение концевика
EepromCell dirL(1, 35);                       // Направление левого
EepromCell dirR(1, 40);                       // Напрвление правого
EepromCell parkingErrorDistance(200, 45);     // Дистанция отжатия концевика мотора
EepromCell parkingSpeed(100, 50);             // Скорость парковки
EepromCell disSteps(10, 55);                  // Кол-во шагов на один миллиметр

bool stepperBreakeFlag;
bool stepperEndCapL;
bool stepperEndCapR;
bool EndCapStatL;
bool EndCapStatR;
bool Spindle;           // Включение шпинделя
bool SpindleStat;
bool dirBoolL;
bool dirBoolR;

char WorkID;

uint32_t LcdRef;  // Таймеры
uint32_t Tmr;     // Таймеры

char const n = 5;
int steps [n] = {10, 50,  100, 500, 1000};
int stepIndex;
int ErrNum;
char const NumMenu = 12;
int CurrentMenu = 0;
const char *NameMenu[NumMenu] = {
  "Depth L",
  "Depth R",
  "Acceleration",
  "Speed",
  "Return speed",
  "Turning spindle",
  "Error timer set",
  "Parking speed",
  "Inverting dir. L",
  "Inverting dir. R",
  "Error dis. park.",
  "= Steps / Dis."
};
char const NumStat = 9;
const char *Stat[NumStat] = {
  "P L",
  "P R",
  "DRILL",
  "BK L",
  "P BL",
  "BK R",
  "P BR",
  "DONE",
  "ERR"
};
char const OnOff = 2;
const char *StatBool[OnOff] = {
  "OFF",
  "ON"
};
enum Mode
{
  Drilling,
  Wait,
  Error,
  Menu
};
enum ModeMenu
{
  DepthL,
  DepthR,
  Accel,
  Speed,
  ReSpeed,
  SpinldeB,
  TimerError,
  ParkingSpeed,
  DirL,
  DirR,
  ParkingErrorDistance,
  DisSteps,
  NoMode
};

Mode mode = Wait;
ModeMenu modemenu = NoMode;

void setup() {

  Serial.begin(9600);

  lcd.begin(16, 2);

  pinMode(ENDCAP_L, INPUT_PULLUP);
  pinMode(ENDCAP_R, INPUT_PULLUP);
  pinMode(SpindleS, OUTPUT);

//  Acceleration.setValue(0);
//  MaxSpeed.setValue(0);
//  MaxReturnSpeed.setValue(0);
//  depthL.setValue(0);
//  depthR.setValue(0);
//  SpindleBlock.setValue(0);
//  Tmr_err.setValue(0);
//  parkingSpeed.setValue(0);
//  dirL.setValue(0);
//  dirR.setValue(0);
//
//  Acceleration.write();
//  MaxSpeed.write();
//  MaxReturnSpeed.write();
//  depthL.write();
//  depthR.write();
//  SpindleBlock.write();
//  Tmr_err.write();
//  parkingSpeed.write();
//  dirL.write();
//  dirR.write();

  Acceleration.read();
  MaxSpeed.read();
  MaxReturnSpeed.read();
  depthL.read();
  depthR.read();
  SpindleBlock.read();
  Tmr_err.read();
  parkingSpeed.read();
  dirL.read();
  dirR.read();
  parkingErrorDistance.read();
  disSteps.read();

  WorkID = 0;
  
  Tmr = millis();

  stepperBreakeFlag = false;
  Timer2.setPeriod(50);
  Timer2.enableISR();
}
ISR(TIMER2_A) {
  stepperL.tick();
  stepperR.tick();

}
void drilling() {

  if (millis() - LcdRef >= 200) {

    LcdRef = millis();
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("L");

    lcd.setCursor(1, 0);
    lcd.print(stepperL.pos / disSteps.value());

    lcd.setCursor(6, 0);
    lcd.print("R");

    lcd.setCursor(7, 0);
    lcd.print(stepperR.pos / disSteps.value());

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
    lcd.print(SpindleStat);
  }

  if (WorkID == 0) {          //  Парковка левого двигателя

    if (digitalRead(ENDCAP_L)) {
      WorkID = 1;
      Tmr = millis();
      stepperL.brake();
      stepperL.reset();
    } else {
      stepperL.enable();
      stepperL.setSpeed((parkingSpeed.value() * disSteps.value()) * (-dirL.value()));
      
      if (millis() - Tmr >= Tmr_err.value()) {
        WorkID = 8;
        ErrNum = 0;
      }
    }
    if (Enter.isSingle()) {
      WorkID = 8;
      ErrNum = 9;
    }
  }

  if (WorkID == 1) {          //  Парковка правого двигателя

    if (digitalRead(ENDCAP_R)) {
      WorkID = 2;
      stepperR.brake();
      stepperR.reset();
    } else {
      stepperR.enable();
      stepperR.setSpeed((parkingSpeed.value() * disSteps.value()) * (-dirR.value()));

      if (millis() - Tmr >= Tmr_err.value()) {
        WorkID = 8;
        ErrNum = 1;
      }
    }
    if (Enter.isSingle()) {
      WorkID = 8;
      ErrNum = 9;
    }
  }

  if (WorkID == 2) {          //  Сверление

    if ((digitalRead(ENDCAP_L)) && (((stepperL.pos) * dirL.value()) > (parkingErrorDistance.value() * disSteps.value()))) {
      WorkID = 8;
      ErrNum = 2;
    } else if ((digitalRead(ENDCAP_R)) && (((stepperR.pos) * dirR.value()) > (parkingErrorDistance.value() * disSteps.value()))) {
      WorkID = 8;
      ErrNum = 3;
    }

    stepperL.enable();                               
    stepperL.setAcceleration(Acceleration.value() * disSteps.value());
    stepperL.setMaxSpeed(MaxSpeed.value() * disSteps.value());
    stepperL.setTarget(dirL.value() * (depthL.value() * disSteps.value()));

    stepperR.enable();
    stepperR.setAcceleration(Acceleration.value() * disSteps.value());
    stepperR.setMaxSpeed(MaxSpeed.value() * disSteps.value());
    stepperR.setTarget(dirR.value() * (depthR.value() * disSteps.value()));

    Spindle = true; 
    
    if (((stepperL.pos * dirL.value()) >= (depthL.value() * disSteps.value())) && ((stepperR.pos * dirR.value()) >= (depthR.value() * disSteps.value()))) {
      WorkID = 3;
    }
    if (Enter.isSingle()) {
      WorkID = 8;
      ErrNum = 9;
    }
  }

  if (WorkID == 3) {          //  Возврат левого двигателя

    stepperL.setAcceleration(Acceleration.value() * disSteps.value());
    stepperL.setMaxSpeed(MaxReturnSpeed.value() * disSteps.value());
    stepperL.setTarget((((depthL.value() * disSteps.value()) / 5)) * (dirL.value()));
    if (dirL.value() <= 0) {
      if ((stepperL.pos) >= ((((depthL.value() * disSteps.value()) / 5) ) * (dirL.value()))) {
      Tmr = millis();
      WorkID = 4;
      }
    } else {
      if ((stepperL.pos) <= ((((depthL.value() * disSteps.value()) / 5) ) * (dirL.value()))) {
      Tmr = millis();
      WorkID = 4;
      }
    }
    

    if (digitalRead(ENDCAP_L)) {
      stepperL.setTarget(0);
      stepperL.brake();
      stepperL.reset();
      WorkID = 5;
      stepperEndCapL = 0;
    }
    if (Enter.isSingle()) {
      WorkID = 8;
      ErrNum = 9;
    }
  }

  if (WorkID == 4) {          //  Парковка левого двигателя

    if (digitalRead(ENDCAP_L)) {
      WorkID = 5;
      Tmr = millis();
      stepperL.brake();
      stepperL.reset();
    } else {
      stepperL.enable();
      stepperL.setSpeed((parkingSpeed.value() * disSteps.value()) * (-dirL.value()));
      
      if (millis() - Tmr >= Tmr_err.value()) {
        WorkID = 8;
        ErrNum = 4;
      }
    }
    if (Enter.isSingle()) {
      WorkID = 8;
      ErrNum = 9;
    }
  }

  if (WorkID == 5) {          //  Возврат правого двигателя

    stepperR.setAcceleration(Acceleration.value() * disSteps.value());
    stepperR.setMaxSpeed(MaxReturnSpeed.value() * disSteps.value());
    stepperR.setTarget((((depthR.value() * disSteps.value()) / 5) ) * (dirR.value()));

    if (dirR.value() <= 0) {
      if ((stepperR.pos) >= ((((depthR.value() * disSteps.value()) / 5) ) * (dirR.value()))) {
      Tmr = millis();
      WorkID = 6;
      }
    } else {
      if ((stepperR.pos) <= ((((depthR.value() * disSteps.value()) / 5) ) * (dirR.value()))) {
      Tmr = millis();
      WorkID = 6;
      }
    }

    if (digitalRead(ENDCAP_R)) {
      stepperR.setTarget(0);
      stepperR.brake();
      stepperR.reset();
      WorkID = 7;
      stepperEndCapR = 0;
    }
    if (Enter.isSingle()) {
      WorkID = 8;
      ErrNum = 9;
    }
  }

  if (WorkID == 6) {          //  Парковка правого двигателя

   if (digitalRead(ENDCAP_R)) {
      WorkID = 7;
      stepperR.brake();
      stepperR.reset();
    } else {
      stepperR.enable();
      stepperR.setSpeed((parkingSpeed.value() * disSteps.value()) * (-dirR.value()));

      if (millis() - Tmr >= Tmr_err.value()) {
        WorkID = 8;
        ErrNum = 5;
      }
    }
    if (Enter.isSingle()) {
      WorkID = 8;
      ErrNum = 9;
    }
  }

  if (WorkID == 7) {          //  Завершение операции

    stepperL.disable();
    stepperR.disable();
    Spindle = false;

    if ((WorkID == 7) & (Enter.isSingle())) {
      mode = Wait;
      WorkID = 0;
    }
  }

  if (WorkID == 8) {          //  Вывод ошибок

    stepperL.brake();
    stepperL.disable();
    stepperR.brake();
    stepperR.disable();
    Spindle = false;

    lcd.setCursor(15, 1);
    lcd.print(ErrNum);
    
    if (Enter.isDouble()) {
      WorkID = 0;
      mode = Wait;
    }
  }

}
void wait() {

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
    lcd.print(Spindle);
  }

  stepperL.disable();
  stepperR.disable();
  Spindle = false;
  
  if (Enter.isSingle()) {
    mode = Menu;
  }
  if (Enter.isHold()) {
    mode = Drilling;
    Tmr = millis();
  }
}
void error() {

}
void menu() {

  switch (modemenu)
  {
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
      case DirL:
      dirLF();
      break;
    case DirR:
      dirRF();
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
        
        switch (CurrentMenu)
        { 
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
        case DirL:
          lcd.setCursor(0, 1);
          lcd.print(StatBool[dirBoolL]);
          break;
        case DirR:
          lcd.setCursor(0, 1);
          lcd.print(StatBool[dirBoolR]);  
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
      if ((Right.isPress()) || (Right.isStep())) {
        if (CurrentMenu < NumMenu - 1)
          CurrentMenu = CurrentMenu + 1;
        else
          CurrentMenu = 0;
      } else if ((Left.isPress()) || (Left.isStep())) {
        if (CurrentMenu != 0)
          CurrentMenu = CurrentMenu - 1;
        else
          CurrentMenu = NumMenu - 1;
      }
      if (Enter.isSingle()) {
        modemenu = ModeMenu(CurrentMenu);
      }
      if (Enter.isDouble()) {
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
  if ((Right.isPress()) || (Right.isStep())) {
    depthL.setValue(depthL.value() + (steps[stepIndex]));
  } else if ((Left.isPress()) || (Left.isStep())) {
    depthL.setValue(depthL.value() - (steps[stepIndex]));
  }
  if (depthL.value() < 0) depthL.setValue(0);
  if (Enter.isSingle()) {
    if (stepIndex < n - 1)
      stepIndex = stepIndex + 1;
    else
      stepIndex = 0;
  }
  if (Enter.isHold()) {
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
  if ((Right.isPress()) || (Right.isStep())) {
    depthR.setValue(depthR.value() + (steps[stepIndex]));
  } else if ((Left.isPress()) || (Left.isStep())) {
    depthR.setValue(depthR.value() - (steps[stepIndex]));
  }
  if (depthR.value() < 0) depthR.setValue(0);
  if (Enter.isSingle()) {
    if (stepIndex < n - 1)
      stepIndex = stepIndex + 1;
    else
      stepIndex = 0;
  }
  if (Enter.isHold()) {
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
  if ((Right.isPress()) || (Right.isStep())) {
    Acceleration.setValue(Acceleration.value() + steps[stepIndex]);
  } else if ((Left.isPress()) || (Left.isStep())) {
    Acceleration.setValue(Acceleration.value() - steps[stepIndex]);
  }
  if (Acceleration.value() < 10) Acceleration.setValue(10);
  if (Enter.isSingle()) {
    if (stepIndex < n - 1)
      stepIndex = stepIndex + 1;
    else
      stepIndex = 0;
  }
  if (Enter.isHold()) {
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
  if ((Right.isPress()) || (Right.isStep())) {
    MaxSpeed.setValue(MaxSpeed.value() + (steps[stepIndex]));
  } else if ((Left.isPress()) || (Left.isStep())) {
    MaxSpeed.setValue(MaxSpeed.value() - (steps[stepIndex]));
  }
  if (MaxSpeed.value() < 10) MaxSpeed.setValue(10);
  if (Enter.isSingle()) {
    if (stepIndex < n - 1)
      stepIndex = stepIndex + 1;
    else
      stepIndex = 0;
  }
  if (Enter.isHold()) {
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
  if ((Right.isPress()) || (Right.isStep())) {
    MaxReturnSpeed.setValue(MaxReturnSpeed.value() + (steps[stepIndex]));
  } else if ((Left.isPress()) || (Left.isStep())) {
    MaxReturnSpeed.setValue(MaxReturnSpeed.value() - (steps[stepIndex]));
  }
  if (MaxReturnSpeed.value() < 10) MaxReturnSpeed.setValue(10);
  if (Enter.isSingle()) {
    if (stepIndex < n - 1)
      stepIndex = stepIndex + 1;
    else
      stepIndex = 0;
  }
  if (Enter.isHold()) {
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
  if ((Right.isPress()) || (Right.isStep())) {
    SpindleBlock.setValue(false);
  } else if ((Left.isPress()) || (Left.isStep())) {
    SpindleBlock.setValue(true);
  }
  
  if (Enter.isHold()) {
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
  if ((Right.isPress()) || (Right.isStep())) {
    Tmr_err.setValue(Tmr_err.value() + (steps[stepIndex] * 10));
  } else if ((Left.isPress()) || (Left.isStep())) {
    Tmr_err.setValue(Tmr_err.value() - (steps[stepIndex] * 10));
  }
  if (Tmr_err.value() < 1000) Tmr_err.setValue(1000);
  if (stepIndex < 2) stepIndex = 2; 
  if (Enter.isSingle()) {
    if (stepIndex < n - 1)
      stepIndex = stepIndex + 1;
    else
      stepIndex = 2;
  }
  if (Enter.isHold()) {
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
  if ((Right.isPress()) || (Right.isStep())) {
    parkingSpeed.setValue(parkingSpeed.value() + (steps[stepIndex]));
  } else if ((Left.isPress()) || (Left.isStep())) {
    parkingSpeed.setValue(parkingSpeed.value() - (steps[stepIndex]));
  }
  if (parkingSpeed.value() < 10) parkingSpeed.setValue(10);
  if (Enter.isSingle()) {
    if (stepIndex < n - 1)
      stepIndex = stepIndex + 1;
    else
      stepIndex = 0;
  }
  if (Enter.isHold()) {
    modemenu = NoMode;
  }
}
void dirLF() {
  
  if (millis() - LcdRef >= 200) {
    LcdRef = millis();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(StatBool[dirBoolL]);
    dirL.write();
  }
  if ((Right.isPress()) || (Right.isStep())) {
    dirL.setValue(1);
  } else if ((Left.isPress()) || (Left.isStep())) {
    dirL.setValue(-1);
  }
  
  if (Enter.isHold()) {
    modemenu = NoMode;
  }
}
void dirRF() {
  
  if (millis() - LcdRef >= 200) {
    LcdRef = millis();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(StatBool[dirBoolR]);
    dirR.write();
  }
  if ((Right.isPress()) || (Right.isStep())) {
    dirR.setValue(1);
  } else if ((Left.isPress()) || (Left.isStep())) {
    dirR.setValue(-1);
  }
  
  if (Enter.isHold()) {
    modemenu = NoMode;
  }
}
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
  if ((Right.isPress()) || (Right.isStep())) {
    parkingErrorDistance.setValue(parkingErrorDistance.value() + (steps[stepIndex]));
  } else if ((Left.isPress()) || (Left.isStep())) {
    parkingErrorDistance.setValue(parkingErrorDistance.value() - (steps[stepIndex]));
  }
  if (parkingErrorDistance.value() < 1) parkingErrorDistance.setValue(1); 
  if (Enter.isSingle()) {
    if (stepIndex < n - 1)
      stepIndex = stepIndex + 1;
    else
      stepIndex = 0;
  }
  if (Enter.isHold()) {
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
  if ((Right.isPress()) || (Right.isStep())) {
    disSteps.setValue(disSteps.value() + (steps[stepIndex] / 10));
  } else if ((Left.isPress()) || (Left.isStep())) {
    disSteps.setValue(disSteps.value() - (steps[stepIndex] / 10));
  }
  if (disSteps.value() < 1) disSteps.setValue(1);
  if (Enter.isSingle()) {
    if (stepIndex < n - 1)
      stepIndex = stepIndex + 1;
    else
      stepIndex = 0;
  }
  if (Enter.isHold()) {
    modemenu = NoMode;
  }
}
void loop() {

  Left.tick();
  Right.tick();
  Enter.tick();
  
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

  if (dirL.value() <= 0) {
    dirBoolL = 0; 
  } else {
    dirBoolL = 1;
  }

  if (dirR.value() <= 0) {
    dirBoolR = 0; 
  } else {
    dirBoolR = 1;
  }

  if ((Spindle == true) && (SpindleBlock.value() == true)) {
    digitalWrite(SpindleS, true);
    SpindleStat = Spindle;
  } else {
    SpindleStat = false;
    digitalWrite(SpindleS, false);
  }
  
  // if (Right.isClick()) Serial.println("Button 1");
  // if (Enter.isClick()) Serial.println("Button 2");
  //if (LimitSwitch.isClick()) Serial.println("Button 3");
  //Serial.println(CurrentMenu);

  switch (mode)
  {
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