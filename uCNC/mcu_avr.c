/*
	Name: mcu_avr.c
	Description: Implements mcu interface on AVR.
		Besides all the functions declared in the mcu.h it also implements the code responsible
		for handling:
			interpolator.h
				void interpolator_step_isr();
				void interpolator_step_reset_isr();
			serial.h
				void serial_rx_isr(char c);
				char serial_tx_isr();
			trigger_control.h
				void tc_limits_isr(uint8_t limits);
				void tc_controls_isr(uint8_t controls);
	Copyright: Copyright (c) João Martins 
	Author: João Martins
	Date: 01/11/2019

	uCNC is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version. Please see <http://www.gnu.org/licenses/>

	uCNC is distributed WITHOUT ANY WARRANTY;
	Also without the implied warranty of	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
	See the	GNU General Public License for more details.
*/
#include "config.h"
#include "mcudefs.h"
#include "mcumap.h"
#include "mcu.h"
#include "utils.h"
#include "serial.h"
#include "interpolator.h"
#include "trigger_control.h"

#include <math.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/delay.h>
#include <avr/eeprom.h>


#define PORTMASK (OUTPUT_INVERT_MASK|INPUT_PULLUP_MASK)
#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#ifndef BAUD
#define BAUD 115200
#endif

#ifndef COM_BUFFER_SIZE
#define COM_BUFFER_SIZE 50
#endif

#define PULSE_RESET_DELAY MIN_PULSE_WIDTH_US * F_CPU / 1000000

typedef union{
    uint32_t r; // occupies 4 bytes
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    struct
    {
        uint16_t rl;
        uint16_t rh;
    };
    struct
    {
        uint8_t r0;
        uint8_t r1;
        uint8_t r2;
        uint8_t r3;
    };
#else
    struct
    {
        uint16_t rh;
        uint16_t rl;
    };
    struct
    {
        uint8_t r3;
        uint8_t r2;
        uint8_t r1;
        uint8_t r0;
    }
    ;
#endif
} IO_REGISTER;

//USART communication
volatile bool mcu_tx_ready;
int mcu_putchar(char c, FILE* stream);
FILE g_mcu_streamout = FDEV_SETUP_STREAM(mcu_putchar, NULL, _FDEV_SETUP_WRITE);

static uint8_t mcu_prev_limits;
uint8_t mcu_prev_controls;

#ifdef __PERFSTATS__
volatile uint16_t mcu_perf_step;
volatile uint16_t mcu_perf_step_reset;

uint16_t mcu_get_step_clocks()
{
	uint16_t res = mcu_perf_step;
	return res;
}
uint16_t mcu_get_step_reset_clocks()
{
	uint16_t res = mcu_perf_step_reset;
	return res;
}
#endif

ISR(TIMER1_COMPA_vect, ISR_BLOCK)
{
	static bool busy = false;
	#ifdef __PERFSTATS__
	uint16_t clocks = TCNT1;
	#endif
	if(busy)
	{
		return;
	}
	busy = true;
	interpolator_step_reset_isr();
	
	#ifdef __PERFSTATS__
    uint16_t clocks2 = TCNT1;
    clocks2 -= clocks;
	mcu_perf_step_reset = MAX(mcu_perf_step_reset, clocks2);
	#endif
	busy = false;
}

ISR(TIMER1_COMPB_vect, ISR_BLOCK)
{
	static bool busy = false;
	#ifdef __PERFSTATS__
	uint16_t clocks = TCNT1;
	#endif
	if(busy)
	{
		return;
	}
	busy = true;
    interpolator_step_isr();
    #ifdef __PERFSTATS__
    uint16_t clocks2 = TCNT1;
    clocks2 -= clocks;
	mcu_perf_step = MAX(mcu_perf_step, clocks2);
	#endif
	busy = false;
}

/*
	Fazer modifica��o
	ISR apenas para limites e controls
	criar static e comparar com valor anterior e so disparar callback caso tenha alterado
*/

ISR(PCINT0_vect, ISR_NOBLOCK) // input pin on change service routine
{
	#if(LIMITS_ISR_ID==0)
	static uint8_t prev_limits = 0;
	uint8_t limits = mcu_get_limits();
	if(prev_limits != limits)
	{
		tc_limits_isr(limits);
		prev_limits = limits;
	}
	#endif
	
	#if(CONTROLS_ISR_ID==0)
	static uint8_t prev_controls = 0;
	uint8_t controls = mcu_get_controls();
	if(prev_controls != controls)
	{
		tc_controls_isr(controls);
		prev_controls = controls;
	}
	#endif		
}

