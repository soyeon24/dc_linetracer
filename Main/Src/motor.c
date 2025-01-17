/*
 * motor.c
 */

#include <stdint.h>
#include <stdbool.h>
#include "main.h"
#include "sensor.h"
#include "motor.h"
#include "drive.h"
#include "custom_delay.h"
#include "custom_gpio.h"
#include "custom_oled.h"
#include "custom_switch.h"

#define ABS(x) ((x>0) ? x:(-x))
#define ENCODER_RATIO 2048
#define GEAR (69/17)
#define WHEEL 3 //3cm라 가정
#define PI 3.1415926
#define TICK_PER_METER (GEAR/(3*PI))
#define ANGLE_PER_TICK (1/ENCODER_RATIO)
#define ANGLE_PER_METER (TICK_PER_METER * ANGLE_PER_TICK)
#define TIME 0.000001
#define MOTOR_RES 2.32//모터래지스터
#define MOTOR_KE 0.023405f//모터역기전력 토크상수


typedef struct MOTOR{
uint32_t CurrEncVal; //현재 엔코더
uint32_t PastEncVal; //이전 엔코더
int32_t ErrEnc;	// 현재 - 이전 엔코더 == 거리 차이
float EncV;
float EncD;

float ComV; //커맨드 속도
float ComD; //커맨드 위치
float v; //바퀴속도


};

float gain_p = 1.0f;
float gain_d = 0.f;

struct MOTOR MOTORL;
struct MOTOR MOTORR;


void Motor_Start() {
	LL_TIM_EnableCounter(TIM2);
	LL_TIM_EnableIT_UPDATE(TIM2);

	TIM2->CNT = 0;

	LL_TIM_EnableCounter(TIM5);
	LL_TIM_EnableIT_UPDATE(TIM5);

	TIM5->CNT = 0;

}

void Motor_Stop() {
	LL_TIM_DisableCounter(TIM2);
	LL_TIM_DisableIT_UPDATE(TIM2);

	LL_TIM_DisableCounter(TIM5);
	LL_TIM_DisableIT_UPDATE(TIM5);

}

void Motor_L_TIM3_IRQ() {
	MOTORL.CurrEncVal = TIM2->CNT;
	MOTORL.ErrEnc = MOTORL.CurrEncVal - MOTORL.PastEncVal;
	MOTORL.EncV = MOTORL.ErrEnc * ANGLE_PER_TICK / TIME;

	MOTORL.EncD += MOTORL.EncV * TIME; //적분

	MOTORL.ComV = MOTORL.v * ANGLE_PER_METER;//meter per time* angle per meter = angle per time
	MOTORL.ComD += MOTORL.ComV * TIME;

	float ErrV = MOTORL.EncV - MOTORL.ComV;	//현재 각속도 - 목표각속도
	float ErrD = MOTORL.EncD - MOTORL.ComD; //현재거리-목표거리

	int32_t duty_t =0;
	float Cur = -(ErrV * gain_p + ErrD * gain_d);
	float Volt = Cur * MOTOR_RES +  MOTORL.EncV * MOTOR_KE;
	if(Volt>batteryVolt) duty_t = TIM3->ARR;
	duty_t = Volt/ batteryVolt * TIM3->ARR;
	uint16_t duty = ABS(duty_t);

	MOTORL.PastEncVal = MOTORL.CurrEncVal;
}

void Motor_Test_Phase() {
	/*
	 * 모터의 각 상을 잠깐씩 잡아본다.
	 * 모터에 무리를 주지 않기 위해 100ms 동안만 상을 잡은 후 바로 놓는다.
	 */
	// --- Write your code ---
	Motor_Start();

	Motor_Stop();

}

void Motor_Test_Velocity() {
	/*
	 * 모터 속도를 부드럽게 올렸다가 내리기를 반복한다.
	 */

}
