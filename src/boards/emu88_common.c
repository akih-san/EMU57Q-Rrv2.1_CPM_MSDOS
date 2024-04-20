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

#include "../emu88.h"

// console input buffers
#define U3B_SIZE 128
unsigned char rx_buf[U3B_SIZE];	//UART Rx ring buffer
unsigned int rx_wp, rx_rp, rx_cnt;

// AUX: input buffers
#define U1B_SIZE 128
unsigned char ax_buf[U1B_SIZE];	//UART Rx ring buffer
unsigned int ax_wp, ax_rp, ax_cnt;
int nmi_flg;


//TIMER0 seconds counter
static union {
    unsigned int w; //16 bits Address
    struct {
        unsigned char l; //Address low
        unsigned char h; //Address high
    };
} adjCnt;

TPB tim_pb;			// TIME device parameter block

void uart1_init(void) {
	ax_wp = 0;
	ax_rp = 0;
	ax_cnt = 0;
}

//initialize TIMER0 & TIM device parameter block
void timer0_init(void) {
	adjCnt.w = TIMER0_INITC;	// set initial adjust timer counter
	tim_pb.TIM_DAYS = TIM20240101;
	tim_pb.TIM_MINS = 0;
	tim_pb.TIM_HRS = 0;
	tim_pb.TIM_SECS = 0;
	tim_pb.TIM_HSEC = 0;
}

//
// define interrupt
//
// Never called, logically
void __interrupt(irq(default),base(8)) Default_ISR(){}

////////////// TIMER0 vector interrupt ////////////////////////////
//TIMER0 interrupt
/////////////////////////////////////////////////////////////////
void __interrupt(irq(TMR0),base(8)) TIMER0_ISR(){

	union {
    	unsigned int w; //16 bits Address
    	struct {
        	unsigned char l; //Address low
        	unsigned char h; //Address high
    	};
	} tmpCnt;

	TMR0IF =0; // Clear timer0 interrupt flag

	// adjust timer conter
	tmpCnt.l = TMR0L;
	tmpCnt.h = TMR0H;
	tmpCnt.w = tmpCnt.w + adjCnt.w;

	TMR0L = tmpCnt.l;
	TMR0H = tmpCnt.h;

	if( ++tim_pb.TIM_SECS == 60 ) {
		tim_pb.TIM_SECS = 0;
		if ( ++tim_pb.TIM_MINS == 60 ) {
			tim_pb.TIM_MINS = 0;
			if ( ++tim_pb.TIM_HRS == 24 ) {
				tim_pb.TIM_HRS = 0;
				tim_pb.TIM_DAYS++;
			}
		}
	}
	tim_pb.TIM_HSEC = 0;
}

////////////// UART3 Receive interrupt ////////////////////////////
// UART3 Rx interrupt
// PIR9 (bit0:U3RXIF bit1:U3TXIF)
/////////////////////////////////////////////////////////////////
void __interrupt(irq(U3RX),base(8)) URT3Rx_ISR(){

	unsigned char rx_data;

	rx_data = U3RXB;			// get rx data

	if (rx_data == CTL_Q) {
		nmi_flg = 1;
	}
	else if (rx_cnt < U3B_SIZE) {
		rx_buf[rx_wp] = rx_data;
		rx_wp = (rx_wp + 1) & (U3B_SIZE - 1);
		rx_cnt++;
	}
}

// UART3 Transmit
void putch(char c) {
    while(!U3TXIF);             // Wait or Tx interrupt flag set
    U3TXB = c;                  // Write data
}

// UART3 Recive
int getch(void) {
	char c;

	while(!rx_cnt);             // Wait for Rx interrupt flag set
	GIE = 0;                // Disable interrupt
	c = rx_buf[rx_rp];
	rx_rp = (rx_rp + 1) & ( U3B_SIZE - 1);
	rx_cnt--;
	GIE = 1;                // enable interrupt
    return c;               // Read data
}

