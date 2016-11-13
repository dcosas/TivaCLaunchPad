#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include "inc/hw_ssi.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "driverlib/ssi.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/udma.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "utils/uartstdio.c"

uint32_t rgbColours[8] = {0xff0000, 0x00ff00, 0x0000ff,0xffff00, 0x00ffff, 0xff00ff, 0xff7700, 0xd500ff};

volatile uint8_t gColour;
uint32_t ssiData=0xaaaaaaaa;

#pragma DATA_ALIGN(pui8ControlTable, 1024)
uint8_t pui8ControlTable[1024];

#define SSI_BUFFER_SIZE         2

//*****************************************************************************
//
// The size of the UART transmit and receive buffers.  They do not need to be
// the same size.
//
//*****************************************************************************

static uint8_t g_ui8SSITxBuf[SSI_BUFFER_SIZE];
static uint8_t g_ui8SSIRxBuf[SSI_BUFFER_SIZE];
static uint32_t g_ui32uDMAErrCount = 0;

static uint32_t g_ui32SSIRxCount = 0;
static uint32_t g_ui32SSITxCount = 0;

void
uDMAErrorHandler(void)
{
    uint32_t ui32Status;
    ui32Status = ROM_uDMAErrorStatusGet();
    if(ui32Status)
    {
        ROM_uDMAErrorStatusClear();
        g_ui32uDMAErrCount++;
    }
}

void
InitSPITransfer(void)
{

    uint_fast16_t ui16Idx;

    //
    // Fill the TX buffer with a simple data pattern.
    //
    for(ui16Idx = 0; ui16Idx < SSI_BUFFER_SIZE; ui16Idx++)
    {
    	g_ui8SSITxBuf[ui16Idx] = 0xaa;
        /*if(ui16Idx%2)
        	g_ui8SSITxBuf[ui16Idx] = 0b11110000;
        else
        	g_ui8SSITxBuf[ui16Idx] = 0b11000000;*/
    }

    //
    // Enable the uDMA interface for both TX and RX channels.
    //
    //ROM_SSIDMAEnable(SSI0_BASE, SSI_DMA_RX | SSI_DMA_TX);
    ROM_SSIDMAEnable(SSI0_BASE, SSI_DMA_TX);

    //****************************************************************************
    //uDMA SSI0 RX
    //****************************************************************************

    //
    // Put the attributes in a known state for the uDMA SSI0RX channel.  These
    // should already be disabled by default.
    //
    ROM_uDMAChannelAttributeDisable(UDMA_CHANNEL_SSI0RX,
                                    UDMA_ATTR_USEBURST | UDMA_ATTR_ALTSELECT |
                                    (UDMA_ATTR_HIGH_PRIORITY |
                                    UDMA_ATTR_REQMASK));

    //
    // Configure the control parameters for the primary control structure for
    // the SSIORX channel.
    //
//    ROM_uDMAChannelControlSet(UDMA_CHANNEL_SSI0RX | UDMA_PRI_SELECT,
//                              UDMA_SIZE_8 | UDMA_SRC_INC_NONE | UDMA_DST_INC_8 |
//                              UDMA_ARB_4);
    //
    // Set up the transfer parameters for the SSI0RX Channel
    //
//    ROM_uDMAChannelTransferSet(UDMA_CHANNEL_SSI0RX | UDMA_PRI_SELECT,
//                               UDMA_MODE_BASIC,
//                               (void *)(SSI0_BASE + SSI_O_DR),
//                               g_ui8SSIRxBuf, sizeof(g_ui8SSIRxBuf));


    //****************************************************************************
    //uDMA SSI0 TX
    //****************************************************************************

    //
    // Put the attributes in a known state for the uDMA SSI0TX channel.  These
    // should already be disabled by default.
    //
    ROM_uDMAChannelAttributeDisable(UDMA_CHANNEL_SSI0TX,
                                    UDMA_ATTR_ALTSELECT |
                                    UDMA_ATTR_HIGH_PRIORITY |
                                    UDMA_ATTR_REQMASK);

    //
    // Set the USEBURST attribute for the uDMA SSI0TX channel.  This will
    // force the controller to always use a burst when transferring data from
    // the TX buffer to the SSI0.  This is somewhat more effecient bus usage
    // than the default which allows single or burst transfers.
    //
    ROM_uDMAChannelAttributeEnable(UDMA_CHANNEL_SSI0TX, UDMA_ATTR_USEBURST);

    //
    // Configure the control parameters for the SSI0 TX.
    //
    ROM_uDMAChannelControlSet(UDMA_CHANNEL_SSI0TX | UDMA_PRI_SELECT,
    		UDMA_SIZE_8 | UDMA_SRC_INC_8 | UDMA_DST_INC_NONE |
							  UDMA_ARB_1024);


    //
    // Set up the transfer parameters for the uDMA SSI0 TX channel.  This will
    // configure the transfer source and destination and the transfer size.
    // Basic mode is used because the peripheral is making the uDMA transfer
    // request.  The source is the TX buffer and the destination is theUART0
    // data register.
    //
    ROM_uDMAChannelTransferSet(UDMA_CHANNEL_SSI0TX | UDMA_PRI_SELECT,
                                                   UDMA_MODE_BASIC, g_ui8SSITxBuf,
                                                   (void *)(SSI0_BASE + SSI_O_DR),
                                                   sizeof(g_ui8SSITxBuf));

    //
    // Now both the uDMA SSI0 TX and RX channels are primed to start a
    // transfer.  As soon as the channels are enabled, the peripheral will
    // issue a transfer request and the data transfers will begin.
    //
    //ROM_uDMAChannelEnable(UDMA_CHANNEL_SSI0RX);
    ROM_uDMAChannelEnable(UDMA_CHANNEL_SSI0TX);

    //
    // Enable the SSI0 DMA TX/RX interrupts.
    //
    //ROM_SSIIntEnable(SSI0_BASE, SSI_DMATX );//| SSI_DMARX);

    //
    // Enable the SSI0 peripheral interrupts.
    //
   // ROM_IntEnable(INT_SSI0);

}
void SSI_init()
{
	uint32_t pui32DataRx[3];
	SysCtlPeripheralReset(SYSCTL_PERIPH_SSI0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);
	SSIDisable(SSI0_BASE);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	GPIOPinConfigure(GPIO_PA2_SSI0CLK);
	GPIOPinConfigure(GPIO_PA3_SSI0FSS);
	GPIOPinConfigure(GPIO_PA4_SSI0RX);
	GPIOPinConfigure(GPIO_PA5_SSI0TX);

	GPIOPinTypeSSI(GPIO_PORTA_BASE,GPIO_PIN_5|GPIO_PIN_4|GPIO_PIN_3|GPIO_PIN_2);
	SSIClockSourceSet(SSI0_BASE, SSI_CLOCK_SYSTEM);
	//SSIConfigSetExpClk(SSI0_BASE, SysCtlClockGet(), SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, 4000000, 8);
	SSIConfigSetExpClk(SSI0_BASE, SysCtlClockGet(), SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, 6600000, 8);
	SSIEnable(SSI0_BASE);
	while(SSIDataGetNonBlocking(SSI0_BASE, &pui32DataRx[0]))
		    {
		    }

	//SSIIntEnable(SSI0_BASE,SSI_TXFF);//	| SSI_RXFF);


	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
	ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_UDMA);
    ROM_IntEnable(INT_UDMAERR);
    ROM_uDMAEnable();
    ROM_uDMAControlBaseSet(pui8ControlTable);
	//IntEnable(INT_SSI0);
	IntMasterEnable();

    InitSPITransfer();
}

