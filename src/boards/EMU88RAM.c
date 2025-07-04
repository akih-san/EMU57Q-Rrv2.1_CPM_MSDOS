/*
 *  This source is for PIC18F57Q43 UART, I2C, SPI and TIMER0
 *
 * Base source code is maked by @hanyazou
 *  https://twitter.com/hanyazou
 *
 * Redesigned by Akihito Honda(Aki.h @akih_san)
 *  https://twitter.com/akih_san
 *  https://github.com/akih-san
 *
 *  Target: PIC18F57QXX
 *  Date. 2024.4.20
*/

#define BOARD_DEPENDENT_SOURCE

#include "../../src/emu88.h"
#include <stdio.h>
#include "../../drivers/SDCard.h"
#include "../../drivers/picregister.h"

#define SPI_PREFIX      SPI_SD
#define SPI_HW_INST     SPI1
#include "../../drivers/SPI.h"

#define I88_DATA        B
#define I88_ADDR_H      F
#define I88_ADDR_L      D

#define I88_A16		A0
#define I88_A17		A1
#define I88_A18		A2
#define I88_A19		A3

#define I88_IOM		C0
#define I88_ALE		C5

#define I88_RESET	E0
#define I88_HOLD	E1		// BUS request
#define I88_HOLDA	A5

// /WR
#define I88_WR		C1
// /RD
#define I88_RD		C2

// CLK
#define I88_CLK		C7
#define I88_NMI		C6

#define SPI_SS		E2
#define SPI_SD_POCI	B2

#define SPI_SD_PICO     B0
#define SPI_SD_CLK      B1
#define SPI_SD_SS       SPI_SS

#define URT1_RXD		C4
#define URT1_TXD		C3


#define CMD_REQ CLC3OUT

#include "emu88_common.c"

static void reset_ioreq(void);
void (*bus_master_operation)(void);

