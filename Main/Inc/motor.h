/*
 * motor.h
 */

#ifndef INC_MOTOR_H_
#define INC_MOTOR_H_

void Motor_Start();
void Motor_Stop();
void Motor_L_TIM3_IRQ();
void Motor_R_TIM4_IRQ();
void Motor_Test_Velocity();
void Motor_Test_Phase();
void Motor_velocity_change();

typedef struct {
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

extern motor MotorL;
extern motor MotorR;

extern float curve_decel;

#endif /* INC_MOTOR_H_ */
