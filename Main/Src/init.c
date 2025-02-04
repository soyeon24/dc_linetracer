/*
 * init.c
 */

#include <stdio.h>
#include "init.h"
#include "motor.h"
#include "drive.h"
#include "sensor.h"
#include "custom_oled.h"
#include "custom_switch.h"
#include "custom_exception.h"
#include "custom_filesystem.h"
#define SENSOR_SETTING_FILE "Sensor Setting"
/*
 * syscalls.c에 있는 _io_putchar 재정의
 */
//int __io_putchar(int ch)
//{
//	while (!LL_USART_IsActiveFlag_TXE(USART2));
//	LL_USART_TransmitData8(USART2, (char)ch);
//	return ch;
//}

typedef struct {
	uint8_t num;
	int32_t adcCoeffRange[SENSOR_NUM];
	int32_t adcCoeffBias[SENSOR_NUM];
	int32_t sensorThreshold;
} SensorSetting_t;


typedef struct {
	char name[30];
	void (*func)();
} In_Menu;



uint8_t count = 0;

//static bool Sensor_Setting_Save() {
//   // 텅 빈 설정 구조체를 만든다.
//   SensorSetting_t setting = { 0 };
//
//   // 현재 설정 값을 설정 구조체에 복사한다.
//
//   setting.num = count;
//
//   // 복사한 구조체를 저장한다.
//   bool saveSuccess = Custom_FileSystem_Write(
//   SENSOR_SETTING_FILE,
//         (uint8_t*) &setting,
//         sizeof(SensorSetting_t)
//         );
//   ASSERT(saveSuccess);
//   bool flushSuccess = Custom_FileSystem_Flush();
//   ASSERT(flushSuccess);
//   return true;
//}
//
//static bool Sensor_Setting_Load() {
//   // 텅 빈 설정 구조체를 만든다.
//   SensorSetting_t setting = { 0 };
//
//   // 설정 구조체에 설정 정보를 쓴다.
//   FileInfo_t *file = Custom_FileSystem_Find(SENSOR_SETTING_FILE);
//   bool loadSuccess = Custom_FileSystem_Read(
//         file,
//         (uint8_t*) &setting,
//         sizeof(SensorSetting_t)
//         );
//
//   if (loadSuccess) {
//      // 만약 읽어오기에 성공했다면 현제 설정 값을 업데이트한다.
//
//      // 현재 설정 값을 설정 구조체에 복사한다.
//     count = setting.num;
//   }
//   return loadSuccess;
//}
void check_delay() {
	Custom_OLED_Printf("a");
	Custom_Delay_ms(1000);
	Custom_OLED_Clear();
}

In_Menu in_menu[] = {
		{"/wcheck delay", check_delay},
		{"sensor Raw", Sensor_Test_Raw},
		{"window test", test_window},
		{"calibration", Sensor_Calibration},
		{"first drive", Drive_First},
		{"mark check", mark_check},
		{"state debug",state_debug}
};


void Init() {
	/*
	 * STM 보드와 컴퓨터 간 UART 통신을 통해 컴퓨터 터미널로 디버깅할 수 있도록 USART2를 활성화한다.
	 */
//	LL_USART_Enable(USART2);

	Custom_Delay_Init_SysTick();

	/*
	 * OLED를 사용하기 전에는 Custom_OLED_Init 함수를 호출하여 여러가지 초기화를 수행해야 한다.
	 * 이 함수는 OLED 처음 쓰기 전에 딱 한 번만 호출하면 된다.
	 */
	Custom_OLED_Init();

	/*
	 * 플래시를 사용하기 전에는 Custom_FileSystem_Load 함수를 호출하여 플래시 정보를 불러와야 한다.
	 * 이 함수는 플래시를 처음 쓰기 전에 딱 한 번만 호출하면 된다.
	 */
	Custom_FileSystem_Load();

	/**
	 * Custom_OLED_Printf 함수는 C언어에서 printf와 동일하게 동작한다.
	 * 즉, %d, %f 등의 서식 문자를 사용하여 숫자를 출력할 수 있다.
	 * 다만 특수한 기능들 추가되어있는데, /0이라는 부분이 있으면 첫 번째 줄의 첫 번째 칸으로 돌아가고,
	 * /1이라는 부분이 있으면 두 번째 줄의 첫 번째 칸으로 돌아간다.
	 * 그리고 /r, /g, /b라는 부분이 있으면 각각 문자를 빨강, 초록, 파랑으로 출력한다.
	 * 즉, 아래 예제에서는 첫 번째 줄에 "Hello"를 출력한 후, 두 번째 줄의 첫 번째 칸으로 커서가 이동하고 파란색 글씨로 "ZETIN!"을 쓴다.
	 */

	Custom_OLED_Printf("/0Hello, /1/bZETIN!");
	Custom_Delay_ms(500);
	Custom_OLED_Clear();

	/*
	 * 아래는 스위치를 사용하는 예제다.
	 * Custom_Switch_Read 함수 내부에는 1ms 딜레이가 존재하기 때문에, 이 함수를 주행 알고리즘 내부에 집어넣으면 성능이 크게 떨어지니 주의한.
	 */
	uint8_t sw =0;
	int8_t counting =0;


	int num_of_menu = sizeof(in_menu) / sizeof(In_Menu);
		//위의 In menu 구조체의 연속을 배열로 표현한 것임 배열1칸(?)이 구조체 하나와 같은거
		Custom_OLED_Clear();
		for (;;) {

			while (CUSTOM_SW_BOTH != (sw = Custom_Switch_Read())) {
				if (sw == CUSTOM_SW_1) {
					counting--;
					Custom_OLED_Clear();
				}
				else if (sw == CUSTOM_SW_2) {
					counting++;
					Custom_OLED_Clear();
				}
				counting = (counting + num_of_menu) % num_of_menu;
				Custom_OLED_Printf(" %s", in_menu[counting].name);
			}
			Custom_OLED_Clear();
			in_menu[counting].func();
			Custom_OLED_Clear();
			Custom_OLED_Printf("BYE!");
		}


}
