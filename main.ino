// EEPROM order: 0-4 - max(bent); 5-9 - min(unbent)

#include <Servo.h>         //Servomotor library
#include <GyverOLED.h>     //OLED scren library
#include "GyverEncoder.h"  //Encoder library
#include <EEPROM.h>        //Библиотека для записи данных в EEPROM
#include "varibles.h"      //Файл со всеми переменными

Encoder enc1(CLK, DT, SW);  //Создание объекта энкодера

Servo b_f;  //Создание объектов серво
Servo s_f;
Servo m_f;
Servo u_f;
Servo sm_f;

GyverOLED<SSD1306_128x64, OLED_NO_BUFFER> oled;  //Создание объекта дисплея

void setup() {
  oled.init();       //Инициализация экрана
  oled.clear();      //Очистка экрана
  oled.setScale(3);  //Вывод начальной заставки на экран
  oled.setCursor(12, 0);
  oled.print("PhandR");
  oled.setScale(2);
  oled.setCursor(40, 3);
  oled.print("V1.0");
  oled.setScale(1);
  oled.setCursor(20, 5);
  oled.print("Initialization");
  Serial.begin(9600);  //Инициализация Com-port

  for (int i = 0; i < 5; i++) {                         //Чтение данных из EEPROM
    angle_bent[i] = EEPROM[i];                          //Угол при котором палец согнут
    angle_unbent[i] = EEPROM[i + 5];                    //Угол, при котором палец разогнут
    sensor_max_val[i] = readIntFromEEPROM(10 + i * 2);  //Максимальное возможное значение с датчика с датчика изгиба
    sensor_min_val[i] = readIntFromEEPROM(20 + i * 2);  //Минимальное возможное значение с датчика изгиба
  }

  b_f.attach(b_f_pin);  //Инициализация сервомторов
  s_f.attach(s_f_pin);
  m_f.attach(m_f_pin);
  u_f.attach(u_f_pin);
  sm_f.attach(sm_f_pin);

  b_f.write(angle_unbent[0]);  //Приведение сервомоторов в начальное положение (разогнуто)
  s_f.write(angle_unbent[1]);
  m_f.write(angle_unbent[2]);
  u_f.write(angle_unbent[3]);
  sm_f.write(angle_unbent[4]);

  enc1.setType(TYPE2);  //Определение типа энкодера
  oled.setCursor(20, 5);
  oled.print("              ");
  oled.setCursor(54, 5);
  oled.print("Done");
  oled.setScale(1);
  oled.setCursor(7, 6);
  oled.print("Press any key . . .");
  while (!(enc1.isTurn() or enc1.isClick() or enc1.isHolded())) {  //Ожидание подтверждения оконгчания инициализации от пользователя
    enc1.tick();
  }
  gen_menu();
  select_menu(0, 1);
}

