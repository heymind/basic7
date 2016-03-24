

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

//巡线卡数值类型(黑为真，白为假)
typedef struct {
    bool pin_l2;
    bool pin_l1; 
    bool pin0;
    bool pin_r1; 
    bool pin_r2;
} light_senser_val_t;

//编码器定义及数值类型
typedef struct {
    uint8_t pin_a;
    uint8_t pin_b;
    uint32_t code;
} motor_coder_t;

//-------------------------------------
//
//　　　　常量定义区
//
//-------------------------------------
//
//分别定义了当中间的光感为白，距离中间最近的光感为黑，最外边的光感为黑马达的偏差值（理论上中间为黑，其余为白是稳态）
#define FL_CENTRE_WHITE_DIFF 10
#define FL_NEAR_BLACK_DIFF 10
#define FL_FAR_BLACK_DIFF 10

//定义马达习惯性偏差校准，未来通过自校准程序实现(分度值为1ooo)
#define LEFT_MOTOR_OFFSET 0
#define RIGHT_MOTOR_OFFSET 0
const motor_t left_motor={11,12,13,LEFT_MOTOR_OFFSET};
const motor_t right_motor={10,8,9,RIGHT_MOTOR_OFFSET};
const light_senser_t light_senser_config={4,5,6};


//-------------------------------------
//           移动函数
//-------------------------------------
void move_go_line(int8_t power){
    set_motor_output(left_motor,power);
    set_motor_output(right_motor,power);
}
void move_go_raw(int8_t l_power,int8_t r_power){
    set_motor_output(left_motor,l_power);
    set_motor_output(right_motor,r_power);
}
void move_stop(){
    move_go_line(0);
}

//-------------------------------------
//           巡线函数
//-------------------------------------
//通过巡迹卡光感调整输出功率，如果处于十字路口返回真
bool fl_adjust_motor_output(const light_senser_t &ls_config,int8_t base_power){

    light_senser_val_t ls_value;
    light_senser_value_update(ls_config,ls_value);
    if(ls_value.pin_l2 && ls_value.pin_l2 \
            && ls_value.pin_r2 && ls_value.pin_r1 \
            && ls_value.pin0) return true;
    int8_t diff = 0; //左偏为正
    if(ls_value.pin_l1) diff+=FL_NEAR_BLACK_DIFF;
    if(ls_value.pin_l2) diff+=FL_FAR_BLACK_DIFF;

    if(ls_value.pin_r1) diff-=FL_NEAR_BLACK_DIFF;
    if(ls_value.pin_r2) diff-=FL_FAR_BLACK_DIFF;

    if(ls_value.pin0 == false) diff = diff>0 ? diff+=FL_CENTRE_WHITE_DIFF : diff-=FL_CENTRE_WHITE_DIFF;

    move_go_raw(base_power - diff,base_power+diff);
}
//巡线跑过几个十字路口
void fl_until_counter(const light_senser_t &ls_config,int8_t base_power,uint8_t times){
    for(;times>0;times--){
        while(!fl_adjust_motor_output(ls_config,base_power)) delay(100);
        delay(500); //防止在一处黑线重复计数
    }
    move_stop();
}

//-------------------------------------
//           巡线卡部分
//-------------------------------------
void light_senser_init(const light_senser_t &light_senser){
    pinMode(light_senser.pin_l2,INPUT);
    pinMode(light_senser.pin_l1,INPUT);
    pinMode(light_senser.pin0,INPUT);
    pinMode(light_senser.pin_r1,INPUT);
    pinMode(light_senser.pin_r2,INPUT);
}
void light_senser_value_update(const light_senser_t &light_senser,light_senser_val_t &senser_val){
    senser_val.pin_l2=digitalRead(light_senser.pin_l2);
    senser_val.pin_l1=digitalRead(light_senser.pin_l1);
    senser_val.pin0=digitalRead(light_senser.pin0);
    senser_val.pin_r1=digitalRead(light_senser.pin_r1);
    senser_val.pin_r2=digitalRead(light_senser.pin_r2);
#ifdef DEBUG
    DEBUG.print(senser_val.pin_l2);
    DEBUG.print('|');
    DEBUG.print(senser_val.pin_l1);
    DEBUG.print('|');
    DEBUG.print(senser_val.pin0);
    DEBUG.print('|');
    DEBUG.print(senser_val.pin_r1);
    DEBUG.print('|');
    DEBUG.print(senser_val.pin_r2);
    DEBUG.println('|');
#endif

}

//-------------------------------------
//           马达驱动部分
//-------------------------------------
void motor_init(const motor_t &motor){
    pinMode(motor.a_pin,OUTPUT);
    pinMode(motor.b_pin,OUTPUT);
    pinMode(motor.pwm_pin,OUTPUT);
    digitalWrite(motor.a_pin,LOW);
    digitalWrite(motor.b_pin,LOW);
}
void set_motor_output(const motor_t &motor,int power){
    if (power == 0){
        digitalWrite(motor.a_pin,HIGH);
        digitalWrite(motor.b_pin,HIGH);    
    }else{
        if(power > 0){
            digitalWrite(motor.a_pin,HIGH);
            digitalWrite(motor.b_pin,LOW);    
        }else{
            digitalWrite(motor.a_pin,LOW);
            digitalWrite(motor.b_pin,HIGH);      
        }
        analogWrite(motor.pwm_pin,abs(power) * 10+motor.offset);
    }
}
light_senser_val_t front_light_senser_value;
void setup(){
    motor_init(left_motor);
    motor_init(right_motor);
    light_senser_init(light_senser_config);
    Serial.begin(9600);
}

//---------------
//---------------
//调试注意：调试每个test注释其他的。

void loop(){
  //test1 应该正常稳定的巡线，进行FL_CENTRE_WHITE_DIFF　FL_NEAR_BLACK_DIFF　FL_FAR_BLACK_DIFF三个预编译量的调整
    while(1){
      fl_adjust_motor_output(light_senser_config,80);
    }

 //test 2 摆放５个地图块，应该巡３次十字路口后停止
 fl_until_counter(light_senser_config,80,3);


 //test 3 
 fl_until_counter(light_senser_config,80,2);
 move_go_raw(-100,100);//左转
 delay(1000); //!!!!!修改这个时间
 fl_until_counter(light_senser_config,80,2);
 
 
}
