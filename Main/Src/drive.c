/*
 * drive.c
 */

#include <stdint.h>
#include <stdbool.h>
#include "init.h"
#include "drive.h"
#include "motor.h"
#include "sensor.h"
#include "custom_oled.h"
#include "custom_switch.h"
//#include "custom_exception.h"
//#include "custom_filesystem.h"

#define STATE_IDLE 0
#define STATE_CROSS 122
#define STATE_MARK 2
#define STATE_DECISION 3
#define ABS(x) ((x>0) ? x:(-x))

#define MARK_NONE 0
#define MARK_CROSS 8
#define MARK_LEFT 1
#define MARK_RIGHT 2
#define MARK_END 4

uint8_t mark_read[400];
int32_t positionCenter[15] = { -28000, -24000, -20000, -16000, -12000, -8000,
		-4000, 0, 4000, 8000, 12000, 16000, 20000, 24000, 28000 };

volatile uint16_t sensorState_Sum = 0;
volatile uint8_t state = STATE_IDLE;
volatile int index_markcnt = 0;

volatile float current_velocity;
volatile float target_velocity;
volatile float target_velocity_setting = 2.f;
volatile float accel;
volatile float accel_setting = 4.5f;
volatile float deccel_Setting = 8.0f;
volatile float deccel;
volatile float pit_in_line;
volatile float curve_rate = 0.000068f;

float curve_deccel = 19000.f;


void Drive_TIM7_IRQ() {
	if (current_velocity < target_velocity) {
		current_velocity += accel * 0.0005f;
		if (current_velocity > target_velocity){
			current_velocity = target_velocity;
		}
	}
	else if (current_velocity >= target_velocity) {
		current_velocity -= deccel * 0.0005f; //0.0005초마다 불러오는 타이머 이기때문
		if (current_velocity < target_velocity) {
			current_velocity = target_velocity;
		}
	}
	float velocity_center = current_velocity * curve_deccel / (curve_deccel + position_value);

		MotorR.v = velocity_center* (1 - curve_rate * (float) position_value);
		MotorL.v = velocity_center * (1 + curve_rate * (float) position_value);
}

void Drive_Start() {
	LL_TIM_EnableCounter(TIM7);
	LL_TIM_EnableIT_UPDATE(TIM7);

}

void Drive_Stop() {
	LL_TIM_DisableCounter(TIM7);
	LL_TIM_DisableIT_UPDATE(TIM7);
}

__STATIC_INLINE uint8_t state_machine() {

	uint8_t mark = 0;

	static bool cross_decision = false;

	switch (state) {
	case STATE_IDLE:

		if ((__builtin_popcount(SensorState & Window.CENTER)) > 4) {
			state = STATE_CROSS;
		} else if (SensorState & (~Window.CENTER)) {
			state = STATE_MARK;
		} else
			state = STATE_IDLE;
		break;

	case STATE_CROSS:

		if ((!(SensorState & ~(Window.CENTER)))
				&& (sensorState_Sum == 0xffff)) {
			cross_decision = true;
			state = STATE_DECISION;
		} else {
			sensorState_Sum |= SensorState;
			state = STATE_CROSS;
		}
		break;
	case STATE_MARK:
		if ((!(SensorState & ~(Window.CENTER)))) {
			state = STATE_DECISION;
			break;
		} else {
			sensorState_Sum |= (SensorState & (~Window.CENTER));
		}
		break;
	case STATE_DECISION:

		if (cross_decision) {
			mark = MARK_CROSS;
			cross_decision = false;
		} else if ((sensorState_Sum & Window.LEFT)
				&& (sensorState_Sum & Window.RIGHT)) {
			mark = MARK_END;
		} else if (sensorState_Sum & Window.LEFT) {
			mark = MARK_LEFT;
		} else if (sensorState_Sum & Window.RIGHT) {
			mark = MARK_RIGHT;
		}
		sensorState_Sum = 0;
		state = STATE_IDLE;
		return mark;

	}
	return MARK_NONE;

}

