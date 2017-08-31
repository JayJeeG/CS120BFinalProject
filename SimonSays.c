#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include "io.c"

unsigned char objectives[9];
unsigned char userinputs[9];
unsigned char rightnotes[9];
unsigned char level = 1;
unsigned char gameactive = 0;
unsigned char lives = 3;

//Timer Functions. Do not modify
volatile unsigned char TimerFlag = 0; // TimerISR() sets this to 1. C programmer should clear to 0.
// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks

void TimerOn() 
{
	// AVR timer/counter controller register TCCR1
	TCCR1B = 0x0B;// bit3 = 0: CTC mode (clear timer on compare)
	// bit2bit1bit0=011: pre-scaler /64
	// 00001011: 0x0B
	// SO, 8 MHz clock or 8,000,000 /64 = 125,000 ticks/s
	// Thus, TCNT1 register will count at 125,000 ticks/s
	// AVR output compare register OCR1A.
	OCR1A = 125;	// Timer interrupt will be generated when TCNT1==OCR1A
	// We want a 1 ms tick. 0.001 s * 125,000 ticks/s = 125
	// So when TCNT1 register equals 125,
	// 1 ms has passed. Thus, we compare to 125.
	// AVR timer interrupt mask register
	TIMSK1 = 0x02; // bit1: OCIE1A -- enables compare match interrupt
	//Initialize avr counter
	TCNT1=0;

	_avr_timer_cntcurr = _avr_timer_M;
	// TimerISR will be called every _avr_timer_cntcurr milliseconds

	//Enable global interrupts
	SREG |= 0x80; // 0x80: 1000000
}

void TimerOff() {
	TCCR1B = 0x00; // bit3bit1bit0=000: timer off
}

void TimerISR() {
	TimerFlag = 1;
}