void sys_init()
{
    emu88_common_sys_init();

	// NMI definition
	WPU(I88_NMI) = 0;     // disable week pull up
	PPS(I88_NMI) = 0;     // set as latch port
	LAT(I88_NMI) = 0;     // NMI=0
	TRIS(I88_NMI) = 0;    // Set as output

	// HOLD output pin
    LAT(I88_HOLD) = 1;
    TRIS(I88_HOLD) = 0;          // Set as output
	
	// IO/#M
	WPU(I88_IOM) = 1;     // I88_IOM Week pull up
	LAT(I88_IOM) = 0;     // memory /CE active
	TRIS(I88_IOM) = 0;    // Set as onput

	// ALE
	WPU(I88_ALE) = 0;     // disable week pull up
    TRIS(I88_ALE) = 1;    // Set as input

	// /WR output pin
	WPU(I88_WR) = 1;		// /WR Week pull up
    LAT(I88_WR) = 1;		// disactive
    TRIS(I88_WR) = 0;		// Set as output
    PPS(I88_WR) = 0x00;	//reset:output PPS is LATCH

	// /RD output pin
	WPU(I88_RD) = 1;		// /WR Week pull up
    LAT(I88_RD) = 1;		// disactive
    TRIS(I88_RD) = 0;		// Set as output
    PPS(I88_RD) = 0x00;	//reset:output PPS is LATCH

	// SPI_SS
	WPU(SPI_SS) = 1;     // SPI_SS Week pull up
	LAT(SPI_SS) = 1;     // set SPI disable
	TRIS(SPI_SS) = 0;    // Set as onput

	LAT(I88_CLK) = 1;	// 8088_CLK = 1
    TRIS(I88_CLK) = 0;	// set as output pin
	
	//RA4 : setup /A19
	WPUA4 = 1;			// Week pull up
	TRISA4 = 0;			// Set as output
	RA4PPS = 0x01;		// CLC1OUT ( not A19 ) -> RA4

// Setup CLC
//
	//========== CLC pin assign ===========
    CLCIN4PPS = 0x15;			// assign RC5(ALE)
    CLCIN5PPS = 0x10;			// assign RC0(IO/M#)
	CLCIN0PPS = 0x03;			// assign RA3(A19)

	//========== CLC1 : make not A19  ==========
	CLCSELECT = 0;		// CLC1 select

	CLCnSEL0 = 0;		// CLCIN0PPS : RA3(A19)
	CLCnSEL1 = 127;		// NC
	CLCnSEL2 = 127;		// NC
	CLCnSEL3 = 127;		// NC

    CLCnGLS0 = 0x01;	// invert A19 -> log1
    CLCnGLS1 = 0x00;	// not gated
    CLCnGLS2 = 0x00;	// not gated
    CLCnGLS3 = 0x00;	// not gated

    CLCnPOL = 0x0e;		// POL=0, (G4POL,G3POL,G2POL)=1, G1POL=1
    CLCnCON = 0x82;		// 4-Input AND , no interrupt occurs

	
	//========== CLC3 : IOREQ ==========
	// reset DFF with software(reset_ioreq();)

	CLCSELECT = 2;		// CLC3 select

	CLCnSEL0 = 4;		// CLCIN4PPS : RC5(ALE)
    CLCnSEL1 = 5;		// CLCIN5PPS : RC0(IO/M#)
	CLCnSEL2 = 0x35;	// CLC3OUT
	CLCnSEL3 = 127;		// NC

    CLCnGLS0 = 0x02;	// ALE -> (log1) DFF(CK)
	CLCnGLS1 = 0x08;	// IO/#M - > (log2) OR gate
    CLCnGLS2 = 0x00;	// not gated
    CLCnGLS3 = 0x20;	// CLC3OUT -> (log4) OR gate

    CLCnPOL = 0x00;		// POL=0
    CLCnCON = 0x85;		// 2-Input DFF with R , no interrupt occurs

	// reset CLC3
	reset_ioreq();

	// SPI data and clock pins slew at maximum rate

	SLRCON(SPI_SD_PICO) = 0;
	SLRCON(SPI_SD_CLK) = 0;
	SLRCON(SPI_SD_POCI) = 0;

/*********** CLOCK TIMING ************************
 I88_CLK TIMING REQUIREMENTS(MUST)
 CLK Low Time  : minimum 118ns
 CLK High Time : minimum 69ns
*************************************************/

/**************** PWM1 ********************
// 8088 TIMING REQUIREMENTS(MUST)
// CLK Low Time  : minimum 118ns
// CLK High Time : minimum 69ns
// CLK duty 33%

// P64 = 1/64MHz = 15.625ns
// P5  = 1/5MHz  = 200ns = P64 * 12.8
// P8  = 1/8MHz  = 125nz = P64 * 8
//
// --- 4.92MHz ---
// Set PWM Left Aligned mode
// PR = 12
// P1 = 5 : P64*5 = 78.125ns
// P2 = 8 : P64*8 = 125ns
// MODE = 0
//     high period time: 78.125ns
//     low period time: 125ns
//     78.125 + 125ns = 203.125ns f = 4923076.9230769230769230769230769 Hz
//     duty = 38.4%
// --- 8MHz ---
// Set PWM Left Aligned mode
// PR = 7
// P1 = 3 : P64*3 = 46.875ns
// P2 = 5 : P64*5 = 78.125ns
// MODE = 0
//     high period time: 46.875ns
//     low period time: 78.125ns
//     46.875ns + 78.125 = 125ns f = 8 Hz
//     duty = 37.5%
******************************************/

	PWM1CLK = 0x02;		// Fsoc
	PWM1GIE = 0x00;		// interrupt disable
#ifdef CLK8M
//8MHz
	PWM1PR = 0x0007;	// 8 periods ( 0 - 7 )
	PWM1S1P1 = 0x0003;	// P1 = 3
	PWM1S1P2 = 0x0005;	// P2 = 5
#else
//5MHz
	PWM1PR = 0x000C;	// 13 periods ( 0 - 12 )
	PWM1S1P1 = 0x0005;	// P1 = 5
	PWM1S1P2 = 0x0008;	// P2 = 8
#endif
	PWM1S1CFG = 0x00;	// (POL1, POL2)= 0, PPEN = 0 MODE = 0 (Left Aligned mode)
	PWM1CON = 0x84;		// EN=1, LD=1
	RC7PPS = 0x18;		// PWM1S1P1_OUT

    emu88_common_wait_for_programmer();

    //
    // Initialize SD Card
    //
    static int retry;
    for (retry = 0; 1; retry++) {
        if (20 <= retry) {
            printf("No SD Card?\n\r");
            while(1);
        }
//        if (SDCard_init(SPI_CLOCK_100KHZ, SPI_CLOCK_2MHZ, /* timeout */ 100) == SDCARD_SUCCESS)
        if (SDCard_init(SPI_CLOCK_100KHZ, SPI_CLOCK_4MHZ, /* timeout */ 100) == SDCARD_SUCCESS)
//        if (SDCard_init(SPI_CLOCK_100KHZ, SPI_CLOCK_8MHZ, /* timeout */ 100) == SDCARD_SUCCESS)
            break;
        __delay_ms(200);
    }

	GIE = 1;             // Global interrupt enable
}

