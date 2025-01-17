/*
 ******************************************************************************
 * file			custom_switch.h
 * author		Joonho Gwon
 * brief		Switch input
 ******************************************************************************
 */

#ifndef INC_CUSTOM_SWITCH_H_
#define INC_CUSTOM_SWITCH_H_

#define CUSTOM_SW_NONE	0x00
#define CUSTOM_SW_1		0x01
#define CUSTOM_SW_2		0x02
#define CUSTOM_SW_BOTH	0x03

#include "stm32h5xx_ll_gpio.h"
#include "custom_delay.h"
#include <stdint.h>
#include <stdbool.h>

uint8_t Custom_Switch_Read(void);

/*
 ******************************************************************************
 * User Settings
 *
 * SW1 : Left switch
 * SW2 : Right swicth
 ******************************************************************************
 */

#define SW1_PORT	GPIOB
#define SW1_PIN		LL_GPIO_PIN_4
#define SW2_PORT	GPIOB
#define SW2_PIN		LL_GPIO_PIN_5

#endif /* INC_CUSTOM_SWITCH_H_ */