ISR(PCINT1_vect, ISR_NOBLOCK) // input pin on change service routine
{
    #if(LIMITS_ISR_ID==1)
	static uint8_t prev_limits = 0;
	uint8_t limits = mcu_get_limits();
	if(prev_limits != limits)
	{
		tc_limits_isr(limits);
		prev_limits = limits;
	}
	#endif

	#if(CONTROLS_ISR_ID==1)
	static uint8_t prev_controls = 0;
	uint8_t controls = mcu_get_controls();
	if(prev_controls != controls)
	{
		tc_controls_isr(controls);
		prev_controls = controls;
	}
	#endif
}

ISR(PCINT2_vect, ISR_NOBLOCK) // input pin on change service routine
{
    #if(LIMITS_ISR_ID==2)
	static uint8_t prev_limits = 0;
	uint8_t limits = mcu_get_limits();
	if(prev_limits != limits)
	{
		tc_limits_isr(limits);
		prev_limits = limits;
	}
	#endif

	#if(CONTROLS_ISR_ID==2)
	static uint8_t prev_controls = 0;
	uint8_t controls = mcu_get_controls();
	if(prev_controls != controls)
	{
		tc_controls_isr(controls);
		prev_controls = controls;
	}
	#endif
}

ISR(PCINT3_vect, ISR_NOBLOCK) // input pin on change service routine
{
    #if(LIMITS_ISR_ID==3)
	static uint8_t prev_limits = 0;
	uint8_t limits = mcu_get_limits();
	if(prev_limits != limits)
	{
		tc_limits_isr(limits);
		prev_limits = limits;
	}
	#endif

	#if(CONTROLS_ISR_ID==3)
	static uint8_t prev_controls = 0;
	uint8_t controls = mcu_get_controls();
	if(prev_controls != controls)
	{
		tc_controls_isr(controls);
		prev_controls = controls;
	}
	#endif
}

ISR(USART_RX_vect, ISR_BLOCK)
{
	uint8_t c = UDR0;
	serial_rx_isr(c);
}

ISR(USART_UDRE_vect, ISR_BLOCK)
{
	if(serial_tx_is_empty())
	{
		UCSR0B &= ~(1<<UDRIE0);
		mcu_tx_ready = true;
		return;
	}
	
	UDR0 = serial_tx_isr();
}