// In our approach, the C programmer does not touch this ISR, but rather TimerISR()
ISR(TIMER1_COMPA_vect) 
{
	// CPU automatically calls when TCNT1 == OCR1 (every 1 ms per TimerOn settings)
	_avr_timer_cntcurr--; // Count down to 0 rather than up to TOP
	if (_avr_timer_cntcurr == 0) { // results in a more efficient compare
		TimerISR(); // Call the ISR that the user uses
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

// Set TimerISR() to tick every M ms
void TimerSet(unsigned long M) 
{
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}
//End of Timer Functions. Modify below:

void PrintIntroduction()
{
	PORTB = 0x00;
	LCD_ClearScreen();
	LCD_DisplayString(1,"Welcome to Simon Says!");
	PORTB = 0x08;
	for (unsigned char i = 0; i < 5; ++i)
	{
		while(!TimerFlag);
		TimerFlag = 0;
	}
	LCD_ClearScreen();
	LCD_DisplayString(1,"Replicate button patterns.");
	PORTB = 0x04;
	for (unsigned char i = 0; i < 5; ++i)
	{
		while(!TimerFlag);
		TimerFlag = 0;
	}
	LCD_ClearScreen();
	LCD_DisplayString(1,"Get 9 to win.");
	PORTB = 0x02;
	for (unsigned char i = 0; i < 5; ++i)
	{
		while(!TimerFlag);
		TimerFlag = 0;
	}
	LCD_ClearScreen();
	LCD_DisplayString(1,"Fail 3 times and you lose.");
	PORTB = 0x01;
	for (unsigned char i = 0; i < 5; ++i)
	{
		while(!TimerFlag);
		TimerFlag = 0;
	}
	LCD_ClearScreen();
	LCD_DisplayString(1,"Press a button to start");
	PORTB = 0x0F;
	unsigned char letstart = 0;
	while(!letstart)
	{
		if (~PINA & 0x01){letstart = 1;}
		if (~PINA & 0x02){letstart = 1;}
		if (~PINA & 0x04){letstart = 1;}
		if (~PINA & 0x08){letstart = 1;}
	}
	PORTB = 0x00;
	for (unsigned char i = 0; i < 5;++i)
	{
		while(!TimerFlag);
		TimerFlag = 0;
	}
	gameactive = 1;
	return;
}

void GenerateSequence()
{
	for (unsigned char i = 0; i < 9; ++i)
	{
		int randomnumber = rand() % 4 + 1;
		if      (randomnumber == 4){objectives[i] = 0x08;rightnotes[i] = 523.25;}
		else if (randomnumber == 3){objectives[i] = 0x04;rightnotes[i] = 440.00;}
		else if (randomnumber == 2){objectives[i] = 0x02;rightnotes[i] = 349.23;}
		else                       {objectives[i] = 0x01;rightnotes[i] = 293.66;}		
	}
	return;	
}

void ShowLevel()
{
	Display();
	for (unsigned char i = 0; i < level; i++)
	{
		PORTB = objectives[i];
		if (objectives[i] == 0x08){set_PWM(523.25);}
		else if (objectives[i] == 0x04){set_PWM(440.00);}
		else if (objectives[i] == 0x02){set_PWM(349.23);}
		else if (objectives[i] == 0x01){set_PWM(293.66);}
		while(!TimerFlag);
		TimerFlag = 0;
		PORTB = 0x00;
		set_PWM(0);
		while(!TimerFlag);
		TimerFlag = 0; 
	}
	return;
}

void UserGameplay()
{
	unsigned char debouncer = 0;
	unsigned char i = 0;
	while(i != level)
	{	
		if (!(~PINA & 0x01) && debouncer)				  {debouncer = 0;}
		else if (!(~PINA & 0x02) && debouncer)            {debouncer = 0;}
		else if (!(~PINA & 0x04) && debouncer)            {debouncer = 0;}
		else if (!(~PINA & 0x04) && debouncer)            {debouncer = 0;}			
		if (!debouncer && (~PINA & 0x01)){userinputs[i] = 0x01; debouncer = 1;PORTB = userinputs[i];set_PWM(293.66);++i;} 
		else if (!debouncer && (~PINA & 0x02)){userinputs[i] = 0x02; debouncer = 1;PORTB = userinputs[i];set_PWM(349.23);++i;}
		else if (!debouncer && (~PINA & 0x04)){userinputs[i] = 0x04; debouncer = 1;PORTB = userinputs[i];set_PWM(440.00);++i;}
		else if (!debouncer && (~PINA & 0x08)){userinputs[i] = 0x08; debouncer = 1;PORTB = userinputs[i];set_PWM(523.25);++i;}
		while(!TimerFlag);
		TimerFlag = 0;
		set_PWM(0);
		PORTB = 0xF0;
		while(!TimerFlag);
		TimerFlag = 0;
	}
	return;
}

void SequenceCheck()
{
	unsigned char gamecheck = 0;
	for (unsigned char i = 0; i < level; i++)
			{
				if (userinputs[i] != objectives[i]){gamecheck++;}
			}
			if (gamecheck)
			{
				LCD_ClearScreen();
				LCD_DisplayString(1,"You is not right");
				lives--;
			}
			else if (!gamecheck)
			{
				LCD_ClearScreen();
				LCD_DisplayString(1,"You is right");
				level++;
			}
			PORTB = 0x0F;
			while(!TimerFlag);
			TimerFlag = 0;
			PORTB = 0x00;
			while(!TimerFlag);
			TimerFlag = 0;
	return;
}

void CheckEndGame()
{
	if (!lives)
	{
		LCD_ClearScreen();
		LCD_DisplayString(1, "GAME OVER");
		gameactive = 0;
		for(unsigned char i = 0; i < 10; i++)
		{
			while(!TimerFlag);
			TimerFlag = 0;
		}
	}
	else if (lives && level == 10)
	{
		LCD_ClearScreen();
		LCD_DisplayString(1, "WINNER OF CHICKEN DINNER");
		gameactive = 0;
		for(unsigned char i = 0; i < 10; i++)
		{
			while(!TimerFlag);
			TimerFlag = 0;
		}
	}
	return;
}

void Display()
{
	LCD_ClearScreen();
	LCD_DisplayString(3,"Live, Score: ");
	LCD_Cursor(1);
	LCD_WriteData('0' + lives);
	LCD_Cursor(16);
	LCD_WriteData('0' + level);
	return;
}

void set_PWM(double frequency) {
	
	
	// Keeps track of the currently set frequency
	// Will only update the registers when the frequency
	// changes, plays music uninterrupted.
	static double current_frequency;
	if (frequency != current_frequency) {

		if (!frequency) TCCR3B &= 0x08; //stops timer/counter
		else TCCR3B |= 0x03; // resumes/continues timer/counter
		
		// prevents OCR3A from overflowing, using Pre-scaler 64
		// 0.954 is smallest frequency that will not result in overflow
		if (frequency < 0.954) OCR3A = 0xFFFF;
		
		// prevents OCR3A from underflowing, using Pre-scaler 64					// 31250 is largest frequency that will not result in underflow
		else if (frequency > 31250) OCR3A = 0x0000;
		
		// set OCR3A based on desired frequency
		else OCR3A = (short)(8000000 / (128 * frequency)) - 1;

		TCNT3 = 0; // resets counter
		current_frequency = frequency;
	}
}

void PWM_on() {
	TCCR3A = (1 << COM3A0);
	// COM3A0: Toggle PB6 on compare match between counter and OCR3A
	TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);
	// WGM32: When counter (TCNT3) matches OCR3A, reset counter
	// CS31 & CS30: Set a Pre-scaler of 64
	set_PWM(0);
}

void PWM_off() {
	TCCR3A = 0x00;
	TCCR3B = 0x00;
}


int main(void) //Objective: Simon Says Game that holds up to nine sequences
{
	DDRA = 0x00; PORTA = 0xFF; // Button inputs
	DDRB = 0xFF; PORTB = 0x00; // LED outputs
	DDRC = 0xFF; PORTC = 0x00; // LCD data lines
	DDRD = 0xFF; PORTD = 0x00; // LCD control lines
	srand(time(NULL));
	LCD_init();
	TimerSet(1000);
	TimerOn();
	PWM_on();
	set_PWM(0);
	while (1) 
    {
		lives = 3;
		PrintIntroduction();
		GenerateSequence();
		while(gameactive)
		{
			ShowLevel();
			UserGameplay();
			SequenceCheck();
			CheckEndGame();
		}
		
    }
}

