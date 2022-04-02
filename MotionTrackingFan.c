// Software to run on a TI MSP430 Microcontroller for a Motion Tracking Fan
// Created by Dillon McCardell


#include <msp430.h> 

void delay_ms(int msecs); // Declare function created below

// Global variables to work between functions
int x;
float speed;

void main(void)
{
    //Initialize variables
    unsigned char i=0;
    int j;
    int key1;
    int key2;


    WDTCTL = WDTPW | WDTHOLD;               // stop watchdog timer
    CCTL0 |= CCIE;                          // Interrupt Enable for CCR0;
    TACTL = TASSEL_2 + MC_1;                // Select SMCLK and STOP Mode and Interrupt Enable
    _enable_interrupt();                    // Enable all interrupts
    BCSCTL1 = CALBC1_1MHZ;                  // Set MCLK = SMCLK = 1MHz
    DCOCTL = CALDCO_1MHZ;                   // Calibrate DCO

    ADC10CTL1 = INCH_7;                     // Select Analog Input A7 (P1.7)
    ADC10CTL0 = ADC10ON;                    // Enable ADC

    P1SEL = 0x00;                           // Select port 1 as an I/O port
    P2SEL = 0x00;                           // Select port 2 as an I/O port
    P1DIR = BIT0|BIT1|BIT2|BIT3;            // P1.0-P1.3 as outputs for Stepper Motor
    P2DIR = BIT3|BIT4|BIT6;                 // P2.3,P2.4 as outputs for LED indicator
    P1REN = BIT4|BIT5;                      // Engage pull up/down resistor on P1.4 and P1.5 for PIR sensor input
    P2REN = BIT7;                           // Engage pull up/down resistor on P2.7 for Button Input
    P1OUT &= ~BIT4 + ~BIT5;                 // Selects pull down resistors
    P2OUT &= BIT7;                          // Selects pull up resistor

    P2IE = 0x80;                            // Enables interrupt on P2.7
    P2IES = 0;                              // Interrupt Edge Select -> Trigger on rising edge
    P2IFG &= ~0x80;                         // Clear flag

    key1 = 1;   // Locks CCW to 1 execution per cycle of main
    key2 = 1;   // Locks CW to 1 execution per cycle of main

    P2OUT &= ~BIT3 + ~BIT4 + ~BIT6;         // Initialize LEDS off

for(;;)
{

// CCW rotation of Stepper Motor
    if((((P1IN & BIT5)!=0) &! ((P1IN & BIT4)!=0)) & (key1)){       // If sensor input on P1.5, and not P1.4, and key1 is TRUE
        P2OUT ^= BIT4;                      // Set P2.4 LED on
        for(j=0; j<1000; j++)               // Loop 1000 times to rotate stepper motor ~180º
        {                                   //                                                 CCW BIT SHIFT PATTERN:
            x=0;
            P1OUT = (1<<i);                 // Left shift 0001 pattern i times (CCW rotation)       0 0 0 1     i=0
            i++;                            // Advance to the next output pin                       0 0 1 0     i=1
            delay_ms(speed);                // Delay 5ms between pulses (speed adjustment)          0 1 0 0     i=2
            if (i==4) i=0;                  // Reset i -> Repeat Pulse                              1 0 0 0     i=3
            key1 = 0;                       // Reset key1, locking CCW rotation
            key2 = 1;                       // Set key2, unlocking CW rotation
        }
        P2OUT ^= BIT4;                      // Reset P2.4 LED to off
     }

// CW rotation of Stepper Motor
    if((((P1IN  & BIT4)!=0) &! ((P1IN & BIT5)!=0)) & (key2)){       // If sensor input on P1.4, and not P1.3, and key2 is TRUE
        P2OUT ^= BIT3;                      // Set P2.3 LED on
        for(j=0; j<1000; j++)               // Loop 1000 times to rotate Stepper Motor ~180º
        {                                   //                                                  CW BIT SHIFT PATTERN:
            x=0;
            P1OUT = (8>>i);                 // Right shift 1000 pattern i times (CW rotation)       1 0 0 0     i=0
            i++;                            // Advance to the next output pin                       0 1 0 0     i=1
            delay_ms(speed);                // Delay 5ms                                            0 0 1 0     i=2
            if (i==4) i=0;                  // Reset i -> Repeat pulse                              0 0 0 1     i=3
            key1 = 1;                       // Set key1, unlocking CCW rotation
            key2 = 0;                       // Reset key2, locking CW rotation
        }
        P2OUT ^= BIT3;                      // Reset P2.3 LED to off
    }
}
}

void delay_ms(int msecs){                   // Function for Delay call in milliseconds
    TACCR0 = 999;   // Start Timer (Calibrated for 1ms delay with 1MHz Clock)       TACCR0+1
                                            //                                     --------- = .001s -> TACCR0 = 999
                                            //                                        1MHz
    while(x<msecs);                         // Do nothing while waiting for TAR = TACCR0 to trigger interrupt. Repeat msecs times.
    TACCR0 = 0;                             // Stop Timer
}

// Timer ISR
#pragma vector= TIMER0_A0_VECTOR
__interrupt void TIMER_A_CCR0_ISR(void){
    x++;                                    // Increment x, allowing while loop in delay_ms function to work
}

#pragma vector= PORT2_VECTOR
__interrupt void button_press(void){
    // ADC reading potentiometer for stepper motor speed
    while((P2IN & BIT7)==0);                // While button is pressed, do nothing (execute on release)
    while((P2IN & BIT7)!=0){                // While button is not pressed (allows for toggle switch)
        P2OUT |= BIT6;                      // Turn P2.6 LED on (indicates ADC is ready for input)
        ADC10CTL0 |= ENC + ADC10SC;         // ENC -> Enables ADC conversion, ADC10SC -> Starts ADC conversion
        while(ADC10CTL1 & ADC10BUSY);       // While a conversion is in progress do nothing
        speed = ADC10MEM * 0.024;           // Scale ADC10MEM value to less than 25.
        if(speed < 5) speed = 5;            // Set a bottom threshold of 5 (max speed)
        speed = (int)speed;                 // Convert speed to int
    }
    P2OUT &= ~BIT6;                         // Clear P2.6 LED (indicates ADC is no longer taking input)
    while((P2IN & BIT7)==0);                // While button is pressed do nothing (execute on release)
    P2IFG &= ~0x80;                         // Reset flag
}