void mcu_init()
{
    IO_REGISTER reg = {};
    
    #ifdef __PERFSTATS__
	mcu_perf_step = 0;
	mcu_perf_step_reset = 0;
	#endif
	
	//disable WDT
	wdt_reset();
    MCUSR &= ~(1<<WDRF);
	WDTCSR |= (1<<WDCE) | (1<<WDE);
	WDTCSR = 0x00;

    //sets all outputs and inputs
    //inputs
    #ifdef CONTROLS_DIRREG
        CONTROLS_DIRREG = 0;
    #endif
    
	#ifdef LIMITS_DIRREG
        LIMITS_DIRREG = 0;
    #endif
    
    #ifdef PROBE_DIRREG
        PROBE_DIRREG = 0;
    #endif
    
	#ifdef COM_DIRREG
        COM_DIRREG = 0;
    #endif
    
    #ifdef DINS_LOW_DIRREG
        DINS_LOW_DIRREG = 0;
    #endif
    
    #ifdef DINS_HIGH_DIRREG
        DINS_HIGH_DIRREG = 0;
    #endif
    
    //pull-ups
    #ifdef CONTROLS_PULLUPREG
        CONTROLS_PULLUPREG |= CONTROLS_PULLUP_MASK;
    #endif
    
	#ifdef LIMITS_PULLUPREG
        LIMITS_PULLUPREG |= LIMITS_PULLUP_MASK;
    #endif
    
    #ifdef PROBE_PULLUPREG
        PROBE_PULLUPREG |= PROBE_PULLUP_MASK;
    #endif

    #ifdef DINS_LOW_PULLUPREG
        DINS_LOW_PULLUPREG |= DINS_LOW_PULLUP_MASK;
    #endif
    
    #ifdef DINS_HIGH_PULLUPREG
        DINS_HIGH_PULLUPREG |= DINS_HIGH_PULLUP_MASK;
    #endif
    
    //outputs
    #ifdef STEPS_DIRREG
        STEPS_DIRREG |= STEPS_MASK;
    #endif
    
	#ifdef DIRS_DIRREG
        DIRS_DIRREG |= DIRS_MASK;
    #endif
    
    #ifdef COM_DIRREG
        COM_DIRREG |= TX_MASK;
    #endif
    
	#ifdef DOUTS_LOW_DIRREG
        DOUTS_LOW_DIRREG |= DOUTS_LOW_MASK;
    #endif
    
    #ifdef DOUTS_HIGH_DIRREG
        DOUTS_HIGH_DIRREG |= DOUTS_HIGH_MASK;
    #endif
    
    //initializes interrupts on all input pins
    //first read direction registers on all ports (inputs are set to 0)
    reg.r = 0;
    #ifdef PORTRD0
        reg.r0 = PORTDIR0;
    #endif
    #ifdef PORTRD1
        reg.r1 = PORTDIR1;
    #endif
    #ifdef PORTRD2
        reg.r2 = PORTDIR2;
    #endif
    #ifdef PORTRD3
        reg.r3 = PORTDIR3;
    #endif
    
    //input interrupts
    
    //activate Pin on change interrupt
    PCICR |= ((1<<LIMITS_ISR_ID) | (1<<CONTROLS_ISR_ID));
    
    #ifdef LIMITS_ISRREG
    	LIMITS_ISRREG |= LIMITS_MASK;
    #endif
    #ifdef CONTROLS_ISRREG
    	CONTROLS_ISRREG |= CONTROLS_MASK;
    #endif

    stdout = &g_mcu_streamout;
    mcu_tx_ready = true;

	//PWM's
	#ifdef PWM0
		PWM0_DIRREG |= PWM0_MASK;
		PWM0_TMRAREG |= (1 | (1<<(6 + PWM0_REGINDEX)));
		PWM0_TMRBREG = 3;
		PWM0_CNTREG = 0;
	#endif
	#ifdef PWM1
		PWM1_DIRREG |= PWM1_MASK;
		PWM1_TMRAREG |= (1 | (1<<(6 + PWM1_REGINDEX)));
		PWM1_TMRBREG = 3;
		PWM1_CNTREG = 1;
	#endif
	#ifdef PWM2
		PWM2_DIRREG |= PWM2_MASK;
		PWM2_TMRAREG |= (1 | (1<<(6 + PWM2_REGINDEX)));
		PWM2_TMRBREG = 3;
		PWM2_CNTREG = 0;
	#endif
	#ifdef PWM3
		PWM3_DIRREG |= PWM3_MASK;
		PWM3_TMRAREG |= (1 | (1<<(6 + PWM3_REGINDEX)));
		PWM3_TMRBREG = 3;
		PWM3_CNTREG = 0;
	#endif

    // Set baud rate
    #if BAUD < 57600
      uint16_t UBRR0_value = ((F_CPU / (8L * BAUD)) - 1)/2 ;
      UCSR0A &= ~(1 << U2X0); // baud doubler off  - Only needed on Uno XXX
    #else
      uint16_t UBRR0_value = ((F_CPU / (4L * BAUD)) - 1)/2;
      UCSR0A |= (1 << U2X0);  // baud doubler on for high baud rates, i.e. 115200
    #endif
    UBRR0H = UBRR0_value >> 8;
    UBRR0L = UBRR0_value;
  
    // enable rx, tx, and interrupt on complete reception of a byte and UDR empty
    UCSR0B |= (1<<RXEN0 | 1<<TXEN0 | 1<<RXCIE0);
    
	//enable interrupts
	sei();
}

//IO functions    
//Inputs  
uint16_t mcu_get_inputs()
{
	IO_REGISTER reg;
	reg.r = 0;
	#ifdef DINS_LOW
	reg.r0 = (DINS_LOW & DINS_LOW_MASK);
	#endif
	#ifdef DINS_HIGH
	reg.r1 = (DINS_HIGH & DINS_HIGH_MASK);
	#endif
	return reg.rl;	
}

uint8_t mcu_get_controls()
{
	return (CONTROLS_INREG & CONTROLS_MASK);
}

uint8_t mcu_get_limits()
{
	return (LIMITS_INREG & LIMITS_MASK);
}

uint8_t mcu_get_probe()
{
	return (LIMITS_INREG & PROBE_MASK);
}

//outputs

//sets all step pins
void mcu_set_steps(uint8_t value)
{
	STEPS_OUTREG = (~STEPS_MASK & STEPS_OUTREG) | value;
}
//sets all dir pins
void mcu_set_dirs(uint8_t value)
{
	DIRS_OUTREG = (~DIRS_MASK & DIRS_OUTREG) | value;
}