// ************ timer0 setup ******************
void setup_tomer0(void) {

	T0CON0 = 0x90;	// timer enable, 16bit counter mode , 1:1 Postscaler
	T0CON1 = 0x80;	// sorce clk:LFINTOSC, 1:1 Prescaler
	LFOEN = 1;		// LFINTOSC is explicitly enabled

	TMR0L = TIMER0_INITCL;
	TMR0H = TIMER0_INITCH;		//	timer counter set to 0x86e8
								//  LFINTOSC = 31Khz!!
	timer0_init();

	TMR0IF =0; // Clear timer0 interrupt flag
	TMR0IE = 1;	// Enable timer0 interrupt
}

void setup_I2C(void) {
	//Source clock enable
	MFOEN = 1;		// MFINTOSC is explicitly enabled
	
	// I2C PIN definition
	LATC4 = 0;			// Set as output
	LATC3 = 0;			// Set as output
	TRISC4 = 0;			// Set as output
	TRISC3 = 0;			// Set as output
	WPUC4 = 1;			// week pull up
	WPUC3 = 1;			// week pull up
	
	RC4PPS = 0x38;			// set RC4PPS for I2C1 SDA
	I2C1SDAPPS = 0x14;		// set RC4 for I2C SDA port

	RC3PPS = 0x37;			// set RC3PPS for I2C SCL
	I2C1SCLPPS = 0x13;		// set RC3 for I2C_SCL port

	//Open-Drain Control
	ODCONC = 0x18;		// RC4 and RC3 are Open-Drain output

	//set I2C Pad Control Register TH=01 (I2C-specific input thresholds)
	RC4I2C = 0x01;		// Std GPIO Slew Rate, Std GPIO weak pull-up, I2C-specific input thresholds
	RC3I2C = 0x01;		// Std GPIO Slew Rate, Std GPIO weak pull-up, I2C-specific input thresholds
//	RC4I2C = 0x41;		// Fast mode (400 kHz), Std GPIO weak pull-up, I2C-specific input thresholds
//	RC3I2C = 0x41;		// Fast mode (400 kHz), Std GPIO weak pull-up, I2C-specific input thresholds
//	RC4I2C = 0xc1;		// Fast mode Plus (1 MHz), Std GPIO weak pull-up, I2C-specific input thresholds
//	RC3I2C = 0xc1;		// Fast mode Plus (1 MHz), Std GPIO weak pull-up, I2C-specific input thresholds

	I2C1_Init();

}

void setup_uart1(void)
{
// Begin UART1 initialize -----------------------------------------
// RC3 is used as UART1 TXD
// RC4 is used as UART1 RXD

//	U1BRG = 1664;		// 2400bps @ 64MHz
//	U1BRG = 832;		// 4800bps @ 64MHz
	U1BRG = 416;		// 9600bps @ 64MHz
//	U1BRG = 208;		// 19200bps @ 64MHz
//	U1BRG = 104;		// 38400bps @ 64MHz
//	U1BRG = 34;			// 115200bps @ 64MHz

	// Reset I2C parameter
	ODCONC = 0x00;		// RC4 and RC3 areoperates as standard push-pull drive (source and sink current)
	RC4I2C = 0x00;		// Std GPIO Slew Rate, Std GPIO weak pull-up, 
						// Standard GPIO Input pull-up, enabled via the INLVLxy registers
	RC3I2C = 0x00;		// Std GPIO Slew Rate, Std GPIO weak pull-up, 
						// Standard GPIO Input pull-up, enabled via the INLVLxy registers

    // UART1 Receiver
	TRIS(URT1_RXD) = 1;	// RXD set as input
	U1RXPPS = 0x14;		 // RC4->UART1:RXD;

	// UART1 Transmitter
    LAT(URT1_TXD) = 1;		// Default level
    TRIS(URT1_TXD) = 0;		// TXD set as output
    RC3PPS = 0x20;			// UART1:TXD -> RC3;

	U1RXEN = 1;			// Receiver enable
    U1TXEN = 1;			// Transmitter enable

    U1ON = 1;           // Serial port enable

    U1RXIE = 1;          // Receiver interrupt enable

	uart1_init();

	// End UART1 initialize -------------------------------------------
}

