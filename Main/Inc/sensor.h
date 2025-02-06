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


extern float_t batteryVolt;
//extern volatile int16_t position_value;
extern uint16_t SensorState;
extern uint8_t whiteMax[16];
extern uint8_t blackMax[16];
extern uint8_t window_start_index;
extern uint8_t window_end_index;
extern uint16_t windowCenter[15];


typedef struct{
	uint16_t LEFT;
	uint16_t CENTER;
	uint16_t RIGHT;
}window_t;

extern window_t Window;

