/*
 * sensor.c
 */

#define MIN(x,y) ((x<y)?(x):(y))
#define MAX(x,y) ((x>y)? (x):(y))

#include "main.h"
#include "sensor.h"
#include "motor.h"
#include "drive.h"
#include "custom_delay.h"
#include "custom_gpio.h"
#include "custom_oled.h"
#include "custom_switch.h"
//#include "custom_filesystem.h"
//#include "custom_exception.h"

//constants
int32_t sensor_positions[16] = {
// Left
		-30000, -26000, -22000, -18000, -14000, -10000, -6000, -2000,
// Right
		2000, 6000, 10000, 14000, 18000, 22000, 26000, 30000
//
		};
uint8_t sensorThreshold = 100;

// 배열 인덱스 순서는 라인트레이서를 위쪽에서 봤을 때 왼쪽이 0번, 오른쪽이 15번
// 비트 인덱스 순서는 동일한 조건에서 왼쪽이 MSB(&1<<15), 오른쪽이 LSB(&1<<<0)

//input
uint8_t sensorRaw[16] = { 0 };

//state

uint8_t whiteMax[16] = { 0 };
uint8_t blackMax[16] = { 0 };
volatile uint8_t normalized[16] = { 0 };
volatile uint16_t sensorState = 0;
volatile int32_t position_value = 0;
uint8_t window_start_index = 17;
uint8_t window_end_index = 0;
window_t Window;
volatile uint8_t center = 7;

//output
float_t batteryVolt;

void Sensor_Start() {
	LL_ADC_Enable(ADC1);
	LL_ADC_Enable(ADC2);
	LL_TIM_EnableCounter(TIM6);
	LL_TIM_EnableIT_UPDATE(TIM6);
	Custom_Delay_ms(10);
	center = 7;
}

void Sensor_Stop() {
	LL_ADC_Disable(ADC1);
	LL_ADC_Disable(ADC2);
	LL_TIM_DisableCounter(TIM6);
	LL_TIM_DisableIT_UPDATE(TIM6);

}

void sensor_print16(uint8_t *sensor) {
	Custom_OLED_Printf(
			"LEFT/1/g%02x/w%02x/g%02x/w%02x/g%02x/w%02x/g%02x/w%02x/2RIGHT/3/g%02x/w%02x/g%02x/w%02x/g%02x/w%02x/g%02x/w%02x",
			*sensor, *(sensor + 1), *(sensor + 2), *(sensor + 3), *(sensor + 4),
			*(sensor + 5), *(sensor + 6), *(sensor + 7), *(sensor + 8),
			*(sensor + 9), *(sensor + 10), *(sensor + 11), *(sensor + 12),
			*(sensor + 13), *(sensor + 14), *(sensor + 15));
}

__STATIC_INLINE uint16_t Sensor_ADC_Read() {
	__disable_irq();
	LL_ADC_ClearFlag_EOC(ADC1);
	LL_ADC_REG_StartConversion(ADC1);
	while (!LL_ADC_IsActiveFlag_EOC(ADC1))
		;
	uint16_t adcValue = LL_ADC_REG_ReadConversionData12(ADC1);
	LL_ADC_ClearFlag_EOC(ADC1);
	__enable_irq();
	return adcValue;
}

__STATIC_INLINE uint16_t Battery_ADC_Read() {
	__disable_irq();
	LL_ADC_ClearFlag_EOC(ADC2);
	LL_ADC_REG_StartConversion(ADC2);
	while (!LL_ADC_IsActiveFlag_EOC(ADC2))
		;
	uint16_t adcValue = LL_ADC_REG_ReadConversionData12(ADC2);
	LL_ADC_ClearFlag_EOC(ADC2);
	__enable_irq();
	return adcValue;
}

