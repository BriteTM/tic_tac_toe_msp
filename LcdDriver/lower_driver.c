// This code was ported from TI's sample code. See Copyright notice at the bottom of this file.

#include <LcdDriver/lower_driver.h>
#include "msp430fr6989.h"
#include "Grlib/grlib/grlib.h"
#include <stdint.h>

void HAL_LCD_PortInit(void)
{
    /////////////////////////////////////
    // Configuring the SPI pins
    /////////////////////////////////////

    // Configure UCB0CLK/P1.4 pin to serial clock (Primary module function: P1SEL1=0, P1SEL0=1)
    P1SEL0 |= BIT4;
    P1SEL1 &= ~BIT4;

    // Configure UCB0SIMO/P1.6 pin to SIMO (Primary module function: P1SEL1=0, P1SEL0=1)
    P1SEL0 |= BIT6;
    P1SEL1 &= ~BIT6;

    // OK to ignore UCB0STE/P1.5 since we'll connect the display's enable bit to low (enabled all the time)
    // OK to ignore UCB0SOMI/P1.7 since the display doesn't give back any data

    ///////////////////////////////////////////////
    // Configuring the display's other pins
    ///////////////////////////////////////////////
    // Set reset pin as output (P4.7)
    P4DIR |= BIT7;

    // Set the data/command pin as output (P2.3)
    P2DIR |= BIT3;

    // Set the chip select pin as output (P2.5)
    P2DIR |= BIT5;

    return;
}

void HAL_LCD_SpiInit(void)
{
    //////////////////////////
    // SPI configuration
    //////////////////////////

    // Put eUSCI in reset state and set all fields in the register to 0
    UCB0CTLW0 = UCSWRST;

    // Fields that need to be nonzero are changed below

    // Set clock phase to "capture on 1st edge, change on following edge"
    UCB0CTLW0 |= UCCKPH;

    // Set clock polarity to "inactive low" (UCCKPL = 0)

    // Set data order to "transmit MSB first"
    UCB0CTLW0 |= UCMSB;

    // Set data size to 8-bit (UC7BIT = 0)

    // Set MCU to "SPI master"
    UCB0CTLW0 |= UCMST;

    // Set SPI to "3-pin SPI" (we won't use eUSCI's chip select) (UCMODE_0 = 0)

    // Set module to synchronous mode
    UCB0CTLW0 |= UCSYNC;

    // Set clock to SMCLK
    UCB0CTLW0 |= UCSSEL__SMCLK;

    // Configure the clock divider (SMCLK is set to 16 MHz; run SPI at 8 MHz using SMCLK)
    // Maximum SPI supported frequency on the display is 10 MHz
    UCB0BRW = 2;

    // Exit the reset state at the end of the configuration
    UCB0CTLW0 &= ~UCSWRST;

    // Set CS' (chip select) bit to 0 (display always enabled)
    P2OUT &= ~BIT5;

    // Set DC' bit to 0 (assume data)
    P2OUT &= ~BIT3;

    return;
}


//*****************************************************************************
// Writes a command to the CFAF128128B-0145T.  This function implements the basic SPI
// interface to the LCD display.
//*****************************************************************************
void HAL_LCD_writeCommand(uint8_t command)
{
    // Wait as long as the module is busy
    while (UCB0STATW & UCBUSY);

    // For command, set the DC' bit to low before transmission
    P2OUT &= ~BIT3;

    // Transmit data
    UCB0TXBUF = command;

    return;
}


//*****************************************************************************
// Writes a data to the CFAF128128B-0145T.  This function implements the basic SPI
// interface to the LCD display.
//*****************************************************************************
void HAL_LCD_writeData(uint8_t data)
{
    // Wait as long as the module is busy
    while (UCB0STATW & UCBUSY);

    // Set DC' bit back to high
    P2OUT |= BIT3;

    // Transmit data
    UCB0TXBUF = data;

    return;
}