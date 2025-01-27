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

uint16_t positionCenter[15] = { -28000, -24000, -20000, -16000, -12000, -8000,
		-4000, 0, 4000, 8000, 12000, 16000, 20000, 24000, 28000 };
uint16_t center;
volatile uint16_t sensorState_Sum = 0;

void Drive_Start() {
	LL_TIM_EnableCounter(TIM7);
	LL_TIM_EnableIT_UPDATE(TIM7);
}

void Drive_Stop(){
	LL_TIM_DisableCounter(TIM7);
	LL_TIM_DisableIT_UPDATE(TIM7);
}

__STATIC_INLINE state_machine() {
	uint8_t mark = 0;
	uint8_t state;
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
		} else if ((sensorState_Sum&Window.LEFT)&&(sensorState_Sum&Window.RIGHT)) {
			mark = MARK_END;
		}
		else if(sensorState_Sum&Window.LEFT){
			mark = MARK_LEFT;
		}
		else if(sensorState_Sum&Window.RIGHT){
			mark = MARK_RIGHT;
		}

		sensorState_Sum = 0;
		state = STATE_IDLE;
		return mark;
	}
	return MARK_NONE;

}

void Drive_First() {
	center = 7;
	if (whiteMax[1] - blackMax[1] == 0) {
		while (1) {
			Custom_OLED_Printf("/r do cali");
		}
	}
	Sensor_Start();
	Motor_Start();
	Drive_Start();


	Motor_Stop();
	Sensor_Stop();
	Drive_Stop();
}
