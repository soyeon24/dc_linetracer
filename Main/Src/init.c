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
void threshold_change() {
	uint8_t sw = 0;
	while ((sw = Custom_Switch_Read()) != CUSTOM_SW_BOTH) {
		if (sw == CUSTOM_SW_1) {
			sensorThreshold--;
		} else if (sw == CUSTOM_SW_2) {
			sensorThreshold++;
		}

		Custom_OLED_Printf("th /1 %d", sensorThreshold);
	}
	Custom_OLED_Clear();
}

void curverate_change() {
	uint8_t sw = 0;
	while ((sw = Custom_Switch_Read()) != CUSTOM_SW_BOTH) {
		if (sw == CUSTOM_SW_1) {
			curve_rate -= 0.000001;
		} else if (sw == CUSTOM_SW_2) {
			curve_rate += 0.000001;
		}

		Custom_OLED_Printf("th /1 %d", sensorThreshold);
	}
	Custom_OLED_Clear();
}

void accel_change() {
	uint8_t sw = 0;
	while ((sw = Custom_Switch_Read()) != CUSTOM_SW_BOTH) {

		if (sw == CUSTOM_SW_1) {
			accel_setting -= 0.01f;
		} else if (sw == CUSTOM_SW_2) {
			accel_setting += 0.01f;
		}
		Custom_OLED_Printf("accel/1 %f", accel_setting);
	}
	Custom_OLED_Clear();
}
void decel_change() {
	uint8_t sw = 0;
	while ((sw = Custom_Switch_Read()) != CUSTOM_SW_BOTH) {

		if (sw == CUSTOM_SW_1) {
			decel_Setting -= 0.5f;
		} else if (sw == CUSTOM_SW_2) {
			decel_Setting += 0.5f;
		}
		Custom_OLED_Printf("deccel/1 %f", decel_Setting);
	}
	Custom_OLED_Clear();
}

void curve_decel_change() {
	uint8_t sw = 0;
	while ((sw = Custom_Switch_Read()) != CUSTOM_SW_BOTH) {

		if (sw == CUSTOM_SW_1) {
			curve_decel -= 500;
		} else if (sw == CUSTOM_SW_2) {
			curve_decel += 500;
		}
		Custom_OLED_Printf("curve decel/1 %f", curve_decel);
	}
	Custom_OLED_Clear();
}

void target_v_change() {
	uint8_t sw = 0;
	while ((sw = Custom_Switch_Read()) != CUSTOM_SW_BOTH) {

		if (sw == CUSTOM_SW_1) {
			target_velocity_setting -= 0.01f;
		} else if (sw == CUSTOM_SW_2) {
			target_velocity_setting += 0.01f;
		}
		Custom_OLED_Printf("target v/1 %f", target_velocity_setting);
	}
	Custom_OLED_Clear();
}

void pit_in_change() {
	uint8_t sw = 0;
	while ((sw = Custom_Switch_Read()) != CUSTOM_SW_BOTH) {
		if (sw == CUSTOM_SW_1) {
			pit_in_line -= 0.01;
		} else if (sw == CUSTOM_SW_2) {
			pit_in_line += 0.01;
		}

		Custom_OLED_Printf("pit in line/1 %f", pit_in_line);
	}

	Custom_OLED_Clear();

}
void back_to_menu() {
	uint8_t sw = 0;
	while ((sw = Custom_Switch_Read()) != CUSTOM_SW_BOTH) {
		Custom_OLED_Printf("yes");
	}
}

typedef struct {
	char name[30];
	void (*func)();
} In_Setting;

In_Setting in_setting[] = { { "/g threshold", threshold_change }, { "/g pit in",
		pit_in_change },
//	{ "/g save tick", save_tick_change },
		{ "/g accel", accel_change }, { "/g decel", decel_change },
//		{ "/g pick velocity", pick_velocity_change },
		{ "/g target velocity", target_v_change }, { "/g curve decel",
				curve_decel_change }, { "/g curve rate", curverate_change }, {
				"menu?", back_to_menu }

};

