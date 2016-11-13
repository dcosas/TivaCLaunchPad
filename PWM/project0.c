#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_i2c.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/i2c.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "utils/uartstdio.c"
#include "driverlib/pwm.h"
#include "driverlib/systick.h"


#define NUM_I2C_DATA 3
#define SLAVE_ADDRESS 0x3C

typedef enum
{
	RED,
	GREEN,
	BLUE
}LED_RGB;

typedef enum
{
	UP,
	DOWN
}RAMP_PWM;

void
InitConsole(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioConfig(0, 115200, 16000000);
}

void setPwmLed(LED_RGB ledRgb, uint16_t dutyCycle)
{
	switch(ledRgb)
	{
	case RED:
		PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, dutyCycle);
		break;
	case GREEN:
		PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, dutyCycle);
		break;
	case BLUE:
		PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, dutyCycle);
		break;
	default:
		break;
	}
	//UARTprintf("%d",ledRgb);
	//UARTprintf("\t%d\n",dutyCycle);
}

void rampPwm(LED_RGB ledRgb, RAMP_PWM ramp)
{
	uint16_t dutyCycle = 0;
	const uint16_t maxDutyCycle = 60000;
	const uint16_t delayRamp = 150;
	if(ramp == UP)
	{
		for(dutyCycle = 0; dutyCycle<=maxDutyCycle; dutyCycle++)
		{
			setPwmLed(ledRgb, dutyCycle);
			SysCtlDelay(delayRamp);
		}
	}
	else
	{
		for(dutyCycle = maxDutyCycle+1; dutyCycle>0; dutyCycle--)
		{
			setPwmLed(ledRgb, dutyCycle-1);
			SysCtlDelay(delayRamp);
		}
	}
}

void cycleRGB()
{
	uint8_t cycleRGBState = 0;
	for(cycleRGBState = 0;cycleRGBState<8;cycleRGBState++)
	{
		switch(cycleRGBState)
		{
		case 0:
			rampPwm(RED, UP);
			break;
		case 1://Ramp up Blue
			rampPwm(BLUE, UP);
			break;
		case 2://Ramp down Red
			rampPwm(RED, DOWN);
			break;
		case 3://Ramp up Green
			rampPwm(GREEN, UP);
			break;
		case 4://Ramp down Blue
			rampPwm(BLUE, DOWN);
			break;
		case 5://
			rampPwm(RED, UP);
			break;
		case 6:
			rampPwm(GREEN, DOWN);
			break;
		case 7:
			rampPwm(RED, DOWN);
			break;
		default:
			break;
		}
	}
}

void cycleSingleLeds()
{
	uint8_t cycleRGBState = 0;
		for(cycleRGBState = 0;cycleRGBState<6;cycleRGBState++)
		{
			switch(cycleRGBState)
			{
			case 0:
				rampPwm(RED, UP);
				break;
			case 1:
				rampPwm(RED, DOWN);
				break;
			case 2:
				rampPwm(GREEN, UP);
				break;
			case 3:
				rampPwm(GREEN, DOWN);
				break;
			case 4:
				rampPwm(BLUE, UP);
				break;
			case 5://
				rampPwm(BLUE, DOWN);
				break;
			default:
				break;
			}
		}

}

int
main(void)
{
	uint8_t uartChar;
	uint16_t counter = 0;

    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_PLL | SYSCTL_OSC_INT |
                   SYSCTL_XTAL_16MHZ);


    InitConsole();
    SysTickPeriodSet(SysCtlClockGet()/100);//10ms tick for sd card
    //SysTickIntEnable();
    SysTickEnable();
    //
    // The PWM peripheral must be enabled for use.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);

    //
    // For this example PWM0 is used with PortB Pin6.  The actual port and
    // pins used may be different on your part, consult the data sheet for
    // more information.
    // GPIO port B needs to be enabled so these pins can be used.
    // TODO: change this to whichever GPIO port you are using.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    //
    // Configure the GPIO pin muxing to select PWM00 functions for these pins.
    // This step selects which alternate function is available for these pins.
    // This is necessary if your part supports GPIO pin function muxing.
    // Consult the data sheet to see which functions are allocated per pin.
    // TODO: change this to select the port/pin you are using.
    //
    GPIOPinConfigure(GPIO_PB6_M0PWM0);
    GPIOPinConfigure(GPIO_PB7_M0PWM1);
    GPIOPinConfigure(GPIO_PB4_M0PWM2);
    //
    // Configure the PWM function for this pin.
    // Consult the data sheet to see which functions are allocated per pin.
    // TODO: change this to select the port/pin you are using.
    //
    GPIOPinTypePWM(GPIO_PORTB_BASE, GPIO_PIN_6);
    GPIOPinTypePWM(GPIO_PORTB_BASE, GPIO_PIN_7);
    GPIOPinTypePWM(GPIO_PORTB_BASE, GPIO_PIN_4);

    //
    // Configure the PWM0 to count up/down without synchronization.
    //
    PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_UP_DOWN |
                    PWM_GEN_MODE_NO_SYNC);
    PWMGenConfigure(PWM0_BASE, PWM_GEN_1, PWM_GEN_MODE_UP_DOWN |
                    PWM_GEN_MODE_NO_SYNC);

    //
    // Set the PWM period to 250Hz.  To calculate the appropriate parameter
    // use the following equation: N = (1 / f) * SysClk.  Where N is the
    // function parameter, f is the desired frequency, and SysClk is the
    // system clock frequency.
    // In this case you get: (1 / 250Hz) * 16MHz = 64000 cycles.  Note that
    // the maximum period you can set is 2^16.
    // TODO: modify this calculation to use the clock frequency that you are
    // using.
    //
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, 64000);
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, 64000);

    //
    // Set PWM0 to a duty cycle of 25%.  You set the duty cycle as a function
    // of the period.  Since the period was set above, you can use the
    // PWMGenPeriodGet() function.  For this example the PWM will be high for
    // 25% of the time or 16000 clock ticks (64000 / 4).
    //
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, 0);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, 0);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, 0);
    //
    // Enable the PWM0 Bit0 (PD0) output signal.
    //
    PWMOutputState(PWM0_BASE, PWM_OUT_0_BIT|PWM_OUT_1_BIT|PWM_OUT_2_BIT, true);

    //
    // Enable the PWM generator block.
    //
    PWMGenEnable(PWM0_BASE, PWM_GEN_0);
    PWMGenEnable(PWM0_BASE, PWM_GEN_1);

    //
    // Loop forever while the PWM signals are generated.
    //

    while(1)
	{
    	cycleRGB();
    	cycleSingleLeds();
    	/*
    	//RED
    	uartChar = UARTCharGet(UART0_BASE);
		if(uartChar >= '0' && uartChar <= 'z')
		{
			PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, (uartChar-48)*885);
		}
		//GREEN
		uartChar = UARTCharGet(UART0_BASE);
		if(uartChar >= '0' && uartChar <= 'z')
		{
			PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, (uartChar-48)*885);
		}
		//BLUE
		uartChar = UARTCharGet(UART0_BASE);
		if(uartChar >= '0' && uartChar <= 'z')
		{
			PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, (uartChar-48)*885);
		}
    	//PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, counter++);
    	//PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, counter++);
    	//PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, counter++);
    	SysCtlDelay(1000);*/

	}

    return(0);
}