////////////// UART1 Receive interrupt ////////////////////////////
// UART1 Rx interrupt
// PIR4 (bit0:U1RXIF bit1:U1TXIF)
/////////////////////////////////////////////////////////////////
void __interrupt(irq(U1RX),base(8)) URT1Rx_ISR(){

	unsigned char ax_data;

	ax_data = U1RXB;			// get rx data

	if (ax_cnt < U1B_SIZE) {
		ax_buf[ax_wp] = ax_data;
		ax_wp = (ax_wp + 1) & (U1B_SIZE - 1);
		ax_cnt++;
	}
}

// UART1 Transmit
void putax(char c) {
    while(!U1TXIF);             // Wait or Tx interrupt flag set
    U1TXB = c;                  // Write data
}

// UART1 Recive
int getax(void) {
	char c;

	while(!ax_cnt);			// Wait for Rx interrupt flag set
	GIE = 0;                // Disable interrupt

	c = ax_buf[ax_rp];
	ax_rp = (ax_rp + 1) & ( U1B_SIZE - 1);
	ax_cnt--;
	GIE = 1;                // enable interrupt

	return c;               // Read data
}

void devio_init(void) {
	rx_wp = 0;
	rx_rp = 0;
	rx_cnt = 0;
	nmi_flg = 0;
}

uint32_t get_physical_addr(uint16_t ah, uint16_t al)
{
// real 32 bit address
//	return (uint32_t)ah*0x1000 + (uint32_t)al;

// 8086 : segment:offset
	return (uint32_t)ah*0x10 + (uint32_t)al;
}


static void emu88_common_sys_init()
{
    // System initialize
    OSCFRQ = 0x08;      // 64MHz internal OSC

	// Disable analog function
    ANSELA = 0x00;
    ANSELB = 0x00;
    ANSELC = 0x00;
    ANSELD = 0x00;
    ANSELF = 0x00;
    ANSELE0 = 0;
    ANSELE1 = 0;
    ANSELE2 = 0;


    // RESET output pin
	LAT(I88_RESET) = 1;        // Reset
    TRIS(I88_RESET) = 0;       // Set as output

	// HOLDA
	WPU(I88_HOLDA) = 1;     // I88_HOLDA Week pull up
	LAT(I88_HOLDA) = 1;     // set HOLDA=1 (74LS373 is Hi-Z)
	TRIS(I88_HOLDA) = 0;    // Set as output during RESET period

    // UART3 initialize
    U3BRG = 416;        // 9600bps @ 64MHz
    U3RXEN = 1;         // Receiver enable
    U3TXEN = 1;         // Transmitter enable

    // UART3 Receiver
    TRISA7 = 1;         // RX set as input
    U3RXPPS = 0x07;     // RA7->UART3:RXD;

    // UART3 Transmitter
    LATA6 = 1;          // Default level
    TRISA6 = 0;         // TX set as output
    RA6PPS = 0x26;      // UART3:TXD -> RA6;

    U3ON = 1;           // Serial port enable

    U3RXIE = 1;          // Receiver interrupt enable

	// Init address LATCH to 0
	// Address bus A7-A0 pin
    WPU(I88_ADDR_L) = 0xff;     // Week pull up
	LAT(I88_ADDR_L) = 0xff;
    TRIS(I88_ADDR_L) = 0x00;    // Set as output

    // Address bus A15-A8
    WPU(I88_ADDR_H) = 0xff;     // Week pull up
    LAT(I88_ADDR_H) = 0xff;
    TRIS(I88_ADDR_H) = 0x00;    // Set as output


	WPU(I88_A16) = 1;     // A16 Week pull up
	LAT(I88_A16) = 1;     // init A16=0
    TRIS(I88_A16) = 0;    // Set as output

	WPU(I88_A17) = 1;     // A17 Week pull up
	LAT(I88_A17) = 1;     // init A17=0
    TRIS(I88_A17) = 0;    // Set as output

	WPU(I88_A18) = 1;     // A18 Week pull up
	LAT(I88_A18) = 1;     // init A18=0
    TRIS(I88_A18) = 0;    // Set as output

	WPU(I88_A19) = 1;     // A19 Week pull up
	LAT(I88_A19) = 1;     // init A19=0
    TRIS(I88_A19) = 0;    // Set as output

	// Data bus D7-D0 pin
    WPU(I88_DATA) = 0x00;       // disable week pull up
    LAT(I88_DATA) = 0x00;
    TRIS(I88_DATA) = 0x00;      // Set as output

}