void settings() {
	uint8_t sw = 0;
	int8_t count_in_setting = 0;
	int num_of_setting = sizeof(in_setting) / sizeof(In_Setting);

	Custom_OLED_Clear();
	for (;;) {

		while (CUSTOM_SW_BOTH != (sw = Custom_Switch_Read())) {
			if (sw == CUSTOM_SW_1) {
				count_in_setting--;
				Custom_OLED_Clear();
			} else if (sw == CUSTOM_SW_2) {
				count_in_setting++;
				Custom_OLED_Clear();
			}
			count_in_setting = (count_in_setting + num_of_setting)
					% num_of_setting;
			Custom_OLED_Printf(" %s", in_setting[count_in_setting].name);
		}
		Custom_OLED_Clear();

		in_setting[count_in_setting].func();
		if (count_in_setting == (num_of_setting - 1))
			break;

	}
}
/*typedef struct {
 uint8_t num;
 int32_t adcCoeffRange[SENSOR_NUM];
 int32_t adcCoeffBias[SENSOR_NUM];
 int32_t sensorThreshold;
 } SensorSetting_t;*/

typedef struct {
	char name[30];
	void (*func)();
} In_Menu;

uint8_t count = 0;

#define FLASH_ADDR  0x08040000

//===

// MAX_FLASH_SIZE_BYTES must be multiple of 16
#define MAX_FLASH_SIZE_BYTES 128

typedef struct {
	uint8_t whitemax[16];
	uint8_t blackmax[16];
} SaveData_t;

typedef union {
	SaveData_t saveData;
	uint8_t array[MAX_FLASH_SIZE_BYTES];
} SaveData_u;

void Cali_Flash_save() {
	uint8_t sw = 0;

	while (CUSTOM_SW_BOTH != (sw = Custom_Switch_Read())) {
		Custom_OLED_Printf(
				"wanna save flash/2no:l yes:R /3/g%02x/w%02x/g%02x/w%02x/g%02x/w%02x/g%02x/w%02x/4/g%02x/w%02x/g%02x/w%02x/g%02x/w%02x/g%02x/w%02x /5/g%02x/w%02x/g%02x/w%02x/g%02x/w%02x/g%02x/w%02x/6/g%02x/w%02x/g%02x/w%02x/g%02x/w%02x/g%02x/w%02x",
				whiteMax[0], whiteMax[1], whiteMax[2], whiteMax[3], whiteMax[4],
				whiteMax[5], whiteMax[6], whiteMax[7], whiteMax[8], whiteMax[9],
				whiteMax[10], whiteMax[11], whiteMax[12], whiteMax[13],
				whiteMax[14], whiteMax[15], blackMax[0], blackMax[1],
				blackMax[2], blackMax[3], blackMax[4], blackMax[5], blackMax[6],
				blackMax[7], blackMax[8], blackMax[9], blackMax[10],
				blackMax[11], blackMax[12], blackMax[13], blackMax[14],
				blackMax[15], blackMax[16]);
		if (sw == CUSTOM_SW_1) {
			break;
		}
		if (sw == CUSTOM_SW_2) {
			Custom_OLED_Clear();
			HAL_FLASH_Unlock();

			FLASH_EraseInitTypeDef erase;
			erase.Banks = FLASH_BANK_2;
			erase.TypeErase = FLASH_TYPEERASE_MASSERASE;

			uint32_t err;
			if (HAL_FLASHEx_Erase(&erase, &err) != HAL_OK) {
				Custom_OLED_Printf("Write Fail");
				while (1)
					;
			}
			SaveData_u myUnion = { 0 };
			SaveData_t *myData = &myUnion.saveData;

			for (int i = 0; i < 16; i++) {
				myData->whitemax[i] = whiteMax[i];
				myData->blackmax[i] = blackMax[i];
			}

			for (int i = 0; i < MAX_FLASH_SIZE_BYTES; i += 16) {
				// 이 함수는 세 번째 인자가 16byte 길이의 배열이라고 가정한다.
				// 그래서 그 배열을 두 번째 인자로 주어진 주소에 쓴다.
				HAL_FLASH_Program(
				FLASH_TYPEPROGRAM_QUADWORD, //STM32에서 word size=32bit. 그러므로 quadword = 128bit = 16bytes
						FLASH_ADDR + i, // 데이터를 쓸 위치
						(uint32_t) (&(myUnion.array[i])) // 16bytes 배열의 첫번째 원소를 가리키는 포인터(를 uint32_t로 캐스팅한 것)
						);
			}
		}
	}

}

