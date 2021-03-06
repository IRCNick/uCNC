/*
	Name: boarddefs.h
	Description: Defines the available board types.

	Copyright: Copyright (c) João Martins
	Author: João Martins
	Date: 07/02/2020

	µCNC is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version. Please see <http://www.gnu.org/licenses/>

	µCNC is distributed WITHOUT ANY WARRANTY;
	Also without the implied warranty of	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
	See the	GNU General Public License for more details.
*/

#ifndef BOARDDEFS_H
#define BOARDDEFS_H

#include "mcus.h"
#include "boards.h"

/*
	MCU port map
*/
#if (BOARD != 0)
#else
#error Invalid board configuration
#endif

#if (BOARD == BOARD_GRBL)
#define MCU MCU_AVR
#include "mcus/avr/boardmap_grbl.h"
#endif

#if (BOARD == BOARD_RAMBO14)
#define MCU MCU_AVR
#include "mcus/avr/boardmap_rambo14.h"
#endif

#if (BOARD == BOARD_RAMPS14)
#define MCU MCU_AVR
#include "mcus/avr/boardmap_ramps14.h"
#endif

#if (BOARD == BOARD_BLUEPILL)
#define MCU MCU_STM32F10X
#include "mcus/stm32f10x/boardmap_bluepill.h"
#ifndef COM_PORT //enable sync send (used for USB VCP)
#define ENABLE_SYNC_TX
#endif
#endif

#if (BOARD == BOARD_VIRTUAL)
#define MCU MCU_VIRTUAL
#endif

#ifndef BOARD
#error Undefined board
#endif

#include "mcudefs.h" //configures the mcu based on the board pin configuration

#endif