void start_i88(void)
{

    emu88_common_start_i88();

	// Unlock IVT
    IVTLOCK = 0x55;
    IVTLOCK = 0xAA;
    IVTLOCKbits.IVTLOCKED = 0x00;

    // Default IVT base address
    IVTBASE = 0x000008;

    // Lock IVT
    IVTLOCK = 0x55;
    IVTLOCK = 0xAA;
    IVTLOCKbits.IVTLOCKED = 0x01;

	TRIS(I88_HOLDA) = 1;    // HOLDA is set as input
	LAT(I88_HOLD) = 0;		// Release HOLD
	// I88 start
    LAT(I88_RESET) = 0;		// Release reset

}

static void reset_ioreq(void)
{
	// Release wait (D-FF reset)
	G3POL = 1;
	G3POL = 0;
}

static void set_hold_pin(void)
{
	LAT(I88_HOLD) = 1;
	while( !R(I88_HOLDA) ) {}		// wait until bus release
}

static void reset_hold_pin(void)
{
	LAT(I88_HOLD) = 0;
	while( R(I88_HOLDA) ) {}		// wait until bus release
}

void nmi_sig_off(void)
{
	LAT(I88_NMI) = 0;
}

static void nmi_sig_on(void)
{
	LAT(I88_NMI) = 1;
}

static void bus_hold_req(void) {
	// Set address bus as output
	TRIS(I88_ADDR_L) = 0x00;	// A7-A0
	TRIS(I88_ADDR_H) = 0x00;	// A8-A15
	TRIS(I88_A16) = 0;			// Set as output
	TRIS(I88_A17) = 0;			// Set as output
	TRIS(I88_A18) = 0;			// Set as output
	TRIS(I88_A19) = 0;			// Set as output

	TRIS(I88_RD) = 0;           // output
	TRIS(I88_WR) = 0;           // output
	// SRAM U4, U5 are HiZ

	TRIS(I88_IOM) = 0;    // Set as output

}

static void bus_release_req(void) {
	// Set address bus as input
	TRIS(I88_ADDR_L) = 0xff;    // A7-A0
	TRIS(I88_ADDR_H) = 0xff;    // A8-A15
	TRIS(I88_A16) = 1;    // Set as input
	TRIS(I88_A17) = 1;    // Set as input
	TRIS(I88_A18) = 1;    // Set as input
	TRIS(I88_A19) = 1;    // Set as input

	// Set /RD and /WR as input
	TRIS(I88_RD) = 1;           // input
	TRIS(I88_WR) = 1;           // input
	TRIS(I88_IOM) = 1;          // input
}

//--------------------------------
// event loop ( PIC MAIN LOOP )
//--------------------------------
void board_event_loop(void) {

	while(1) {
		if (CMD_REQ) {					// CLC3OUT =1
			set_hold_pin();				// HOLD = 1, wait until HOLDA = 1
		    bus_hold_req();				// PIC becomes a busmaster
			(*bus_master_operation)();
			bus_release_req();
			reset_ioreq();				// reset CLC3 (CMD_REQ : CLC3OUT = 0)
			reset_hold_pin();			// HOLD = 0, wait until HOLDA = 0
		}
		if (nmi_flg) nmi_sig_on();
	}
}

#include "../../drivers/pic18f57q43_spi.c"
#include "../../drivers/SDCard.c"

