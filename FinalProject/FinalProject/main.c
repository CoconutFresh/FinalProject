/* Name: Glenn Bersabe NetID: gbers002
 * Project: Final Project (LED Cube)
 */

#include <avr/io.h>
#include <avr/interrupt.h>

//Positive
unsigned char layer = 0x00; //A0, A1, A2, A3
//Negative
unsigned char column12 = 0x00; //B0, B1, B2, B3, B4, B5, B6, B7
unsigned char column34 = 0x00; //D0, D1, D2, D3, D4, D5, D6, D7
//Timing
unsigned char timing = 2;
unsigned char count = 0;
//User input
 uint16_t joyStick = 0x00;
unsigned char hold = 0x00;


volatile unsigned char TimerFlag = 0; // TimerISR() sets this to 1. C programmer should clear to 0.

// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks

//ADC INITIATION (CODE FOUND FROM: http://maxembedded.com/2011/06/the-adc-of-the-avr/)
void adc_init()
{
	// AREF = AVcc
	ADMUX = (1<<REFS0);
	
	// ADC Enable and prescaler of 128
	// 16000000/128 = 125000
	ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
}

uint16_t adc_read(uint8_t ch)
{
	// select the corresponding channel 0~7
	// ANDing with ’7? will always keep the value
	// of ‘ch’ between 0 and 7
	ch &= 0b00000111;  // AND operation with 7
	ADMUX = (ADMUX & 0xF8)|ch; // clears the bottom 3 bits before ORing
	
	// start single convertion
	// write ’1? to ADSC
	ADCSRA |= (1<<ADSC);
	
	// wait for conversion to complete
	// ADSC becomes ’0? again
	// till then, run loop continuously
	while(ADCSRA & (1<<ADSC));
	
	return (ADC);
}

