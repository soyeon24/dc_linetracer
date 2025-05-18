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

//constants
#define STATE_IDLE 0
#define STATE_CROSS 1
#define STATE_MARK 2
#define STATE_DECISION 3
#define STATE_CROSS_DECISION 4
#define ABS(x) ((x>0) ? x:(-x))

#define MARK_NONE 0
#define MARK_CROSS 8
#define MARK_LEFT 1
#define MARK_RIGHT 2
#define MARK_END 4

int32_t positionCenter[15] = { -28000, -24000, -20000, -16000, -12000, -8000,
		-4000, 0, 4000, 8000, 12000, 16000, 20000, 24000, 28000 };

volatile float accel;
volatile float accel_setting = 4.5f;
volatile float decel_Setting = 8.0f;
volatile float decel;
volatile float pit_in_line = 0.2f;
volatile float curve_rate = 0.000068f;
float curve_decel = 19000.f;

//state
volatile uint32_t sensorStateSum = 0;
volatile uint8_t state = STATE_IDLE;
volatile int index_markcnt = 0;

volatile float current_velocity;
volatile float target_velocity;
volatile float target_velocity_setting = 1.f;

//output
uint8_t mark_read[400];
uint32_t mark_length[400];
uint8_t index_length = 0;

void Drive_TIM7_IRQ() {
	if (current_velocity < target_velocity) {
		current_velocity += accel * 0.0005f;
		if (current_velocity > target_velocity) {
			current_velocity = target_velocity;
		}
	} else if (current_velocity >= target_velocity) {
		current_velocity -= decel * 0.0005f; //0.0005초마다 불러오는 타이머이기 때문
		if (current_velocity < target_velocity) {
			current_velocity = target_velocity;
		}
	}
	float velocity_center = current_velocity * curve_decel
			/ (curve_decel + ABS(position_value));
	MotorR.v = velocity_center * (1 - curve_rate * (float) position_value);
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

__STATIC_INLINE uint32_t Center_State(int32_t position, uint16_t state) {
	int32_t position_index = (position + 30000 + 2000) / 4000;
	//포지션 값은 -30000~30000인데 인덱스는 0부터 2000을 더해준 이유는 포지션 중 스케일 4000 중 0~2000은 왼쪽 센서를 2001~4000은 오른쪽으로
	uint32_t extended_state = (uint32_t) (state);
	uint32_t centered_state = extended_state << position_index;
	return centered_state;
}

__STATIC_INLINE uint8_t State_Machine() {
	uint16_t windowMask = 0;
	for (int j = window_start_index; j <= window_end_index; j++) {
		windowMask |= (1 << (15 - j));
	}
	uint16_t markerMask = ~windowMask;
	bool isMarkerDetected = sensorState & markerMask;

	switch (state) {
	case STATE_IDLE:
		if (__builtin_popcount(sensorState & windowMask) > 4
				|| isMarkerDetected) {
			// 윈도우 내에 4개 이상의 센서에서 라인이 감지되면 또는 마커에 하나라도 감지가 된다면
			sensorStateSum = Center_State(position_value, sensorState);
			state = STATE_MARK;
		}
		break;

	case STATE_MARK:
		if (!isMarkerDetected) {
			state = STATE_DECISION;
			break;
		}

		sensorStateSum |= Center_State(position_value, sensorState);
		break;

	case STATE_DECISION:
		state = STATE_IDLE;

		uint32_t MASK_LEFT = ((uint32_t) 0xFFFFFFFF) << 19;
		uint32_t MASK_RIGHT = ((uint32_t) 0xFFFFFFFF) >> 20;

		bool isLeftDetected = sensorStateSum & MASK_LEFT;
		bool isRightDetected = sensorStateSum & MASK_RIGHT;
		bool isSensorStateFull = (sensorStateSum & 0x007FFF00) == 0x007FFF00;

		if (isSensorStateFull) {
			return MARK_CROSS;
		}

		if (isLeftDetected && isRightDetected) {
			return MARK_END;
		}

		if (isLeftDetected) {
			return MARK_LEFT;
		}

		if (isRightDetected) {
			return MARK_RIGHT;
		}
	}
	return MARK_NONE;
}

void Drive_First() {
//output
	uint8_t temp_mark_read[400];
	uint8_t endmark_cnt = 0;
	uint8_t cross_cnt = 0;
	uint8_t markL_cnt = 0;
	uint8_t markR_cnt = 0;
	uint32_t index_mark = 0;
	uint8_t mark;
	current_velocity = 0;

//input
	accel = accel_setting;
	target_velocity = target_velocity_setting;
	decel = decel_Setting;

	if (whiteMax[1] - blackMax[1] == 0) {
		while (1) {
			Custom_OLED_Printf("/r do cali");
		}
	}

	Sensor_Start();
	Motor_Start();
	Drive_Start();

	while (endmark_cnt < 2) {
		mark = State_Machine();
		if (mark == MARK_END) {
			endmark_cnt++;
		} else if (mark == MARK_CROSS) {
			cross_cnt++;
		} else if (mark == MARK_LEFT) {
			markL_cnt++;
		} else if (mark == MARK_RIGHT) {
			markR_cnt++;
		}

		if (mark) {
			temp_mark_read[index_mark] = mark;
			index_mark++;
		}

		if (!sensorState) {
			break;
		}
	}

	decel = (current_velocity * current_velocity) / (2 * pit_in_line);
	target_velocity = 0;

	while (current_velocity > 0)
		;
	Custom_Delay_ms(100);
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
		mark = State_Machine();
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
	uint8_t sw = 0;
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
	uint8_t sw = 0;
	Custom_OLED_Clear();
	current_velocity = 0.f;
	accel = accel_setting;
	decel = decel_Setting;
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

