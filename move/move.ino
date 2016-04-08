

#define DEBUG Serial

typedef struct {
  uint8_t a_pin;
  uint8_t b_pin;
  uint16_t pwm_pin;
  int16_t offset;
} motor_t;

//巡线卡引脚定义类型
typedef struct {
  uint8_t pin_l2;
  uint8_t pin_l1;
  uint8_t pin0;
  uint8_t pin_r1;
  uint8_t pin_r2;
} light_senser_t;

// //编码器定义及数值类型
// typedef struct {
//     uint8_t pin_a;
//     uint8_t pin_b;
//     uint32_t code;
// } motor_coder_t;

//-------------------------------------
//
//　　　　常量定义区
//
//-------------------------------------
//

//扫到十字路口返回
#define FL_BLACK_LINE 1
//调整到中间态返回
#define FL_NORMAL 0

#define LEFT_MOTOR_OFFSET 0
#define RIGHT_MOTOR_OFFSET 0

const int music_meter = 1;
const uint16_t music_pitch[7] = {262, 294, 330, 350, 393, 441, 495};
const uint8_t music_melody[] = {0, 0, 4, 4, 5, 5, 4, 8, 3, 3, 2, 2, 1, 1, 0, 8,
                          4, 4, 3, 3, 2, 2, 1, 8, 4, 4, 3, 3, 2, 2, 1, 8,
                          0, 0, 4, 4, 5, 5, 4, 8, 3, 3, 2, 2, 1, 1, 0, 8};


//引脚定义
const int pin_tone = 5;
const motor_t right_motor = {6, 5, 3, RIGHT_MOTOR_OFFSET};
const motor_t left_motor = {10, 9, 11, LEFT_MOTOR_OFFSET};
const light_senser_t light_senser_config = {A5, A4, A3, A2, A1};

//-------------------------------------
//           移动函数
//-------------------------------------
void move_go_line(int8_t power) {
  set_motor_output(left_motor, power);
  set_motor_output(right_motor, power);
}
void move_go_raw(int8_t l_power, int8_t r_power) {
  set_motor_output(left_motor, l_power);
  set_motor_output(right_motor, r_power);
}
void move_stop() { move_go_line(0); }


//-------------------------------------
//           巡线卡部分
//-------------------------------------
void light_senser_init(const light_senser_t &light_senser) {
  pinMode(light_senser.pin_l2, INPUT);
  pinMode(light_senser.pin_l1, INPUT);
  pinMode(light_senser.pin0, INPUT);
  pinMode(light_senser.pin_r1, INPUT);
  pinMode(light_senser.pin_r2, INPUT);
}
void light_senser_read(const light_senser_t &light_senser, bool l2, bool l1,
                       bool mid, bool r1, bool r2) {
  l2 = !digitalRead(light_senser.pin_l2);
  l1 = !digitalRead(light_senser.pin_l1);
  mid = !digitalRead(light_senser.pin0);
  r1 = !digitalRead(light_senser.pin_r1);
  r2 = !digitalRead(light_senser.pin_r2);
#ifdef DEBUG
  DEBUG.print(l2);
  DEBUG.print('|');
  DEBUG.print(l1);
  DEBUG.print('|');
  DEBUG.print(mid);
  DEBUG.print('|');
  DEBUG.print(r1);
  DEBUG.print('|');
  DEBUG.print(r2);
  DEBUG.println('|');
#endif
}

//-------------------------------------
//           巡线函数
//-------------------------------------
//通过巡迹卡光感调整输出功率，如果处于十字路口返回FL_BLACK_LINE
int follow_line_adjust(const light_senser_t &light_senser) {
  bool l1, l2, mid, r1, r2;
  light_senser_read(light_senser, l1, l2, mid, r1, r2);
  while (!(((l1 && l2 && r1 && r2) == false) && (mid == true))) {
    light_senser_read(light_senser, l1, l2, mid, r1, r2);

    if (l1 && l2 && mid && r1 && r2) { //全黑返回
      return FL_BLACK_LINE;
    }
    if (!(l1 || l2 || r1 || r2)) { //寻线卡未扫到黑线,慢速前进
      move_go_raw(50, 50);
      continue;
    }
    if (l1 && !(l2 || r1 || r2)) {
      move_go_raw(0, 100);
      continue;
    }
    if (l2 && !(l1 || r1 || r2)) {
      move_go_raw(-50, 100);
      continue;
    }
    if (r1 && !(r2 || l1 || l2)) {
      move_go_raw(100, 0);
      continue;
    }
    if (r2 && !(r1 || l1 || l2)) {
      move_go_raw(100, -50);
      continue;
    }
    if ((l1 && l2) && !(r1 || r2)) {
      move_go_raw(-70, 100);
      delay(200);
      continue;
    }
    if ((r1 && r2) && !(l1 || l2)) {
      move_go_raw(100, -70);
      delay(200);
      continue;
    }
    //          if(l1 && !(l2 && r1 && r2)){
    //            move_go_raw(80,100);
    //            continue;
    //          }
  }
  move_go_raw(100, 100);
}

//巡线跑过几个十字路口
void fl_until_counter(const light_senser_t &ls_config, int8_t base_power,
                      uint8_t times) {
  move_go_raw(50, 50);
  for (; times > 0; times--) {
    while (follow_line_adjust(ls_config) != FL_BLACK_LINE)
      delay(1);
#ifdef DEBUG
    DEBUG.print(times);
    DEBUG.println("reminds");
#endif
    delay(200); //防止在一处黑线重复计数
  }
  move_stop();
}


//-------------------------------------
//           马达驱动部分
//-------------------------------------
void motor_init(const motor_t &motor) {
  pinMode(motor.a_pin, OUTPUT);
  pinMode(motor.b_pin, OUTPUT);
  pinMode(motor.pwm_pin, OUTPUT);
  digitalWrite(motor.a_pin, LOW);
  digitalWrite(motor.b_pin, LOW);
}
void set_motor_output(const motor_t &motor, int power) {
  if (power == 0) {
    digitalWrite(motor.a_pin, HIGH);
    digitalWrite(motor.b_pin, HIGH);
  } else {
    if (power > 0) {
      digitalWrite(motor.a_pin, HIGH);
      digitalWrite(motor.b_pin, LOW);
    } else {
      digitalWrite(motor.a_pin, LOW);
      digitalWrite(motor.b_pin, HIGH);
    }
    if (abs(power) > 100)
      power = 100;
    analogWrite(motor.pwm_pin, abs(power) * 10 + motor.offset);
  }
}



void music_init(void) { pinMode(pin_tone, OUTPUT); }

void music_begin(void) {
  for (int i = 0; i < 48; i++) {
    if (music_melody[i] == 8) {
      delay(350);
    } else
      music_beeper(music_pitch[i]);
  }
}

void music_beeper(const int hz) {
  tone(pin_tone, music_pitch[hz]);
  delay(400);
  noTone(pin_tone);
}

void setup() {
  motor_init(left_motor);
  motor_init(right_motor);
  light_senser_init(light_senser_config);
  music_init();
  Serial.begin(9600);
}

void loop(){
    
    //nothing here.
}
