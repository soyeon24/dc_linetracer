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
uint16_t positionCenter[15] = { -28000, -24000, -20000, -16000, -12000, -8000,
		-4000, 0, 4000, 8000, 12000, 16000, 20000, 24000, 28000 };

volatile uint16_t sensorState_Sum = 0;
volatile uint8_t state = STATE_IDLE;
volatile int index_markcnt = 0;
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
		} else if ((__builtin_popcount(SensorState & (~Window.CENTER)))) {
			state = STATE_MARK;
		}

	case STATE_CROSS:
		if ((!(__builtin_popcount(SensorState & (~Window.CENTER))))
				&& (sensorState_Sum == 0xffff)) {
			cross_decision = true;
			state = STATE_DECISION;
		} else {
			sensorState_Sum |= SensorState;
			state = STATE_CROSS;
		}
	case STATE_MARK:
		if ((!(__builtin_popcount(SensorState & (~Window.CENTER))))) {
			state = STATE_DECISION;
			break;
		}
		sensorState_Sum |= (sensorState_Sum & (~Window.CENTER));
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
		if(!mark){
			temp_mark_read[index_mark] = mark;
			index_mark++;
		}
		if (!(SensorState & 0xffff)) {
			break;
		}
	}
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