void mcu_set_outputs(uint16_t value)
{
	IO_REGISTER reg = {};
	reg.rl = value;
	
	#ifdef DOUTS_LOW_OUTREG
		DOUTS_LOW_OUTREG = (~DOUTS_LOW_MASK & DOUTS_LOW_OUTREG) | reg.r0;
	#endif
	#ifdef DOUTS_HIGH_OUTREG
		DOUTS_HIGH_OUTREG = (~DOUTS_HIGH_MASK & DOUTS_HIGH_OUTREG) | reg.r1;
	#endif
}

void mcu_set_pwm(uint8_t pwm, uint8_t value)
{
	switch(pwm)
	{
		case 0:
			#ifdef PWM0
			PWM0_CNTREG = value;
			return;
			#else
			return;
			#endif
		case 1:
			#ifdef PWM1
			PWM1_CNTREG = value;
			return;
			#else
			return;
			#endif
		case 2:
			#ifdef PWM2
			PWM2_CNTREG = value;
			return;
			#else
			return;
			#endif
		case 3:
			#ifdef PWM3
			PWM3_CNTREG = value;
			return;
			#else
			return;
			#endif
	}
}

void mcu_enable_interrupts()
{
	sei();
}
void mcu_disable_interrupts()
{
	cli();
}

//internal redirect of stdout
int mcu_putchar(char c, FILE* stream)
{
	mcu_putc(c);
	return c;
}

void mcu_start_send()
{
	mcu_tx_ready = false;
	UCSR0B |= (1<<UDRIE0);
}

void mcu_putc(char c)
{
	loop_until_bit_is_set(UCSR0A, UDRE0);
	UDR0 = c;
}

bool mcu_is_tx_ready()
{
	return mcu_tx_ready;
}

char mcu_getc()
{
	loop_until_bit_is_set(UCSR0A, RXC0);
    return UDR0;
}


#ifdef __PROF__
/*ISR(TIMER2_OVF_vect)
{
	g_mcu_perfOvfCounter++;
}

//will count up to 16777215 (24 bits)
uint32_t mcu_getCycles()
{
	uint8_t ticks = (uint8_t)TCNT2;
    uint16_t highticks = (uint16_t)g_mcu_perfOvfCounter;
    uint32_t result = highticks;
	result <<= 8;
	if(ticks != 255)
    	result |= ticks;
    
    return result;
}

uint32_t mcu_getElapsedCycles(uint32_t cycle_ref)
{
	uint32_t result = mcu_getCycles();
	result -= g_mcu_perfCounterOffset;
	if(result < cycle_ref)
		result += 1677216;
	return result - cycle_ref;
}

void mcu_startTickCounter()
{
    TCCR2A = 0;
    TCCR2B = 0;
    TCNT2 = 0;  //initialize counter value to 0
    TIFR2 = 0;
    TIMSK2 |= (1 << TOIE2);
    g_mcu_perfOvfCounter = 0;
    TCCR2B = 1;
}
*/
/*

void mcu_startPerfCounter()
{
    TCCR2A = 0;
    TCCR2B = 0;
    TCNT2 = 0;  //initialize counter value to 0
    TIFR2 = 0;
    TIMSK2 |= (1 << TOIE2);
    g_mcu_perfOvfCounter = 0;
    TCCR2B = 1;
}

uint16_t mcu_stopPerfCounter()
{
    uint8_t ticks = TCNT2;
    TCCR2B = 0;
    TIMSK2 &= ~(1 << TOIE2);
    uint16_t res = g_mcu_perfOvfCounter;
    res *= 256;
    res += ticks;
    return res;
}
*/
#endif

//RealTime
void mcu_freq_to_clocks(float frequency, uint16_t* ticks, uint8_t* prescaller)
{
	if(frequency < F_STEP_MIN)
		frequency = F_STEP_MIN;
	if(frequency > F_STEP_MAX)
		frequency = F_STEP_MAX;
		
	float clockcounter = F_CPU;
		
	if(frequency >= 245)
	{
		*prescaller = 9;
		
	}
	else if(frequency >= 31)
	{
		*prescaller = 10;
		clockcounter *= 0.125f;
	}
	else if(frequency >= 4)
	{
		*prescaller = 11;
		clockcounter *= 0.015625f;
		
	}
	else if(frequency >= 1)
	{
		*prescaller = 12;
		clockcounter *= 0.00390625f;
	}
	else
	{
		*prescaller = 13;
		clockcounter *= 0.0009765625f;
	}

	*ticks = floorf((clockcounter/frequency)) - 1;
}
/*
	initializes the pulse ISR
	In Arduino this is done in TIMER1
	The frequency range is from 4Hz to F_PULSE
*/
void mcu_start_step_ISR(uint16_t clocks_speed, uint8_t prescaller)
{
	//stops timer
	TCCR1B = 0;
	//CTC mode
    TCCR1A = 0;
    //resets counter
    TCNT1 = 0;
    //set step clock
    OCR1A = clocks_speed;
	//sets OCR0B to half
	//this will allways fire step_reset between pulses
    OCR1B = OCR1A>>1;
	TIFR1 = 0;
	// enable timer interrupts on both match registers
    TIMSK1 |= (1 << OCIE1B) | (1 << OCIE1A);
    
    //start timer in CTC mode with the correct prescaler
    TCCR1B = prescaller;
}