void loop() {
  enc1.tick();             //Опрос энкодера
  if (!menu_flag) {        //Если на данный момент мы не находимся в одном из пунктов меню, то:
    if (enc1.isRight()) {  //Если энкодер повернут направо и не достугнута максимальная позиция меню, переключиться на эту новую позицию(текущая + 1)
      if (m_pos < 3) {
        m_pos++;
        select_menu(m_pos, m_pos - 1);
      } else {  //Если достигнута максимальная позиция меню, автоматически переключиться на 0-вую
        m_pos = 0;
        select_menu(m_pos, 3);
      }
    } else if (enc1.isLeft()) {
      if (m_pos > 0) {  //Если энкодер повернут влево и не достугнута минимальная позиция меню, переключиться на эту новую позицию(текущая - 1)
        m_pos--;
        select_menu(m_pos, m_pos + 1);
      } else {  //Если достигнута минимальная позиция меню, автоматически переключиться на 3-ю
        m_pos = 3;
        select_menu(m_pos, 0);
      }
    }
  } else {  //Если пользователь уже находится в каком-либо подпункте меню, переключиться на необходимую подпозицию меню. Принцип как и выше но меняется кол-во возможных позиций и сами позиции в меню
    switch (m_pos) {
      case 0:
        if (enc1.isRight()) {
          if (sensor_menu_pos < 5) {
            sensor_menu_pos++;
            select_menu(sensor_menu_pos, sensor_menu_pos - 1);
          } else {
            sensor_menu_pos = 0;
            select_menu(sensor_menu_pos, 5);
          }
        }
        if (enc1.isLeft()) {
          if (sensor_menu_pos > 0) {
            sensor_menu_pos--;
            select_menu(sensor_menu_pos, sensor_menu_pos + 1);
          } else {
            sensor_menu_pos = 5;
            select_menu(sensor_menu_pos, 0);
          }
        }
        break;
      case 1:
        if (enc1.isRight()) {
          if (servo_menu_pos < 4) {
            servo_menu_pos++;
            select_menu(servo_menu_pos, servo_menu_pos - 1);
          } else {
            servo_menu_pos = 0;
            select_menu(servo_menu_pos, 4);
          }
        } else if (enc1.isLeft()) {
          if (servo_menu_pos > 0) {
            servo_menu_pos--;
            select_menu(servo_menu_pos, servo_menu_pos + 1);
          } else {
            servo_menu_pos = 4;
            select_menu(servo_menu_pos, 0);
          }
        }
        break;
    }
  }

  if (enc1.isClick()) {  //Если энкодер нажат переключиться на выбранное меню или подпункт
    open_menu_pos(m_pos);

  } else if (enc1.isHolded()) {  //Если энкодер был удержан в нажатом положении, вернуться на позицию назад
    if (menu_flag) {
      menu_flag = 0;
      gen_menu();
    }
  }
}


void read_data() {  //Чтение данных с датчиков изгиба, фильтрация и вычисления процента изгиба
  for (int i = 0; i < DATA_AMOUNT; i++) {

    sensor_sum[0] += analogRead(b_f_sensor);  //Чтение данных с датчиков изгиба
    sensor_sum[1] += analogRead(s_f_sensor);
    sensor_sum[2] += analogRead(m_f_sensor);
    sensor_sum[3] += analogRead(u_f_sensor);
    sensor_sum[4] += analogRead(sm_f_sensor);
  }
  for (int i = 0; i < 5; i++) {  //Преобразование данных с датчиков изгиба, в зависимость от установленного направления вращения
    sensor_cur_val[i] = sensor_sum[i] / 100;
    if (rotate_dir[i]) {
      rotate_persanteg[i] = float(sensor_cur_val[i] - sensor_min_val[i]) / float(sensor_max_val[i] - sensor_min_val[i]);
    } else {
      rotate_persanteg[i] = float(sensor_cur_val[i] - sensor_max_val[i]) / float(sensor_min_val[i] - sensor_max_val[i]);
    }
    sensor_val[i] = sensor_sum[i] / 100;
    sensor_sum[i] = 0;
  }
}

void select_menu(int m_point, int prev_pos) {  //Функция для отображения перемещения по меню на экране
  if (!menu_flag) {
    oled.setScale(1);
    oled.invertText(true);
    oled.setCursor(5, m_point + 3);
    oled.print(main_menu_labels[m_point]);
    oled.invertText(false);
    oled.setCursor(5, prev_pos + 3);
    oled.print(main_menu_labels[prev_pos]);
  } else {
    switch (m_pos) {
      case 0:
        oled.setScale(1);
        Serial.println(m_point);
        oled.invertText(true);
        oled.setCursor(15, m_point + 2);
        oled.print(sensor_menu_labels[m_point]);
        oled.invertText(false);
        oled.setCursor(15, prev_pos + 2);
        oled.print(sensor_menu_labels[prev_pos]);
        break;
      case 1:
        Serial.print(m_point);
        Serial.print("  ");
        Serial.println(prev_pos);
        oled.setScale(1);
        oled.invertText(true);
        oled.setCursor(15, m_point + 3);
        oled.print(servo_menu_labels[m_point]);
        oled.invertText(false);
        oled.setCursor(15, prev_pos + 3);
        oled.print(servo_menu_labels[prev_pos]);
    }
  }
}