void Drive_First() {
	volatile uint8_t temp_mark_read[400];
	volatile uint8_t endmark_cnt = 0;
	volatile uint8_t cross_cnt = 0;
	volatile uint8_t markL_cnt = 0;
	volatile uint8_t markR_cnt = 0;
	volatile uint8_t mark;
	current_velocity = 0;

	accel = accel_setting;
	target_velocity = target_velocity_setting;

	uint32_t index_mark = 0;

	if (whiteMax[1] - blackMax[1] == 0) {
		while (1) {
			Custom_OLED_Printf("/r do cali");
		}
	}

	Sensor_Start();
	Motor_Start();
	Drive_Start();

	while (endmark_cnt < 2) {

		mark = state_machine();
		if (mark == MARK_END) {
			endmark_cnt++;

		} else if (mark == MARK_CROSS) {
			cross_cnt++;
		} else if (mark == MARK_LEFT) {
			markL_cnt++;
		} else if (mark == MARK_RIGHT) {
			markR_cnt++;
		}
		if (!mark) {
			temp_mark_read[index_mark] = mark;
			index_mark++;
		}
		if (!SensorState) {
			break;
		}
	}

	deccel = (current_velocity * current_velocity) / (2 * pit_in_line);
	target_velocity = 0;

	while (current_velocity > 0)
		;

	Motor_Stop();
	Sensor_Stop();
	Drive_Stop();

	for (int i = 0; i < index_mark; i++) {
		mark_read[i] = temp_mark_read[i];
	}

	uint8_t sw = 0;
	if (mark == MARK_END) {
		Custom_OLED_Printf("end mark");
	} else {
		Custom_OLED_Printf("line out");
	}
	while ((sw = Custom_Switch_Read()) != CUSTOM_SW_BOTH) {
	}
	Custom_OLED_Clear();
	Custom_OLED_Printf(
			"end %d/1cross %d/2Left %d/3Right %d/4save?right/5no?left",
			endmark_cnt, cross_cnt, markL_cnt, markR_cnt);
	index_markcnt = index_mark;
	while (1) {
		sw = Custom_Switch_Read();
		if (sw == CUSTOM_SW_2) {
			for (int i = 0; i < (index_markcnt); i++) {
				mark_read[i] = temp_mark_read[i];
			}
			break;
		} else if (sw == CUSTOM_SW_1) {
			break;
		}
	}
	Custom_OLED_Clear();
}

void state_debug() {
	uint8_t endmark_cnt = 0;
	uint8_t mark;
	Sensor_Start();

	int prev_mark = 0;

	while (Custom_Switch_Read() != CUSTOM_SW_BOTH) {
		mark = state_machine();
		if (mark == 1)
			Custom_OLED_Printf("left  ");
		else if (mark == 2)
			Custom_OLED_Printf("right ");
		else if (mark == 3)
			Custom_OLED_Printf("end   ");
		else if (mark == 8)
			Custom_OLED_Printf("cross ");
		else if (mark == 0)
			Custom_OLED_Printf("NONE  ");

		if (mark == MARK_END)
			endmark_cnt++;
		if (prev_mark != mark)
			Custom_Delay_ms(500);
		prev_mark = mark;
	}
	Sensor_Stop();
}

void mark_check() {
	int markcheaki = 0;
	while ((sw = Custom_Switch_Read()) != CUSTOM_SW_BOTH) {

		if (sw == CUSTOM_SW_1) {
			markcheaki--;
		} else if (sw == CUSTOM_SW_2) {
			markcheaki++;
		}
		Custom_OLED_Printf("%d ", mark_read[markcheaki]);
	}

	Custom_OLED_Clear();

	markcheaki = 0;
}

void velocity_test() {
	Custom_OLED_Clear();
	current_velocity = 0.f;
	accel = accel_setting;
	deccel = deccel_Setting;
	Drive_Start();
	target_velocity = target_velocity_setting;
	while ((sw = Custom_Switch_Read()) != CUSTOM_SW_BOTH) {
		Custom_OLED_Printf("/3in ");
		Custom_OLED_Printf("/4current %f", current_velocity);
		Custom_OLED_Printf("/5target %f", target_velocity);
		if (sw == CUSTOM_SW_1) {
			target_velocity -= 0.1;
		} else if (sw == CUSTOM_SW_2) {
			target_velocity += 0.1;
		}
	}
	Drive_Stop();
}