static void emu88_common_start_i88(void)
{
    // Address bus A15-A8
	TRIS(I88_ADDR_H) = 0xff;    // Set as input

    // Address bus A7-A0 pin
    TRIS(I88_ADDR_L) = 0xff;    // Set as input
    TRIS(I88_A16) = 1;    // Set as input
    TRIS(I88_A17) = 1;    // Set as input
    TRIS(I88_A18) = 1;    // Set as input
    TRIS(I88_A19) = 1;    // Set as input

	// Data bus D7-D0 input pin
    TRIS(I88_DATA) = 0xff;      // Set as input

    TRIS(I88_RD) = 1;           // Set as input
    TRIS(I88_WR) = 1;           // Set as input
	TRIS(I88_IOM) = 1;			// Set as input
}

void write_sram(uint32_t addr, uint8_t *buf, unsigned int len)
{
    union address_bus_u ab;
    unsigned int i;

    ab.w = addr;

	// set SRAM read address
	TRIS(I88_DATA) = 0x00;	// Set as output

	// set SRAM read address
	LAT(I88_ADDR_H) = ab.lh;
    LAT(I88_ADDR_L) = ab.ll;
	LAT(I88_A16) = (ab.hl & 0x01) ? 1 : 0;
	LAT(I88_A17) = (ab.hl & 0x02) ? 1 : 0;
	LAT(I88_A18) = (ab.hl & 0x04) ? 1 : 0;
	LAT(I88_A19) = (ab.hl & 0x08) ? 1 : 0;
    for(i = 0; i < len; i++) {
        LAT(I88_WR) = 0;					// activate /WE
        LAT(I88_DATA) = ((uint8_t*)buf)[i];
        LAT(I88_WR) = 1;					// deactivate /WE

    	LAT(I88_ADDR_L) = ++ab.ll;
        if (ab.ll == 0) {
		    LAT(I88_ADDR_H) = ++ab.lh;
        	if (ab.lh == 0) {
        		ab.hl++;
        		LAT(I88_A16) = (ab.hl & 0x01) ? 1 : 0;
				LAT(I88_A17) = (ab.hl & 0x02) ? 1 : 0;
				LAT(I88_A18) = (ab.hl & 0x04) ? 1 : 0;
				LAT(I88_A19) = (ab.hl & 0x08) ? 1 : 0;
        	}
        }
    }
	TRIS(I88_DATA) = 0xff;	// Set as input
}

void read_sram(uint32_t addr, uint8_t *buf, unsigned int len)
{
    union address_bus_u ab;
    unsigned int i;

	ab.w = addr;

	LAT(I88_RD) = 0;      // activate /OE

	// set SRAM read address
	LAT(I88_ADDR_H) = ab.lh;
    LAT(I88_ADDR_L) = ab.ll;
	LAT(I88_A16) = (ab.hl & 0x01) ? 1 : 0;
	LAT(I88_A17) = (ab.hl & 0x02) ? 1 : 0;
	LAT(I88_A18) = (ab.hl & 0x04) ? 1 : 0;
	LAT(I88_A19) = (ab.hl & 0x08) ? 1 : 0;

	for(i = 0; i < len; i++) {
        ((uint8_t*)buf)[i] = PORT(I88_DATA);	// read data

		LAT(I88_ADDR_L) = ++ab.ll;
        if (ab.ll == 0) {
		    LAT(I88_ADDR_H) = ++ab.lh;
        	if (ab.lh == 0) {
        		ab.hl++;
				LAT(I88_A16) = (ab.hl & 0x01) ? 1 : 0;
				LAT(I88_A17) = (ab.hl & 0x02) ? 1 : 0;
				LAT(I88_A18) = (ab.hl & 0x04) ? 1 : 0;
				LAT(I88_A19) = (ab.hl & 0x08) ? 1 : 0;
        	}
        }
    }

	LAT(I88_RD) = 1;      // deactivate /OE

}

static void emu88_common_wait_for_programmer()
{
    //
    // Give a chance to use PRC (RB6) and PRD (RB7) to PIC programer.
    //
    printf("\n\r");
    printf("wait for programmer ...\r");
    __delay_ms(200);
    printf("                       \r");

    printf("\n\r");
}