// se implementar amass deixo de necessitar de prescaler
void mcu_change_step_ISR(uint16_t clocks_speed, uint8_t prescaller)
{
	//stops timer
	//TCCR1B = 0;
	OCR1B = clocks_speed>>1;
	OCR1A = clocks_speed;
	//sets OCR0B to half
	//this will allways fire step_reset between pulses
    
	//reset timer
    //TCNT1 = 0;
	//start timer in CTC mode with the correct prescaler
    TCCR1B = prescaller;
}

void mcu_step_stop_ISR()
{
	TCCR1B = 0;
    TIMSK1 &= ~((1 << OCIE1B) | (1 << OCIE1A));
}

/*#define MCU_1MS_LOOP F_CPU/1000000
static __attribute__((always_inline)) void mcu_delay_1ms() 
{
	uint16_t loop = MCU_1MS_LOOP;
	do{
	}while(--loop);
}*/

void mcu_delay_ms(uint16_t miliseconds)
{
	do{
		_delay_ms(1);
	}while(--miliseconds);
	
}

#ifndef EEPE
		#define EEPE  EEWE  //!< EEPROM program/write enable.
		#define EEMPE EEMWE //!< EEPROM master program/write enable.
#endif

/* These two are unfortunately not defined in the device include files. */
#define EEPM1 5 //!< EEPROM Programming Mode Bit 1.
#define EEPM0 4 //!< EEPROM Programming Mode Bit 0.

uint8_t mcu_eeprom_getc(uint16_t address)
{
	do {} while( EECR & (1<<EEPE) ); // Wait for completion of previous write.
	EEAR = address; // Set EEPROM address register.
	EECR = (1<<EERE); // Start EEPROM read operation.
	return EEDR; // Return the byte read from EEPROM.
}

//taken from grbl
uint8_t mcu_eeprom_putc(uint16_t address, uint8_t value)
{
	char old_value; // Old EEPROM value.
	char diff_mask; // Difference mask, i.e. old value XOR new value.

	cli(); // Ensure atomic operation for the write operation.
	
	do {} while( EECR & (1<<EEPE) ); // Wait for completion of previous write
	do {} while( SPMCSR & (1<<SELFPRGEN) ); // Wait for completion of SPM.
	
	EEAR = address; // Set EEPROM address register.
	EECR = (1<<EERE); // Start EEPROM read operation.
	old_value = EEDR; // Get old EEPROM value.
	diff_mask = old_value ^ value; // Get bit differences.
	
	// Check if any bits are changed to '1' in the new value.
	if( diff_mask & value ) {
		// Now we know that _some_ bits need to be erased to '1'.
		
		// Check if any bits in the new value are '0'.
		if( value != 0xff ) {
			// Now we know that some bits need to be programmed to '0' also.
			
			EEDR = value; // Set EEPROM data register.
			EECR = (1<<EEMPE) | // Set Master Write Enable bit...
			       (0<<EEPM1) | (0<<EEPM0); // ...and Erase+Write mode.
			EECR |= (1<<EEPE);  // Start Erase+Write operation.
		} else {
			// Now we know that all bits should be erased.

			EECR = (1<<EEMPE) | // Set Master Write Enable bit...
			       (1<<EEPM0);  // ...and Erase-only mode.
			EECR |= (1<<EEPE);  // Start Erase-only operation.
		}
	} else {
		// Now we know that _no_ bits need to be erased to '1'.
		
		// Check if any bits are changed from '1' in the old value.
		if( diff_mask ) {
			// Now we know that _some_ bits need to the programmed to '0'.
			
			EEDR = value;   // Set EEPROM data register.
			EECR = (1<<EEMPE) | // Set Master Write Enable bit...
			       (1<<EEPM1);  // ...and Write-only mode.
			EECR |= (1<<EEPE);  // Start Write-only operation.
		}
	}
	
	sei(); // Restore interrupt flag state.
}
