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
#define MIN(a, b) ((a > b) ? b : a )
#define ENCODER_RATIO 2048.f
#define GEAR (69.f/17.f)
#define WHEEL 3 //3cm라 가정
#define PI 3.1415926f
#define TICK_PER_METER (GEAR/(3*PI))
#define ANGLE_PER_TICK (1/ENCODER_RATIO)
#define ANGLE_PER_METER (TICK_PER_METER * ANGLE_PER_TICK)
float TIME = 0.0005f;
#define MOTOR_RES 2.32//모터래지스터
#define MOTOR_KE 0.023405f//모터역기전력 토크상수

typedef struct MOTOR {
	uint32_t CurrEncVal; //현재 엔코더
	uint32_t PastEncVal; //이전 엔코더
	int32_t ErrEnc;	// 현재 - 이전 엔코더 == 거리 차이
	float EncV;
	float EncD;

	float ComV; //커맨드 속도
	float ComD; //커맨드 위치
	float v; //바퀴속도

} MOTOR;

float gain_p = 1.0f;
float gain_d = 0.f;

struct MOTOR MOTORL;
struct MOTOR MOTORR;

void Motor_Start() {
	LL_TIM_EnableCounter(TIM2);
	LL_TIM_EnableIT_UPDATE(TIM2);
	LL_TIM_EnableCounter(TIM3);
	LL_TIM_EnableIT_UPDATE(TIM3);
	LL_TIM_OC_SetCompareCH1(TIM3, 0);
	LL_TIM_CC_EnableChannel(TIM3, LL_TIM_CHANNEL_CH1);
	LL_TIM_OC_SetCompareCH2(TIM3, 0);
	LL_TIM_CC_EnableChannel(TIM3, LL_TIM_CHANNEL_CH2);
	LL_TIM_OC_SetCompareCH4(TIM3, 0);
	LL_TIM_CC_EnableChannel(TIM3, LL_TIM_CHANNEL_CH4);
	LL_TIM_OC_SetCompareCH1(TIM4, 0);
	LL_TIM_CC_EnableChannel(TIM4, LL_TIM_CHANNEL_CH1);
	LL_TIM_OC_SetCompareCH2(TIM4, 0);
	LL_TIM_CC_EnableChannel(TIM4, LL_TIM_CHANNEL_CH2);
	LL_TIM_OC_SetCompareCH4(TIM4, 0);
	LL_TIM_CC_EnableChannel(TIM4, LL_TIM_CHANNEL_CH3);

	TIM2->CNT = 0;

	LL_TIM_EnableCounter(TIM5);
	LL_TIM_EnableIT_UPDATE(TIM5);

	TIM5->CNT = 0;

}

void Motor_Stop() {
	LL_TIM_DisableCounter(TIM2);
	LL_TIM_DisableIT_UPDATE(TIM2);

	LL_TIM_CC_DisableChannel(TIM3, LL_TIM_CHANNEL_CH1);
	LL_TIM_CC_DisableChannel(TIM3, LL_TIM_CHANNEL_CH2);
	LL_TIM_CC_DisableChannel(TIM3, LL_TIM_CHANNEL_CH4);
	LL_TIM_CC_DisableChannel(TIM4, LL_TIM_CHANNEL_CH1);
	LL_TIM_CC_DisableChannel(TIM4, LL_TIM_CHANNEL_CH2);
	LL_TIM_CC_DisableChannel(TIM4, LL_TIM_CHANNEL_CH3);

	LL_TIM_DisableCounter(TIM5);
	LL_TIM_DisableIT_UPDATE(TIM5);

}

void Motor_L_TIM3_IRQ() {
	MOTORL.CurrEncVal = TIM2->CNT;
	MOTORL.ErrEnc = MOTORL.CurrEncVal - MOTORL.PastEncVal;
	MOTORL.EncV = MOTORL.ErrEnc / TIME * ANGLE_PER_TICK;

	MOTORL.EncD += MOTORL.EncV * TIME; //적분

	MOTORL.ComV = MOTORL.v * ANGLE_PER_METER; //meter per time* angle per meter = angle per time
	MOTORL.ComD += MOTORL.ComV * TIME;

	float ErrV = MOTORL.EncV - MOTORL.ComV;	//현재 각속도 - 목표각속도
	float ErrD = MOTORL.EncD - MOTORL.ComD; //현재거리-목표거리

	float Cur = -(ErrV * gain_p + ErrD * gain_d);
	float Volt = Cur * MOTOR_RES + MOTORL.EncV * MOTOR_KE;

	bool dir = Volt > 0;
	Volt = MIN(ABS(Volt), ABS(batteryVolt));

	if(batteryVolt < 10) batteryVolt = 18;
	int32_t duty = TIM3->ARR * Volt / batteryVolt;
	if (dir) {
		LL_TIM_OC_SetCompareCH1(TIM3, duty);
		LL_TIM_OC_SetCompareCH2(TIM3, 0);
	} else {
		LL_TIM_OC_SetCompareCH1(TIM3, 0);
		LL_TIM_OC_SetCompareCH2(TIM3, duty);
	}
	Custom_OLED_Printf("%12d/1%f", duty, Volt);
	MOTORL.PastEncVal = MOTORL.CurrEncVal;
}
void Motor_R_TIM4_IRQ() {
	MOTORR.CurrEncVal = TIM5->CNT;
	MOTORR.ErrEnc = MOTORR.CurrEncVal - MOTORR.PastEncVal;
	MOTORR.EncV = MOTORR.ErrEnc * ANGLE_PER_TICK / TIME;

	MOTORR.EncD += MOTORR.EncV * TIME; //적분

	MOTORR.ComV = MOTORR.v * ANGLE_PER_METER; //meter per time* angle per meter = angle per time
	MOTORR.ComD += MOTORR.ComV * TIME;

	float ErrV = MOTORR.EncV - MOTORR.ComV;	//현재 각속도 - 목표각속도
	float ErrD = MOTORR.EncD - MOTORR.ComD; //현재거리-목표거리

	float Cur = -(ErrV * gain_p + ErrD * gain_d);
	float Volt = Cur * MOTOR_RES + MOTORR.EncV * MOTOR_KE;

	int32_t duty = TIM4->ARR * Volt / batteryVolt;
	bool dir = duty > 0;
	duty = ABS(duty);
	if (dir) {
		LL_TIM_OC_SetCompareCH1(TIM4, duty);
		LL_TIM_OC_SetCompareCH2(TIM4, 0);
	} else {
		LL_TIM_OC_SetCompareCH1(TIM4, 0);
		LL_TIM_OC_SetCompareCH2(TIM4, duty);
	}


	MOTORR.PastEncVal = MOTORR.CurrEncVal;
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