void gen_menu() {  //Функция для генерации главного меню
  oled.invertText(false);
  oled.clear();
  oled.setCursor(39, 0);
  oled.setScale(2);

  oled.print("Menu");
  oled.invertText(false);

  oled.setScale(1);
  oled.setCursor(5, 3);
  oled.print(main_menu_labels[0]);

  oled.setCursor(5, 4);
  oled.print(main_menu_labels[1]);

  oled.setCursor(5, 5);
  oled.print(main_menu_labels[2]);

  oled.setCursor(5, 6);
  oled.print(main_menu_labels[3]);

  select_menu(m_pos, 10);
}

void gen_servo_menu() {  //Функция для отображения на экране пунктов меню калибровки сервомоторов
  oled.clear();
  oled.setScale(2);
  oled.setCursor(35, 0);
  oled.print("Servo");
  oled.setScale(1);
  oled.setCursor(5, 2);
  oled.print("Choose finger:");
  oled.setCursor(15, 3);
  oled.invertText(true);
  oled.print(servo_menu_labels[0]);
  oled.invertText(false);
  oled.setCursor(15, 4);
  oled.print(servo_menu_labels[1]);
  oled.setCursor(15, 5);
  oled.print(servo_menu_labels[2]);
  oled.setCursor(15, 6);
  oled.print(servo_menu_labels[3]);
  oled.setCursor(15, 7);
  oled.print(servo_menu_labels[4]);
}

void servo_calib(int servo_index) {  //Функция для калибровки максимального и минимального положения сервмоторов
  oled.clear();
  oled.setScale(2);
  oled.setCursor(5, 0);
  oled.print(servo_menu_labels[servo_menu_pos]);

  oled.setScale(1);
  oled.setCursor(25, 3);
  oled.print("min");
  oled.setScale(1);

  oled.invertText(true);
  oled.setCursor(83, 3);
  oled.print("max");
  oled.invertText(false);
  show_servo_angles(servo_index, 0, 0);
  oled.setScale(1);

  int calibration_mode = 0;
  bool calibration_mode_b = 0;

  while (true) {
    enc1.tick();
    if (enc1.isTurn()) {
      calibration_mode_b = !calibration_mode_b;
      oled.invertText(calibration_mode_b);
      oled.setCursor(25, 3);
      oled.print("min");
      oled.invertText(!calibration_mode_b);
      oled.setCursor(83, 3);
      oled.print("max");
      oled.invertText(false);
    }
    if (enc1.isClick()) {
      calibration_mode = calibration_mode_b;
      Serial.println(calibration_mode);
      break;
    }
    if (enc1.isHolded()) {
      menu_flag = 0;
      servo_menu_pos = 0;
      open_menu_pos(1);
      return;
    }
  }
  enc1.tick();
  oled.setScale(2);
  while (!enc1.isClick()) {
    enc1.tick();
    if (enc1.isRight()) {
      angle_bent[servo_index] += !calibration_mode;
      angle_unbent[servo_index] += calibration_mode;
      show_servo_angles(servo_index, 1, calibration_mode);
    }
    if (enc1.isLeft()) {
      angle_bent[servo_index] -= !calibration_mode;
      angle_unbent[servo_index] -= calibration_mode;
      show_servo_angles(servo_index, 1, calibration_mode);
    }
  }
  EEPROM.update(servo_index + 5 * calibration_mode, angle_bent[servo_index] * !calibration_mode + angle_unbent[servo_index] * calibration_mode);
  servo_calib(servo_index);
}

