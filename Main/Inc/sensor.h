/*
 * sensor.h
 */
#define SENSOR_NUM 8

void Sensor_Start();
void Sensor_Stop();
void Sensor_Test_Raw();
void Sensor_TIM6_IRQ();
void test_window();

extern float_t batteryVolt;