void Use_Saved_cali() {
	uint8_t sw = 0;
	while (CUSTOM_SW_BOTH != (sw = Custom_Switch_Read())) {

		Custom_OLED_Printf(
				"load cali/2no:l yes:R /3/g%02x/w%02x/g%02x/w%02x/g%02x/w%02x/g%02x/w%02x/4/g%02x/w%02x/g%02x/w%02x/g%02x/w%02x/g%02x/w%02x /5/g%02x/w%02x/g%02x/w%02x/g%02x/w%02x/g%02x/w%02x/6/g%02x/w%02x/g%02x/w%02x/g%02x/w%02x/g%02x/w%02x",
				*((uint8_t*) FLASH_ADDR), *((uint8_t*) FLASH_ADDR + 1),
				*((uint8_t*) FLASH_ADDR + 2), *((uint8_t*) FLASH_ADDR + 3),
				*((uint8_t*) FLASH_ADDR + 4), *((uint8_t*) FLASH_ADDR + 5),
				*((uint8_t*) FLASH_ADDR + 6), *((uint8_t*) FLASH_ADDR + 7),
				*((uint8_t*) FLASH_ADDR + 8), *((uint8_t*) FLASH_ADDR + 9),
				*((uint8_t*) FLASH_ADDR + 10), *((uint8_t*) FLASH_ADDR + 11),
				*((uint8_t*) FLASH_ADDR + 12), *((uint8_t*) FLASH_ADDR + 13),
				*((uint8_t*) FLASH_ADDR + 14), *((uint8_t*) FLASH_ADDR + 15),
				*((uint8_t*) FLASH_ADDR + 16), *((uint8_t*) FLASH_ADDR + 17),
				*((uint8_t*) FLASH_ADDR + 18), *((uint8_t*) FLASH_ADDR + 19),
				*((uint8_t*) FLASH_ADDR + 20), *((uint8_t*) FLASH_ADDR + 21),
				*((uint8_t*) FLASH_ADDR + 22), *((uint8_t*) FLASH_ADDR + 23),
				*((uint8_t*) FLASH_ADDR + 24), *((uint8_t*) FLASH_ADDR + 25),
				*((uint8_t*) FLASH_ADDR + 26), *((uint8_t*) FLASH_ADDR + 27),
				*((uint8_t*) FLASH_ADDR + 28), *((uint8_t*) FLASH_ADDR + 29),
				*((uint8_t*) FLASH_ADDR + 30), *((uint8_t*) FLASH_ADDR + 31));
		if (sw == CUSTOM_SW_1) {
			break;
		}
		if (sw == CUSTOM_SW_2) {
			Custom_OLED_Clear();
			for (int i = 0; i < 16; i++) {
				whiteMax[i] = *(uint8_t*) (FLASH_ADDR + i);
			}
			for (int i = 0; i < 16; i++) {
				blackMax[i] = *(uint8_t*) (FLASH_ADDR + i + 16);
			}

		}
	}

}
//===