void show_servo_angles(int servo_index, bool on_hand_visibility, bool calibration_mode) {  //Фунция для отображения текущего положения сервомоторов при калибровке
  oled.setScale(2);
  oled.setCursor(20, 5);
  oled.print(String(angle_unbent[servo_index]) + " ");
  oled.setCursor(75, 5);
  oled.print(String(angle_bent[servo_index]) + " ");
  if (on_hand_visibility) {
    switch (servo_menu_pos) {
      case 0:
        b_f.write(angle_bent[0] * !calibration_mode + angle_unbent[0] * calibration_mode);
        break;
      case 1:
        s_f.write(angle_bent[1] * !calibration_mode + angle_unbent[1] * calibration_mode);
        break;
      case 2:
        m_f.write(angle_bent[2] * !calibration_mode + angle_unbent[2] * calibration_mode);
        break;
      case 3:
        u_f.write(angle_bent[3] * !calibration_mode + angle_unbent[3] * calibration_mode);
        break;
      case 4:
        sm_f.write(angle_bent[4] * !calibration_mode + angle_unbent[4] * calibration_mode);
        break;
    }
  }
}
void sensors_calib(int m_point) {  //Функция для калибровки значений с датчиков
  oled.clear();
  oled.invertText(false);
  oled.setScale(2);
  oled.setCursor(5, 0);
  oled.autoPrintln(false);
  oled.print(sensor_menu_labels[m_point]);
  oled.setScale(1);
  oled.setCursor(5, 3);
  oled.autoPrintln(true);
  oled.print(F("For calibration      follow instructions  on the screen!"));
  oled.autoPrintln(false);

  oled.invertText(true);
  oled.setCursor(15, 7);
  oled.print("Resume");
  oled.invertText(false);
  oled.setCursor(75, 7);
  oled.print("Back");
  bool but_1 = 0;
  while (true) {
    enc1.tick();
    if (enc1.isTurn()) {
      but_1 = !but_1;
      oled.invertText(!but_1);
      oled.setCursor(15, 7);
      oled.print("Resume");
      oled.invertText(but_1);
      oled.setCursor(75, 7);
      oled.print("Back");
      oled.invertText(false);
    }
    if (enc1.isClick()) {
      break;
    }
    if (enc1.isHolded()) {
      menu_flag = 0;
      gen_menu();
      return;
    }
  }
  if (but_1) {
    menu_flag = 0;
    open_menu_pos(m_pos);
    return;
  }


  if (m_point == 0) {
    cheker = 0;
    while (!cheker) {
      calibrate_all_finger_sensors_max();
      if (exiting) {
        menu_flag = 0;
        open_menu_pos(m_pos);
        exiting = 0;
        return;
      }
    }
    cheker = 0;
    oled.clear();
    oled.setScale(2);
    oled.setCursor(15, 4);
    oled.print("Done");
    unsigned long timer = millis();
    while (millis() - timer < 3000) { enc1.tick(); }
    while (!cheker) {
      calibrate_all_finger_sensors_min();
      if (exiting) {
        menu_flag = 0;
        open_menu_pos(m_pos);
        exiting = 0;
        return;
      }
    }
    oled.clear();
    oled.setScale(2);
    oled.setCursor(15, 4);
    oled.print("Done");
    timer = millis();
    while (millis() - timer < 3000) { enc1.tick(); }
    open_menu_pos(m_pos);
    return;
  }
}