void Sensor_TIM6_IRQ() {
	static uint8_t i = 0;

	GPIOC->ODR = (GPIOC->ODR & (~0x07)) + i;
	GPIOC->ODR |= 0x08;
	Sensor_ADC_Read();
	uint8_t rawL = Sensor_ADC_Read() >> 4;
	uint8_t rawR = Sensor_ADC_Read() >> 4;
	GPIOC->ODR &= ~0x08;
	sensorRaw[i] = rawL;
	sensorRaw[i + 8] = rawR;

	// TODO: 가끔씩읽으세요 이런 안중요한건 어차피 배터리 측정쪽에 로우패스필터있어서 빨리읽어도 소용x
	// 그리고 인터럽트 핸들러에서 일 많이하면 안돼(무척중요)
	batteryVolt = Battery_ADC_Read() * 21 * 3.3 / 4095; //전압분배 해서 21베 하고 4095가 3.3V스케일로 바꾸는 것

	if (rawL < blackMax[i])
		normalized[i] = 0;
	else if (rawL > whiteMax[i])
		normalized[i] = 255;
	else
		normalized[i] = 255 * (rawL - blackMax[i])
				/ (whiteMax[i] - blackMax[i]);

	if (rawR < blackMax[i + 8])
		normalized[i + 8] = 0;
	else if (rawR > whiteMax[i + 8])
		normalized[i + 8] = 255;
	else
		normalized[i + 8] = 255 * (rawR - blackMax[i + 8])
				/ (whiteMax[i + 8] - blackMax[i + 8]);

	sensorState = (sensorState & ~(0x101 << (7 - i))) //0x101 0000 0001 0000 0001 나머지 유지 1만 바꿈
			| ((normalized[i + 8] > sensorThreshold) << (7 - i)) // 오른쪽 센서 normalized > sensor threshold 1 아니면 0
			| ((normalized[i] > sensorThreshold) << (15 - i)); // 왼쪽 센서 normalized > sensor threshold 1 아니면 0

	int32_t weighted_sum = 0;
	int32_t normalized_value = 0;
	//[-30000,300000]-> f(x)-> [0,15] (x+30000)/600000*15 = x/4000 + 7.5
	float window_start = position_value / 4000 + 4.5; //x/4000 + 7.5 - 3
	float window_end = position_value / 4000 + 10.5; //x/4000 + 7.5 + 3

	uint8_t current_window_start_index = 17;
	uint8_t current_window_end_index = 0;

	for (int sensor_index = 0; sensor_index < 16; sensor_index++) {
		if (sensor_index < window_start) {
			continue;
		}

		if (sensor_index > window_end) {
			continue;
		}

		if (sensor_index < current_window_start_index) {
			current_window_start_index = sensor_index;
		}

		if (sensor_index > current_window_end_index) {
			current_window_end_index = sensor_index;
		}

		weighted_sum += sensor_positions[sensor_index]
				* normalized[sensor_index];
		normalized_value += normalized[sensor_index];
	}

	position_value = weighted_sum / normalized_value;
	window_start_index = current_window_start_index;
	window_end_index = current_window_end_index;

	Window.center = 0;
	Window.left = 0;
	Window.right = 0;

	// Bit mask 인덱스 계산 조심할 것!
	for (int j = window_start_index; j <= window_end_index; j++) {
		Window.center |= (1 << (15 - j));
	}
	Window.left = ((uint16_t) (0xffff)) << (16 - window_start_index);
	Window.right = ((uint16_t) (0xffff)) >> (window_end_index + 1);

	i = (i + 1) & 0x07;
}

void Sensor_Test_Raw() {
	Sensor_Start();
// 센서의 Raw 값을 디스플레이에 출력해 확인하기
// --- 코드 작성 ---
	uint8_t sw;
	for (;;) {
		sensor_print16(sensorRaw);
		if (CUSTOM_SW_BOTH == (sw = Custom_Switch_Read()))
			break;
	}

	Sensor_Stop();
}

void Sensor_Calibration() {
	Sensor_Start();
	Custom_OLED_Clear();
	Custom_OLED_Printf("/w WhiteMax");
	while (CUSTOM_SW_BOTH != Custom_Switch_Read())
		;
	Custom_OLED_Clear();
	for (int i = 0;; i++) {
		i &= 0x0F;
		if (sensorRaw[i] > whiteMax[i])
			whiteMax[i] = sensorRaw[i];
		sensor_print16(whiteMax);
		if (Custom_Switch_Read())
			break;
	}
	Sensor_Stop();
	Custom_OLED_Clear();
	Custom_OLED_Printf("BlackMax");
	while (CUSTOM_SW_BOTH != Custom_Switch_Read())
		;
	Custom_OLED_Clear();
	Sensor_Start();

	for (int i = 0;; i++) {
		i &= 0x0F;
		if (sensorRaw[i] > blackMax[i])
			blackMax[i] = sensorRaw[i];
		sensor_print16(blackMax);
		if (Custom_Switch_Read())
			break;
	}

// Calculate ADC coefficients
// --- 코드 작성 ---

	Sensor_Stop();
}

void position_test() {
	Sensor_Start();
	while (Custom_Switch_Read() != CUSTOM_SW_BOTH) {
		int32_t sigma_p = 0;
		int32_t normalized_value = 0;
		for (int i = 0; i < 16; i++) {
			sigma_p += sensor_positions[i] * normalized[i];
			normalized_value += normalized[i];
		}

		if (!normalized_value)
			position_value = 0;
		else
			position_value = sigma_p / normalized_value;
		Custom_OLED_Printf("%6d", position_value);
	}
}

void window_position_test() {
	Sensor_Start();
	while (Custom_Switch_Read() != CUSTOM_SW_BOTH) {
		Custom_OLED_Printf("%6d", position_value);
	}
}

void test_window() {
	uint8_t sw;
	Sensor_Start();
	while (CUSTOM_SW_BOTH != (sw = Custom_Switch_Read())) {
		Custom_OLED_Printf("%4x/1%4x/2%4x", Window.left, Window.center,
				Window.right);
	}
	Sensor_Stop();
}

void sensor_state_test() {
	uint8_t sw;
	Sensor_Start();
	while (CUSTOM_SW_BOTH != (sw = Custom_Switch_Read())) {
		char stateString[17] = { 0 };
		for (int i = 0; i < 16; i++) {
			stateString[15 - i] = '0' + !!(sensorState & (1 << i));
		}
		Custom_OLED_Printf("%s", stateString);
	}
	Sensor_Stop();
}
