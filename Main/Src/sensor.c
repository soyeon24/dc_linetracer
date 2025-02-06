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

uint8_t sensorRaw[16] = { 0 };
float_t batteryVolt;
uint8_t whiteMax[16] = { 0 };
uint8_t blackMax[16] = { 0 }; //{ 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20 };
uint8_t normalized[16] = { 0 };
uint8_t sw;
int32_t position[16] = { -30000, -26000, -22000, -18000, -14000, -10000, -6000,
		-2000, 2000, 6000, 10000, 14000, 18000, 22000, 26000, 30000 };
uint16_t windowCenter[15] = { 0xf000, 0xf800, 0xfc00, 0x7e00, 0x3f00, 0x1f80,
		0x0fc0, 0x07e0, 0x03f0, 0x01f8, 0x00fc, 0x007e, 0x003f, 0x001f, 0x000f };
//uint32_t positionCenter[15] = { -28000, -24000, -20000, -16000, -12000, -8000,
//		-4000, 0, 4000, 8000, 12000, 16000, 20000, 24000, 28000 };

uint16_t SensorState = 0;
static volatile int32_t position_value = 0;
uint8_t sensorThreshold = 100;

uint8_t window_start_index = 17;
uint8_t window_end_index = 0;

window_t Window;
volatile uint8_t center = 7;

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
	static uint32_t i = 0;
	GPIOC->ODR = (GPIOC->ODR & (~0x07)) + i;
	GPIOC->ODR |= 0x08;
	Sensor_ADC_Read();
	uint8_t rawL = Sensor_ADC_Read() >> 4;
	uint8_t rawR = Sensor_ADC_Read() >> 4;
	GPIOC->ODR &= ~0x08;
	sensorRaw[i] = rawL;
	sensorRaw[i + 8] = rawR;

	batteryVolt = Battery_ADC_Read() * 21 * 3.3 / 4095;

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

	SensorState = (SensorState & ~(0x101 << i))
			| ((normalized[i] > sensorThreshold) << (i))
			| ((normalized[i + 8] > sensorThreshold) << (i + 8));

	int32_t weighted_sum = 0;
	int32_t normalized_value = 0;

	float window_start = position_value / 4000 + 4.5;
	float window_end = position_value / 4000 + 10.5;

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

		weighted_sum += position[sensor_index] * normalized[sensor_index];
		normalized_value += normalized[sensor_index];
	}

	position_value = weighted_sum / normalized_value;
	window_start_index = current_window_start_index;
	window_end_index = current_window_end_index;
	Window.CENTER = 0;
	Window.LEFT = 0;
	Window.RIGHT = 0;
	for (int j = window_start_index; j <= window_end_index; j++){
		Window.CENTER |= (1 << (15 - j));
	}
	Window.LEFT = (0xffff<<(15-window_start_index));
	Window.RIGHT = (0xffff>>window_end_index);

	/*
	 uint8_t windowC_start = 0;
	 uint8_t count = 0;
	 if (i == 0x07) {
	 int32_t sigma_p = 0;
	 int32_t normalized_value = 0;
	 for (int j = 15; j >= 0; j--) {
	 if (j < (position_value / 4000 + 4.5))
	 continue;
	 else if (j > (position_value / 4000 + 10.5))
	 continue;

	 sigma_p += position[j] * normalized[j];
	 normalized_value += normalized[j];
	 windowC_start = j;
	 count++;

	 }
	 }*/
	i = (i + 1) & 0x07;

}
/*
 void Calc_Position() {
 center += (position_value - positionCenter[center]) / 2000;
 Window.CENTER = windowCenter[center];
 int32_t sigma_p = 0;
 int32_t normalized_value = 0;
 for (uint8_t k = MAX(0, center - 2); k < MIN(16, center + 4); k++) {
 sigma_p += position[k] * normalized[k];
 normalized_value += normalized[k];
 }
 if (!normalized_value)
 position_value = 0;
 else
 position_value = sigma_p / normalized_value;
 }*/

void Sensor_Test_Raw() {
	Sensor_Start();
// 센서의 Raw 값을 디스플레이에 출력해 확인하기
// --- 코드 작성 ---
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
			sigma_p += position[i] * normalized[i];
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
/*
void window_position(int16_t position) {
	if (position < -26000) {
		Window.CENTER = 0xf000;
		Window.LEFT = 0;
		Window.RIGHT = 0x0FFF;
	} else if (position < -22000) {
		Window.CENTER = 0xf800;
		Window.LEFT = 0;
		Window.RIGHT = 0x07ff;
	} else if (position < -18000) {
		Window.CENTER = 0xfc00;
		Window.LEFT = 0;
		Window.RIGHT = 0x3ff;
	} else if (position < -14000) {
		Window.CENTER = 0x7e00;
		Window.LEFT = 0x8000;
		Window.RIGHT = 0x01ff;
	} else if (position < -10000) {
		Window.CENTER = 0x3f00;
		Window.LEFT = 0xc00;
		Window.RIGHT = 0x00ff;
	} else if (position < -6000) {
		Window.CENTER = 0x1f80;
		Window.LEFT = 0xe000;
		Window.RIGHT = 0x007f;
	} else if (position < -2000) {
		Window.CENTER = 0x0fc0;
		Window.LEFT = 0xf000;
		Window.RIGHT = 0x003f;
	} else if (position > 26000) {
		Window.CENTER = 0x000f;
		Window.LEFT = 0xfff0;
		Window.RIGHT = 0;
	} else if (position > 22000) {
		Window.CENTER = 0x001f;
		Window.LEFT = 0xffe0;
		Window.RIGHT = 0;

	} else if (position > 18000) {
		Window.CENTER = 0x003f;
		Window.LEFT = 0xffc0;
		Window.RIGHT = 0;
	} else if (position > 14000) {
		Window.CENTER = 0x007e;	//0000000001111110
		Window.LEFT = 0xff80;
		Window.RIGHT = 0x0001;
	} else if (position > 10000) {
		Window.CENTER = 0x00fc;	//00000001.1111.1000
		Window.LEFT = 0xff00;
		Window.RIGHT = 0x0003;
	} else if (position > 6000) {
		Window.CENTER = 0x01f8;
		Window.LEFT = 0xfe00;
		Window.RIGHT = 0x0007;

	} else if (position > 2000) {
		Window.CENTER = 0x03f0;
		Window.LEFT = 0xfc00;
		Window.RIGHT = 0x000f;

	} else {
		Window.CENTER = 0x07e0;
		Window.LEFT = 0xf800;
		Window.RIGHT = 0x001f;
	}
}*/

void test_window() {
	uint8_t sw;
	Sensor_Start();
	while (CUSTOM_SW_BOTH != (sw = Custom_Switch_Read())) {
		Custom_OLED_Printf("%4x/1%4x/2%4x", Window.LEFT, Window.CENTER,
				Window.RIGHT);
	}
	Sensor_Stop();
}

void sensor_state_test() {
	uint8_t sw;
	Sensor_Start();
	while (CUSTOM_SW_BOTH != (sw = Custom_Switch_Read())) {
		Custom_OLED_Printf("%6d", SensorState);
	}
	Sensor_Stop();
}
