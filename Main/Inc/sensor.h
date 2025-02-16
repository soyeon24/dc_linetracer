/*
 * sensor.h
 */
#define SENSOR_NUM 8

void Sensor_Start();
void Sensor_Stop();
void Sensor_Test_Raw();
void Sensor_TIM6_IRQ();
void test_window();
void Sensor_Calibration();
void sensor_state_test();
void window_position_test();
void Calc_Position();
void sensor_print16();

extern float_t batteryVolt;
//extern volatile int16_t position_value;
extern volatile uint16_t sensorState;
extern uint8_t whiteMax[16];
extern uint8_t blackMax[16];
extern uint8_t window_start_index;
extern uint8_t window_end_index;
extern uint16_t windowCenter[15];
extern uint8_t sensorThreshold;
extern volatile int32_t position_value;

typedef struct{
	uint16_t left;
	uint16_t center;
	uint16_t right;
}window_t;

extern window_t Window;