void TimerOn() {
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
ISR(TIMER1_COMPA_vect) {
	// CPU automatically calls when TCNT1 == OCR1 (every 1 ms per TimerOn settings)
	_avr_timer_cntcurr--; // Count down to 0 rather than up to TOP
	if (_avr_timer_cntcurr == 0) { // results in a more efficient compare
		TimerISR(); // Call the ISR that the user uses
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

// Set TimerISR() to tick every M ms
void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}


// Bit-access function
unsigned char SetBit(unsigned char x, unsigned char k, unsigned char b) { //x = unsigned char, k = position, b = HIGH/LOW
	return (b ? x | (0x01 << k) : x & ~(0x01 << k));
}
unsigned char GetBit(unsigned char x, unsigned char k) {
	return ((x & (0x01 << k)) != 0);
}


void turnEverythingOff() { //Turns all LEDs off
	for(unsigned char i = 0; i < 8; i++) {
		column12 = SetBit(column12, i, 0);
		column34 = SetBit(column34, i, 0);
	}
	for(unsigned char i = 0; i < 4; i++) {
		layer = SetBit(layer, i, 0);
	}
	//layer = 0x00;
	//column12 = 0x00;
	//column34 = 0x00;
}

void turnEverythingOn() { //Turns all LEDs on
	for(unsigned char i = 0; i < 8; i++) {
		column12 = SetBit(column12, i, 0);
		column34 = SetBit(column34, i, 0);
	}
	for(unsigned char i = 0; i < 4; i++) {
		layer = SetBit(layer, i, 1);
	}
	//layer = 0xFF;
	//column12 = 0x00;
	//column34 = 0x00;
}

enum FLO_states{FLO_start, FLO_ON, FLO_OFF}FLO_state;
void flickerOn() { //WORKS
	switch (FLO_state) { //	TRANSITION
		case FLO_start:
			FLO_state = FLO_ON;
			break;
		case FLO_ON:
			FLO_state = FLO_OFF;
			break;
		case FLO_OFF:
			FLO_state = FLO_ON;
			break;
	}
	
	switch (FLO_state) { //ACTIONS
		case FLO_start:
			turnEverythingOff();
		break;
		case FLO_ON:
			turnEverythingOn();
		break;
		case FLO_OFF:
			turnEverythingOff();
		break;
	}
	
}


enum LC_states{LC_start, LC_layer1, LC_layer2, LC_layer3, LC_layer4}LC_state;
void layerCascade() {
	
	switch (LC_state) { //transitions
		
		case LC_start: 
			count = 0;
			turnEverythingOff();
			LC_state = LC_layer1;
			break;
		case LC_layer1:
			if (count == timing) {
				LC_state = LC_layer2;
				count = 0;
			}
			else {
				LC_state = LC_layer1;
				count++;
			}
			break;
		case LC_layer2:
			if (count == timing) {
				LC_state = LC_layer3;
				count = 0;
			}
			else {
				LC_state = LC_layer2;
				count++;
			}
			break;
		case LC_layer3:
			if (count == timing) {
				LC_state = LC_layer4;
				count = 0;
			}
			else {
				LC_state = LC_layer3;
				count++;
			}
			break;
		case LC_layer4:
			if (count == timing) {
				LC_state = LC_layer1;
				count = 0;
			}
			else {
				LC_state = LC_layer4;
				count++;
			}
			break;
	}
	switch (LC_state) { //ACTIONS
		case LC_layer1:
			layer = SetBit(layer, 3, 0);
			//layer = layer & 0x07;
			layer = SetBit(layer, 0, 1);
			//layer = layer | 0x01;
			break;
		case LC_layer2:
			layer = SetBit(layer, 0 , 0);
			//layer = layer & 0x0E;
			layer = SetBit(layer, 1, 1);
			//layer = layer | 0x02;
			break;
		case LC_layer3:
			layer = SetBit(layer, 1 , 0);
			//layer = layer & 0x0D;
			layer = SetBit(layer, 2, 1);
			//layer = layer | 0x04;
			break;
		case LC_layer4:
			layer = SetBit(layer, 2 , 0);
			//layer = layer & 0x0B;
			layer = SetBit(layer, 3, 1);
			//layer = layer | 0x08;
			break;
	}
}

enum CC_states{CC_start, CC_column1, CC_column2, CC_column3, CC_column4}CC_state;
void columnCascade() {
	
	switch (CC_state) { //TRANSITIONS
		case CC_start:
			turnEverythingOn();
			count = 0;
			CC_state = CC_column1;
			break;
		case (CC_column1):
			if (count == timing) {
				CC_state = CC_column2;
				count = 0;
			}
			else {
				CC_state = CC_column1;
				count++;
			}
			break;
		case (CC_column2):
			if (count == timing) {
				CC_state = CC_column3;
				count = 0;
			}
			else {
				CC_state = CC_column2;
				count++;
			}
			break;
		case (CC_column3):
			if (count == timing) {
				CC_state = CC_column4;
				count = 0;
			}
			else {
				CC_state = CC_column3;
				count++;
			}
			break;
		case (CC_column4):
			if (count == timing) {
				CC_state = CC_column1;
				count = 0;
			}
			else {
				CC_state = CC_column4;
				count++;
			}
		break;
	}
	switch (CC_state) { //ACTION
		case CC_column1:
			column12 = 0xF0;
			column34 = 0xFF;
			break;
		case CC_column2:
			column12 = 0x0F;
			column34 = 0xFF;
			break;
		case CC_column3:
			column34 = 0xF0;
			column12 = 0xFF;
			break;
		case CC_column4:
			column34 = 0x0F;
			column12 = 0xFF;
			break;
	}
}

enum LE_states{LE_start, LE_state1, LE_state2, LE_state3, LE_state4}LE_state;
void layerExtend() { //works
	switch (LE_state) { //TRANSITIONS
		case LE_start:
			turnEverythingOff();
			count = 0;
			LE_state = LE_state1;
			break;
		case LE_state1:
			if (count == timing) {
				LE_state = LE_state2;
				count = 0;
			}
			else {
				LE_state = LE_state1;
				count++;
			}
			break;
		case LE_state2:
			if (count == timing) {
				LE_state = LE_state3;
				count = 0;
			}
			else {
				LE_state = LE_state2;
				count++;
			}
			break;
		case LE_state3:
			if (count == timing) {
				LE_state = LE_state4;
				count = 0;
			}
			else {
				LE_state = LE_state3;
				count++;
			}
			break;
		case LE_state4:
			if (count == timing) {
				layer = 0x00; //resets
				LE_state = LE_state1;
				count = 0;
			}
			else {
				LE_state = LE_state4;
				count++;
			}
		break;
	}
	switch (LE_state) { //ACTION
		case LE_state1:
			layer = SetBit(layer, 0, 1);
		break;
		case LE_state2: 
			layer = SetBit(layer, 1, 1);
		break;
		case LE_state3:
			layer = SetBit(layer, 2, 1);
		break;
		case LE_state4:
			layer = SetBit(layer, 3, 1);
		break;
	}
}

//void aroundEdgeDown() {
	//
//}
enum RF_states{RF_start, RF_lit, RF_reset}RF_state;
void randomFlicker() { //WORKS
	unsigned char randomLayer;
	unsigned char randomColumnMacro;
	unsigned char randomColumn;
	switch (RF_state) { //TRANSISTIONS
		case RF_start:
			count = 0;
			turnEverythingOff();
			randomLayer = rand() % 4;
			randomColumnMacro = rand() % 2;
			randomColumn = rand() % 8;
			RF_state = RF_lit;
		break;
		case RF_lit:
			if (count == 2) {
				RF_state = RF_reset;
				count = 0;
			}
			else {
				count++;
				RF_state = RF_lit;
			}
		break;
		case RF_reset:
			if (count == 2) {
				randomLayer = rand() % 4;
				randomColumnMacro = rand() % 2;
				randomColumn = rand() % 8;
				RF_state = RF_lit;
				count = 0;
			}
			else {
				count++;
				RF_state = RF_reset;
			}
		break;
	}
	switch (RF_state) { //ACTIONS
		case RF_lit:
			layer = SetBit(layer, randomLayer, 1);
			if (randomColumnMacro == 0)
				column12 = SetBit(column12, randomColumn, 1);
			else if (randomColumnMacro == 1)
				column34 = SetBit(column34, randomColumn, 1);
		break;
		case RF_reset:
			turnEverythingOff();
		break;
	}
}

enum RR_states{RR_start, RR_1, RR_2, RR_3, RR_4, RR_5}RR_state;
void randomRain() {
	unsigned char randomLayer;
	unsigned char randomColumnMacro;
	unsigned char randomColumn;
	switch (RR_state) {
		case RR_start:
			count = 0;
			turnEverythingOff();
			randomLayer = rand() % 4;
			randomColumnMacro = rand() % 2;
			randomColumn = rand() % 8;
			RR_state = RR_1;
			break;
		case RR_1:
			if (count == 1) {
				RR_state = RR_2;
				count = 0;
			}
			else {
				count++;
				RF_state = RR_1;
			}
			break;
		case RR_2:
			if (count == 1) {
				RR_state = RR_3;
				count = 0;
			}
			else {
				count++;
				RF_state = RR_2;
			}
		break;
		case RR_3:
			if (count == 1) {
				RR_state = RR_4;
				count = 0;
			}
			else {
				count++;
				RF_state = RR_3;
			}
		break;
		case RR_4:
			if (count == 1) {
				RR_state = RR_5;
				count = 0;
			}
			else {
				count++;
				RF_state = RR_4;
			}
		break;
		case RR_5:
			if (count == 1) {
				RR_state = RR_start;
				count = 0;
			}
			else {
				count++;
				RF_state = RR_5;
			}
		break;
	}
	switch(RR_state) {
		case RR_1:
			if (randomColumnMacro == 0) {
				column12 = SetBit(column12, randomColumn, 0);
			}
			else if (randomColumnMacro == 2) {
				column34 = SetBit(column34, randomColumn, 0);
			}
			layer = SetBit(layer, 0, 1);
			break;
		case RR_2:
			layer = SetBit(layer, 0, 0);
			layer = SetBit(layer, 1, 1);
			break;
		case RR_3:
			layer = SetBit(layer, 1, 0);
			layer = SetBit(layer, 2, 1);
			break;
		case RR_4:
			layer = SetBit(layer, 2, 0);
			layer = SetBit(layer, 3, 1);
			break;
		case RR_5:
			layer = SetBit(layer, 3, 0);
			if (randomColumnMacro == 0) {
				column12 = SetBit(column12, randomColumn, 1);
			}
			else if (randomColumnMacro == 2) {
				column34 = SetBit(column34, randomColumn, 1);
			}
			break;
	}
}


unsigned char buttonToggle() {
	if (joyStick > 700) {
		hold = 1;
		return 1;
	}
	else if (joyStick <= 300 ) {
		hold = 0;
		return 0;
	}
	return 0; //(!button && !hold)  and (button && hold)
}

enum {start, OFF, ON, FO, LC, CC, LE, RF, RR}state;
void tick() {
	switch (state) { //transitions
		case start:
			state = OFF;
			break;
		case OFF:
			if (buttonToggle()) {
				state = ON;
			}
			else {
				state = OFF;
			}
			break;
		case ON:
			if (buttonToggle()) {
				state = FO;
			}
			else {
				state = ON;
			}
			break;
		case FO:
		if (buttonToggle()) {
			state = LC;
		}
		else {
			state = FO;
		}
		break;
		case LC:
			if (buttonToggle()) {
				state = CC;
			}
			else {
				state = LC;
			}
		break;
		case CC:
			if (buttonToggle()) {
				state = LE;
			}
			else {
				state = CC;
		}
		break;
		case LE:
			if (buttonToggle()) {
				state = RF;
			}
			else {
				state = LE;
			}
		break;
		case RF:
			if (buttonToggle()) {
				state = RR;
			}
			else {
				state = RF;
			}
		break;
		case RR:
			if (buttonToggle()) {
				state = OFF;
			}
			else {
				state = RR;
			}
			break;
	}
	switch (state) { //ACTIONS
		case OFF:
			turnEverythingOff();
			break;
		case ON:
			turnEverythingOn();
			break;
		case FO:
			flickerOn();
			break;
		case LC:
			layerCascade();
			break;
		case CC:
			columnCascade();
			break;
		case LE:
			layerExtend();
			break;
		case RF:
			randomFlicker();
			break;
		case RR:
			randomRain();
			break;
	}
	PORTB = column12;
	PORTC = layer;
	PORTD = column34;
}
int main(void) //LC, CC, LE, RF
{
	adc_init();
	DDRA = 0x00; PORTA = 0xFF; //Input
	DDRC = 0xFF; PORTC = 0x00; //Layer Output
	DDRB = 0xFF; PORTB = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	
	
	FLO_state = FLO_start;
	LC_state = LC_start;
	CC_state = CC_start;
	LE_state = LE_start;
	RF_state = RF_start;
	RR_state = RR_start;
	
	TimerSet(100);
	TimerOn();

    while (1) 
    {
		joyStick = adc_read(0);
		tick();
		while(!TimerFlag);
		TimerFlag = 0;
		
    }
}