void calibrate_all_finger_sensors_max() {  //Функция для калибровки макимального занчения с датчиков
  oled.clear();
  oled.setCursor(5, 2);
  oled.autoPrintln(true);
  oled.print("   Straighten your         fingers");
  unsigned long timer = millis();
  int8_t t_counter = 5;
  oled.setScale(2);
  while (true) {
    if (millis() - timer > 1000) {
      timer = millis();
      t_counter--;
      oled.setCursor(60, 5);
      oled.print(t_counter);
    }
    if (t_counter == 0) {
      sensor_new_max_val[0] = analogRead(b_f_sensor);
      sensor_new_max_val[1] = analogRead(s_f_sensor);
      sensor_new_max_val[2] = analogRead(m_f_sensor);
      sensor_new_max_val[3] = analogRead(u_f_sensor);
      sensor_new_max_val[4] = analogRead(sm_f_sensor);
      break;
    }
  }

  oled.clear();
  oled.setScale(1);
  for (int i = 0; i < 5; i++) {
    oled.setCursor(5, i + 1);
    oled.print(servo_menu_labels[i] + ": " + String(sensor_new_max_val[i]));
  }
  oled.invertText(true);
  oled.setCursor(15, 7);
  oled.print("Resume");
  oled.invertText(false);
  oled.setCursor(68, 7);
  oled.print("Retry");
  bool but_2 = 0;
  while (true) {
    enc1.tick();
    if (enc1.isTurn()) {
      but_2 = !but_2;
      oled.invertText(!but_2);
      oled.setCursor(15, 7);
      oled.print("Resume");
      oled.invertText(but_2);
      oled.setCursor(68, 7);
      oled.print("Retry");
      oled.invertText(false);
    }
    if (enc1.isClick()) {
      break;
    }
    if (enc1.isHolded()) {
      exiting = 1;
      return;
    }
  }
  if (but_2) {
    cheker = 0;
    return;
  } else {
    for (int i = 0; i < 5; i++) {
      sensor_max_val[i] = sensor_new_max_val[i];
      Serial.print(sensor_max_val[i]);
      Serial.print(" ");
      writeIntIntoEEPROM(10 + i * 2, sensor_max_val[i]);
      Serial.println(readIntFromEEPROM(10 + i * 2));
    }
    Serial.println("data updated");
    cheker = 1;
    return;
  }
}

void calibrate_all_finger_sensors_min() {  //Функция для калибровки минимального занчения с датчиков
  oled.clear();
  oled.setScale(1);
  oled.setCursor(5, 2);
  oled.autoPrintln(true);
  oled.print("Bent your fingers");
  unsigned long timer = millis();
  int8_t t_counter = 5;
  oled.setScale(2);
  while (true) {
    if (millis() - timer > 1000) {
      timer = millis();
      t_counter--;
      oled.setCursor(60, 5);
      oled.print(t_counter);
    }
    if (t_counter == 0) {
      sensor_new_min_val[0] = analogRead(b_f_sensor);
      sensor_new_min_val[1] = analogRead(s_f_sensor);
      sensor_new_min_val[2] = analogRead(m_f_sensor);
      sensor_new_min_val[3] = analogRead(u_f_sensor);
      sensor_new_min_val[4] = analogRead(sm_f_sensor);
      break;
    }
  }

  oled.clear();
  oled.setScale(1);
  for (int i = 0; i < 5; i++) {
    oled.setCursor(5, i + 1);
    oled.print(servo_menu_labels[i] + ": " + String(sensor_new_min_val[i]));
  }
  oled.invertText(true);
  oled.setCursor(15, 7);
  oled.print("Resume");
  oled.invertText(false);
  oled.setCursor(68, 7);
  oled.print("Retry");
  bool but_2 = 0;
  while (true) {
    enc1.tick();
    if (enc1.isTurn()) {
      but_2 = !but_2;
      oled.invertText(!but_2);
      oled.setCursor(15, 7);
      oled.print("Resume");
      oled.invertText(but_2);
      oled.setCursor(68, 7);
      oled.print("Retry");
      oled.invertText(false);
    }
    if (enc1.isClick()) {
      break;
    }
    if (enc1.isHolded()) {
      exiting = 1;
      return;
    }
  }
  if (but_2) {
    cheker = 0;
    return;
  } else {
    for (int i = 0; i < 5; i++) {
      sensor_min_val[i] = sensor_new_min_val[i];
      Serial.print(sensor_min_val[i]);
      Serial.print(" ");
      writeIntIntoEEPROM(20 + i * 2, sensor_min_val[i]);
      Serial.println(readIntFromEEPROM(20 + i * 2));
    }
    cheker = 1;
    return;
  }
}