uint8_t SSI_write(uint32_t data32u)
{
	uint32_t data = 0;
	SSIDataPut(SSI0_BASE, (uint8_t)data32u);
	while(SSIBusy(SSI0_BASE)){}
	SSIDataGet(SSI0_BASE, &data);
	return (uint8_t)data;
}

void putData(uint32_t data)
{
	SSIDataPut(SSI0_BASE, (uint8_t)data);
	while(SSIBusy(SSI0_BASE)){}
}

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

void TIMER_init()
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_0);
	//GPIOPadConfigSet(GPIO_PORTE_BASE,GPIO_PIN_4,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_OD);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
	//ROM_TimerLoadSet(TIMER0_BASE, TIMER_A, ROM_SysCtlClockGet());
	TimerLoadSet(TIMER0_BASE, TIMER_A, 2);
	IntEnable(INT_TIMER0A);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	TimerEnable(TIMER0_BASE, TIMER_A);
}

main(void)
{

	SysCtlClockSet(SYSCTL_SYSDIV_4|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
//	SysCtlClockSet(SYSCTL_SYSDIV_2_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);
    InitConsole();
    //SSI_init();
    TIMER_init();
    while(1)
    {

    }
}

void SSI0IntHandler(void)
{
	uint32_t ui32Status;
//	    uint32_t ui32Mode;

	    ui32Status = ROM_SSIIntStatus(SSI0_BASE, 1);

	    ROM_SSIIntClear(SSI0_BASE, ui32Status);

//	    ui32Mode = ROM_uDMAChannelModeGet(UDMA_CHANNEL_SSI0RX | UDMA_PRI_SELECT);
//
//	    if(ui32Mode == UDMA_MODE_STOP)
//	    {
//	        g_ui32SSIRxCount++;
//
//	        ROM_uDMAChannelTransferSet(UDMA_CHANNEL_SSI0RX | UDMA_PRI_SELECT,
//	                                   UDMA_MODE_BASIC,
//	                                   (void *)(SSI0_BASE + SSI_O_DR),
//	                                   g_ui8SSIRxBuf, sizeof(g_ui8SSIRxBuf));
//
//	        ROM_uDMAChannelEnable(UDMA_CHANNEL_SSI0RX);
//	    }

	    if(!ROM_uDMAChannelIsEnabled(UDMA_CHANNEL_SSI0TX))
	    {
	        g_ui32SSITxCount++;

	        ROM_uDMAChannelTransferSet(UDMA_CHANNEL_SSI0TX | UDMA_PRI_SELECT,
	                                   UDMA_MODE_BASIC, g_ui8SSITxBuf,
	                                   (void *)(SSI0_BASE + SSI_O_DR),
	                                   sizeof(g_ui8SSITxBuf));

	        ROM_uDMAChannelEnable(UDMA_CHANNEL_SSI0TX);
	    }

}

void highByte()
{

	HWREG(GPIO_PORTE_BASE+ (GPIO_O_DATA + (GPIO_PIN_0<<2))) = GPIO_PIN_0;
	HWREG(GPIO_PORTE_BASE+ (GPIO_O_DATA + (GPIO_PIN_0<<2))) = GPIO_PIN_0;
	HWREG(GPIO_PORTE_BASE+ (GPIO_O_DATA + (GPIO_PIN_0<<2))) = GPIO_PIN_0;
	HWREG(GPIO_PORTE_BASE+ (GPIO_O_DATA + (GPIO_PIN_0<<2))) = GPIO_PIN_0;
	HWREG(GPIO_PORTE_BASE+ (GPIO_O_DATA + (GPIO_PIN_0<<2))) = GPIO_PIN_0;
	HWREG(GPIO_PORTE_BASE+ (GPIO_O_DATA + (GPIO_PIN_0<<2))) = GPIO_PIN_0;
	HWREG(GPIO_PORTE_BASE+ (GPIO_O_DATA + (GPIO_PIN_0<<2))) = 0;
	HWREG(GPIO_PORTE_BASE+ (GPIO_O_DATA + (GPIO_PIN_0<<2))) = 0;
	HWREG(GPIO_PORTE_BASE+ (GPIO_O_DATA + (GPIO_PIN_0<<2))) = 0;
	HWREG(GPIO_PORTE_BASE+ (GPIO_O_DATA + (GPIO_PIN_0<<2))) = 0;
	HWREG(GPIO_PORTE_BASE+ (GPIO_O_DATA + (GPIO_PIN_0<<2))) = 0;
}

void lowByte()
{
	HWREG(GPIO_PORTE_BASE+ (GPIO_O_DATA + (GPIO_PIN_0<<2))) = GPIO_PIN_0;
	HWREG(GPIO_PORTE_BASE+ (GPIO_O_DATA + (GPIO_PIN_0<<2))) = GPIO_PIN_0;
	HWREG(GPIO_PORTE_BASE+ (GPIO_O_DATA + (GPIO_PIN_0<<2))) = GPIO_PIN_0;
	HWREG(GPIO_PORTE_BASE+ (GPIO_O_DATA + (GPIO_PIN_0<<2))) = GPIO_PIN_0;
	HWREG(GPIO_PORTE_BASE+ (GPIO_O_DATA + (GPIO_PIN_0<<2))) = 0;
	HWREG(GPIO_PORTE_BASE+ (GPIO_O_DATA + (GPIO_PIN_0<<2))) = 0;
	HWREG(GPIO_PORTE_BASE+ (GPIO_O_DATA + (GPIO_PIN_0<<2))) = 0;
	HWREG(GPIO_PORTE_BASE+ (GPIO_O_DATA + (GPIO_PIN_0<<2))) = 0;
	HWREG(GPIO_PORTE_BASE+ (GPIO_O_DATA + (GPIO_PIN_0<<2))) = 0;
	HWREG(GPIO_PORTE_BASE+ (GPIO_O_DATA + (GPIO_PIN_0<<2))) = 0;
	HWREG(GPIO_PORTE_BASE+ (GPIO_O_DATA + (GPIO_PIN_0<<2))) = 0;
	HWREG(GPIO_PORTE_BASE+ (GPIO_O_DATA + (GPIO_PIN_0<<2))) = 0;
}


void send24bit(uint32_t colour)//the format for colour: GRB !!(NOT RGB!)
{
	uint32_t i=1;
	colour&=0xffffff;
	for(i=1;i<0x1000000;i*=2)
		if(colour&i)
			highByte();
		else
			lowByte();
}

void sendRGB(uint8_t _r, uint8_t _g, uint8_t _b)
{
	uint32_t colour;
	colour = _g << 16 | _r << 8 | _b;
	send24bit(colour);
}

void Timer0IntHandler(void)
{
	static uint32_t timCounter = 0;
    static uint8_t i=0;
    uint8_t j;
    timCounter++;
    if(timCounter>156200)
    {
    	i++;
    	if(i>40)
    		i=1;
    	timCounter = 0;
    	for(j=0;j<i;j++)
    	{
    		//send24bit(0x00ff00);
    		gColour++;
    		send24bit(rgbColours[0]);
    	}

    }
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);



}