static void Flash_Save() {
	uint32_t data[4] = { 0 };

	// Select number
	uint8_t sw;
	Custom_OLED_Printf("0");
	while (CUSTOM_SW_BOTH != (sw = Custom_Switch_Read())) {
		if (sw == CUSTOM_SW_1) {
			data[0]++;
			Custom_OLED_Clear();
			Custom_OLED_Printf("%02x", data[0]);
		}
		if (sw == CUSTOM_SW_2) {
			data[0]--;
			Custom_OLED_Clear();
			Custom_OLED_Printf("%02x", data[0]);
		}
	}

	// Save the selected number
	HAL_FLASH_Unlock();

	FLASH_EraseInitTypeDef erase;
	erase.Banks = FLASH_BANK_2;
	erase.TypeErase = FLASH_TYPEERASE_MASSERASE;

	uint32_t err;
	if (HAL_FLASHEx_Erase(&erase, &err) != HAL_OK) {
		Custom_OLED_Printf("Write Fail");
		while (1)
			;
	}

	HAL_StatusTypeDef ret = HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD,
	FLASH_ADDR, (uint32_t) (data));
	HAL_FLASH_Lock();

	Custom_OLED_Clear();
	if (ret == HAL_OK) {
		Custom_OLED_Printf("Write OK");
	} else {
		Custom_OLED_Printf("Write Fail");
	}
	while (!Custom_Switch_Read())
		;
}

static void Flash_Load() {
	Custom_OLED_Printf("READ: %08x", (*((uint32_t*) FLASH_ADDR)));
	while (!Custom_Switch_Read())
		;
}

void check_delay() {
	Custom_OLED_Printf("a");
	Custom_Delay_ms(1000);
	Custom_OLED_Clear();
}

void target_v_0_5() {
	uint8_t sw = 0;
	while ((sw = Custom_Switch_Read()) != CUSTOM_SW_BOTH) {

		if (sw == CUSTOM_SW_2) {
			target_velocity_setting = 0.5f;
		} else if (sw == CUSTOM_SW_1) {
			break;
		}
		Custom_OLED_Printf("target v/1 %f /2to 0.5 yes",
				target_velocity_setting);
	}
	Custom_OLED_Clear();
}

void target_v_1() {
	uint8_t sw = 0;
	while ((sw = Custom_Switch_Read()) != CUSTOM_SW_BOTH) {

		if (sw == CUSTOM_SW_2) {
			target_velocity_setting = 1.0f;
		} else if (sw == CUSTOM_SW_1) {
			break;
		}
		Custom_OLED_Printf("target v/1 %f /2to 1.0 yes",
				target_velocity_setting);
	}
	Custom_OLED_Clear();
}

In_Menu in_menu[] = { //
				//
				{ "/g calibration", Sensor_Calibration }, //
				{ "/g Cali Flash save", Cali_Flash_save }, //
				{ "/g load Saved cali", Use_Saved_cali }, //
				{ "/b tv 0.5", target_v_0_5 }, //
				{ "/g calibration", Sensor_Calibration }, //
				{ "/m first drive", Drive_First }, //

				{ "/l tv 1.0", target_v_1 },
				{ "/y tv setting", target_v_change }, //
				{ "Flash save", Flash_Save },		//
				{ "Flash load", Flash_Load },		//
				{ "velocity test", velocity_test }, //
				{ "v change", Motor_velocity_change }, //
				{ "motor test", Motor_Test_Phase }, //
				{ "sensor Raw", Sensor_Test_Raw }, //
				//

				{ "window test", test_window }, //
				{ "mark check", mark_check }, //
				{ "state debug", state_debug }, //
				{ "sensor state test", sensor_state_test }, //
				{ "position test", window_position_test }, //
				{ "/wcheck delay", check_delay }, //
				{ "settings", settings } };

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
	uint8_t sw = 0;
	int8_t counting = 0;

	int num_of_menu = sizeof(in_menu) / sizeof(In_Menu);
	//위의 In menu 구조체의 연속을 배열로 표현한 것임 배열1칸(?)이 구조체 하나와 같은거
	Custom_OLED_Clear();
	for (;;) {

		while (CUSTOM_SW_BOTH != (sw = Custom_Switch_Read())) {
			if (sw == CUSTOM_SW_1) {
				counting--;
				Custom_OLED_Clear();
			} else if (sw == CUSTOM_SW_2) {
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
