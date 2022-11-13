#include <Arduino.h>

int angle_bent[] = { 0, 0, 0, 0, 0 };    //Угол поворота сервомотора, при котором палец согнут
int angle_unbent[] = { 0, 0, 0, 0, 0 };  //Угол поворота сервомотора, при котором палец разогнут
bool rotate_dir[] = { 1, 1, 1, 0, 0 };  //Направление вращения мотора, для синхронизации
int servo_cur_angle[] = { 0, 0, 0, 0, 0 };

float rotate_persanteg[] = { 0, 0, 0, 0, 0 }; 
int angles[] = { 60, 60, 40, 170, 170 };       //Текущие углы поворта сервомторов
int flag_angles[] = { 60, 60, 40, 170, 170 };  //Придыдущие углы поворота сервомторов

int sensor_max_val[] = { 0, 0, 0, 0, 0 };  //Максимальные значения датчиков изгиба
int sensor_min_val[] = { 0, 0, 0, 0, 0 };  //Минимальные значения датчиков изгиба
int sensor_new_max_val[] = { 0, 0, 0, 0, 0 };
int sensor_new_min_val[] = { 0, 0, 0, 0, 0 };
int sensor_cur_val[] = { 0, 0, 0, 0, 0 };
long sensor_sum[] = { 0, 0, 0, 0, 0 };
int sensor_val[] = { 0, 0, 0, 0, 0 };

int m_pos = 0;  //Местоположения пользователья в меню
int servo_menu_pos = 0;
int sensor_menu_pos = 0;
bool menu_flag = 0;  //Индикатор нахождения пользователя в одном из разделов меню

String sensor_menu_labels[] = { "-Calibrate all", "-Thumb", "-Index finger", "-Middle finger", "-Ring finger", "-Pinky" };
String servo_menu_labels[] = { "-Thumb finger", "-Index finger", "-Middle finger", "-Ring finger", "-Pinky" };
String main_menu_labels[] = { "-Sensor calibration", "-Servo calibration", "-Start", "-Information" };
String start_menu_labels[] = { "T:", "I:", "M:", "R:", "P:" };

bool cheker = 0;
bool exiting = 0;

#define DATA_AMOUNT 100  //Кол-во значение для вычисления среднего арифметического

#define CLK 8  //Пины энкодера
#define DT 9
#define SW 10

#define b_f_pin 2  //Пины сервомоторов
#define s_f_pin 3
#define m_f_pin 4
#define u_f_pin 5
#define sm_f_pin 6

#define b_f_sensor A0  //Пины датчиков
#define s_f_sensor A1
#define m_f_sensor A2
#define u_f_sensor A3
#define sm_f_sensor A6

/*Servo b_f;  //Создание объектов серво
Servo s_f;
Servo m_f;
Servo u_f;
Servo sm_f;*/