void starting() {   //Функция отвечающая, за непосредственный процесс связи сенсорной системы с исполняющим устройством
  oled.clear();
  oled.invertText(false);
  oled.autoPrintln(false);
  oled.setScale(2);
  oled.setCursor(20, 2);
  oled.print("Confirm");
  oled.setScale(1);
  oled.setCursor(22, 5);
  oled.print("by long press");
  while (!enc1.isHolded()) {
    enc1.tick();
  }
  oled.clear();
  oled.setScale(2);
  oled.setCursor(20, 0);
  oled.print("Working");
  enc1.tick();
  oled.setScale(1);
  for (int i = 0; i < 5; i++) {
    oled.setCursor(2, i + 3);
    oled.print(start_menu_labels[i]);
  }
  oled.setCursor(25, 2);
  oled.print("S_v");
  oled.setCursor(60, 2);
  oled.print("A_v");
  oled.setCursor(95, 2);
  oled.print("P_v");

  while (!enc1.isHolded()) {
    enc1.tick();
    read_data();
    rotate_servo();
    print_r_data();
  }
  menu_flag = 0;
}

void writeIntIntoEEPROM(int address, int number) {    //Функция для записи перменной типа INT в EEPROM, занимает 2 байта
  EEPROM.write(address, number >> 8);
  EEPROM.write(address + 1, number & 0xFF);
}

int readIntFromEEPROM(int address) {    //Функция для чтения INT из EEPROM записанной в 2 байта
  return (EEPROM.read(address) << 8) + EEPROM.read(address + 1);
}

void rotate_servo() {   //Функция для поворота сервмоторов на необхожимый угол
  for (int i = 0; i < 5; i++) {
    if (rotate_dir) {
      servo_cur_angle[i] = angle_bent[i] - int(float(angle_bent[i] - angle_unbent[i]) / 100 * rotate_persanteg[i]);
      Serial.println(angle_bent[2] - int(float(angle_bent[2] - angle_unbent[2]) * rotate_persanteg[2]));
    } else {
      servo_cur_angle[i] = angle_bent[i] + int(float(angle_unbent[i] - angle_bent[i]) / 100 * rotate_persanteg[i]);
    }
  }
  b_f.write(servo_cur_angle[0]);
  s_f.write(servo_cur_angle[1]);
  m_f.write(servo_cur_angle[2]);
  u_f.write(servo_cur_angle[3]);
  sm_f.write(servo_cur_angle[4]);
}

void print_r_data() {   //Функция для отображения в реаьном верменени данных, необходимыях для отслеживания уорректности раюоты систем (показания дачтикво, текущий угол поворота сервомоторов, процента сгиба)
  for (int i = 0; i < 5; i++) {
    oled.setCursor(25, i + 3);
    oled.print(String(sensor_val[i]) + " ");
    oled.setCursor(60, i + 3);
    oled.print(String(servo_cur_angle[i]) + " ");
    oled.setCursor(98, i + 3);
    oled.print(String(int(rotate_persanteg[i] * 100)) + " ");
  }
}

void open_menu_pos(int pos) {  //Функция для отображения содержимого подпункта на экране
  oled.invertText(false);
  switch (pos) {
    case 0:
      menu_flag = 1;
      sensors_calib(0);
      break;

    case 1:
      if (menu_flag) {
        servo_calib(servo_menu_pos);
      } else {
        gen_servo_menu();
        menu_flag = 1;
      }
      break;
    case 2:
      starting();
      gen_menu();
      return;
      break;
    case 3:
      oled.clear();
      oled.setScale(2);
      oled.setCursor(38, 0);
      oled.print("Info");
      oled.setScale(1);
      oled.setCursor(5, 3);
      oled.print("-Version: 1.0");
      oled.setCursor(5, 4);
      oled.print("-Build in:");
      oled.setCursor(5, 5);
      oled.print("   Kvantorium Pskov");
      oled.setCursor(5, 6);
      oled.print("-Developer:");
      oled.setCursor(5, 7);
      oled.print("   Shchembelov Ilya");

      menu_flag = 1;
  }
}