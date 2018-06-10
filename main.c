#include <msp430.h> 

int iterator =0;
int flag=0;
char *sequence = "...---...#";
int enable= 0;
int space = 0;
int debouncingActive = 0;
int debouncingPassive = 0;
int debouncingCountedPasses = 0;

#define CONTROL_BUTTON_CHANGED 1
#define LED_DISPLAY 2
#define DEBOUNCING_PROCEED 3
#define CONTROL_BUTTON_PRESSED 4
#define DEBOUNCING_TIME     32
#define DEBOUNCING_SENSITIVITY    32
#define BASIC_TIME 5000
#define LINE_TIME BASIC_TIME
#define DOT_TIME BASIC_TIME/4
#define SHORT_BREAK BASIC_TIME/4
#define BREAK_TIME BASIC_TIME

#define LED     BIT0
#define ENABLE_LED BIT7
#define LED_OUT P1OUT
#define LED_DIR P1DIR
#define LED_SEL P1SEL

#define BTN     BIT0
#define BTN_IN P2IN
#define BTN_DIR P2DIR
#define BTN_IES P2IES
#define BTN_IE  P2IE
#define BTN_IFG P2IFG
/*
 * main.c
 */

void init()
{
    WDTCTL = WDTPW | WDTHOLD;   // Stop watchdog timer

    //LED_SEL |= LED;
    LED_DIR |= 0xFF;
    LED_OUT = 0x00;

    BTN_IES |= BTN;
    BTN_IE  |= BTN;
    BTN_DIR &= ~BTN;

    //Timer do diody
    TA0CCR0 = SHORT_BREAK;

    TA0CCTL0 |= CAP | CM_0;
    TA0CTL      |= TASSEL_1 | ID_3 | TAIE ;

    //Timer do debouncingu
    TB0CTL |= TBSSEL_1 | TBIE;
    TB0CCR0 = DEBOUNCING_TIME;
    TB0CCTL0 |= CAP | CM_0;

}

inline void startDebouncing()
{
    debouncingActive = 0;
    debouncingPassive = 0;
    TBCTL |= MC_1;
}

inline void processDebouncing()
{
    if(BTN_IN & BTN) // przycisk nie był wciśnięty, nie ma sensu zliczać dalej
    {
        debouncingPassive++;
        debouncingActive = 0;
    }
    else{
    	debouncingActive++;
    	debouncingPassive = 0;
    }
}

inline void stopDebouncing()
{
    TBCTL &= ~MC_1;
    BTN_IFG &= ~(BTN);
}

inline void enableLEDOn()
{
    LED_OUT |= ENABLE_LED;
}

inline void enableLEDOff()
{
    LED_OUT &= ~(ENABLE_LED);
}

inline int invert_value (int bool){
	if (bool == 0) bool = 1;
	else bool = 0;
	return bool;
}


int main(void) {
	init();
	__bis_SR_register(LPM3_bits | GIE);  // bo robimy na ACLK
    while(1)
    {
        __disable_interrupt();
        if(flag==LED_DISPLAY)
        {
        	flag=0;
        	if (space){
        		TA0CCR0 = SHORT_BREAK;
        		LED_OUT &= ~LED;
        	}
        	else {
        		int temp = nextBit();
        		if(temp == 0 && enable) space = invert_value(space);
        		if(temp==0 && (enable == 0)){ //właśnie wyświetliliśmy ostatni znak a użytkownik nie chce więcej
        			TACTL &= ~MC_1;
        			TA0CCR0 = SHORT_BREAK;
        		}
        	}
        	space = invert_value(space);
        }

        if(flag==CONTROL_BUTTON_PRESSED)
        {

        	flag=0;
            startDebouncing();
        }

        if(flag==DEBOUNCING_PROCEED)
        {
        	flag=0;
            processDebouncing();
            if(debouncingActive == DEBOUNCING_SENSITIVITY ) //byl wcisniety
            {
            	BTN_IE  |= BTN;
                stopDebouncing();
                enable = invert_value(enable);
                LED_OUT ^= ENABLE_LED;
                TACTL |= MC_1;
            }
            if(debouncingPassive == DEBOUNCING_SENSITIVITY ) // nie byl wcisniety
            {
            	BTN_IE  |= BTN;
            	stopDebouncing();
            }
            BTN_IFG &= ~BTN;
        }
        if(flag==0)
            __bis_SR_register(LPM3_bits | GIE);
    }
}

int nextBit()
{
	char n = sequence[iterator];
	iterator++;
	switch(n)
		   {
			   case '-':
			   TA0CCR0 = LINE_TIME;
			   LED_OUT |= LED;
			   return 1;
			   break;

			   case '.':
			   TA0CCR0 = DOT_TIME;
			   LED_OUT |= LED;
			   return 1;
			   break;

			   case '#':
			   iterator = 0;
			   TA0CCR0 = BREAK_TIME;
			   LED_OUT &= ~LED;
			   break;
		   }
    return 0;

}

int set_timer(int i){
	char n = sequence[i];
	 switch(n)
	               {
	               	   case '-':
	            	   TA0CCR0 = LINE_TIME;
	            	   return 1;
	                   break;

	                   case '.':
	                   TA0CCR0 = DOT_TIME;
	                   return 1;
	                   break;

	                   case '#':
	                   iterator = 0;
	                   TA0CCR0 = BREAK_TIME;
	                   return 0;
	                   break;
	               }
}

#pragma vector=TIMER0_A1_VECTOR
__interrupt void LED_timer (void)
{
	flag = LED_DISPLAY;
	TACTL &= ~(TAIFG);
    _BIC_SR_IRQ(LPM3_bits);
}


#pragma vector=TIMER0_B1_VECTOR
__interrupt void debouncing_timer(void)
{

		flag = DEBOUNCING_PROCEED;
		TBCTL &= ~(TBIFG);
		_BIC_SR_IRQ(LPM3_bits);


}

#pragma vector=PORT2_VECTOR
__interrupt void Port2ISR (void)
{
	flag = CONTROL_BUTTON_PRESSED;
	BTN_IE &= ~BTN;
	BTN_IFG &= ~BTN;
	 _BIC_SR_IRQ(LPM3_bits); // Budzimy sie

}
