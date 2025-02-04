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

typedef struct motor {
	volatile uint32_t CurrEncVal; //현재 엔코더
	volatile uint32_t PastEncVal; //이전 엔코더
	volatile int32_t ErrEnc;	// 현재 - 이전 엔코더 == 거리 차이
	volatile float EncV; //앤코더 변화 속도 (=각속도)
	volatile float EncD; // 엔코더 변화 거리(=속도의 시간대비 적분값)
	volatile float ComV; //커맨드 속도 (=목표 속도)
	volatile float ComD; //커맨드 위치 (=목표 거리)
	volatile float v; //바퀴속도
	volatile int32_t Duty;

} motor;

float gain_p = 1.0f;
float gain_d = 0.f;

struct motor MotorL;
struct motor MotorR;

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
	MotorL.CurrEncVal = TIM2->CNT;
	MotorL.ErrEnc = (int32_t) MotorL.CurrEncVal - MotorL.PastEncVal;
	MotorL.EncV = MotorL.ErrEnc / TIME * ANGLE_PER_TICK;
	MotorL.EncD += MotorL.EncV * TIME; //적분

	MotorL.ComV = MotorL.v * ANGLE_PER_METER; //meter per time* angle per meter = angle per time
	MotorL.ComD += MotorL.ComV * TIME;

	float ErrV = MotorL.EncV - MotorL.ComV;	//현재 각속도 - 목표각속도
	float ErrD = MotorL.EncD - MotorL.ComD; //현재 거리-목표 거리

	float Cur = -(ErrV * gain_p + ErrD * gain_d);
	float Volt = Cur * MOTOR_RES + MotorL.EncV * MOTOR_KE;

	bool dir = Volt > 0;

	if (batteryVolt < 10)
		batteryVolt = 18;
	Volt = MIN(ABS(Volt), batteryVolt);

	int32_t duty = TIM3->ARR * Volt / batteryVolt;
	MotorL.Duty = duty;

	if (dir) {
		LL_TIM_OC_SetCompareCH1(TIM3, duty);
		LL_TIM_OC_SetCompareCH2(TIM3, 0);
	} else {
		LL_TIM_OC_SetCompareCH1(TIM3, 0);
		LL_TIM_OC_SetCompareCH2(TIM3, duty);
	}
	MotorL.PastEncVal = MotorL.CurrEncVal;
}
void Motor_R_TIM4_IRQ() {
	MotorR.CurrEncVal = -TIM5->CNT;
	MotorR.ErrEnc = (int32_t) MotorR.CurrEncVal - MotorR.PastEncVal;
	MotorR.EncV = MotorR.ErrEnc * ANGLE_PER_TICK / TIME;
	MotorR.EncD += MotorR.EncV * TIME; //적분

	MotorR.ComV = MotorR.v * ANGLE_PER_METER; //meter per time* angle per meter = angle per time
	MotorR.ComD += MotorR.ComV * TIME;

	float ErrV = MotorR.EncV - MotorR.ComV;	//현재 각속도 - 목표각속도
	float ErrD = MotorR.EncD - MotorR.ComD; //현재거리-목표거리

	float Cur = -(ErrD * gain_p + ErrV * gain_d);
	float Volt = Cur * MOTOR_RES + MotorR.EncV * MOTOR_KE;

	bool dir = Volt > 0;

	if (batteryVolt < 10)
		batteryVolt = 18;
	Volt = MIN(batteryVolt, ABS(Volt));

	int32_t duty = TIM4->ARR * Volt / batteryVolt;
	MotorR.Duty = duty;

	if (dir) {
		LL_TIM_OC_SetCompareCH1(TIM4, duty);
		LL_TIM_OC_SetCompareCH2(TIM4, 0);
	} else {
		LL_TIM_OC_SetCompareCH1(TIM4, 0);
		LL_TIM_OC_SetCompareCH2(TIM4, duty);
	}

	MotorR.PastEncVal = MotorR.CurrEncVal;
}

void Motor_Test_Phase() {
	/*
	 * 모터의 각 상을 잠깐씩 잡아본다.
	 * 모터에 무리를 주지 않기 위해 100ms 동안만 상을 잡은 후 바로 놓는다.
	 */
	// --- Write your code ---
	Motor_Start();
	Sensor_Start();
	uint8_t sw;
	while(CUSTOM_SW_BOTH != (sw=Custom_Switch_Read())){
		if(sw == CUSTOM_SW_1) gain_p / 2.f;
		else if(sw==CUSTOM_SW_2) gain_p *= 2.f;
		Custom_OLED_Printf("%8f", gain_p);
		Custom_OLED_Printf("%1%8d", TIM2->CNT);
		Custom_OLED_Printf("/2%8d",MotorL.ErrEnc );
		Custom_OLED_Printf("/3%f", MotorL.EncV);
		Custom_OLED_Printf("/4%f", MotorL.EncD);
		Custom_OLED_Printf("/5%8d", MotorL.Duty);
	}

	Motor_Stop();
	Sensor_Stop();

}

void Motor_Test_Velocity() {
	/*
	 * 모터 속도를 부드럽게 올렸다가 내리기를 반복한다.
	 */

}
