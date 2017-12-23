#include "e_ink.h"
#include <Wire.h>


/*******************************************************************************/
unsigned char EPD_FB[60000]; // 1bpp Framebuffer

// Init waveform, basiclly alternating between black and white
#define FRAME_INIT_LEN 33

const unsigned char wave_init[FRAME_INIT_LEN] = {
	0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
	0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
	0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
	0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
	0,
};

// line delay for different shades of grey
// note that delay is accumulative
// it means that if it's the level 4 grey
// the total line delay is 90+90+90+90=360
// this could be used as a method of rough gamma correction
// these settings only affect 4bpp mode
const unsigned char timA[16] = {
	// 1  2  3  4  5  6  7  8  9 10 11  12  13  14  15
	90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 120, 120, 120, 120, 200
};

#define timB 20

// this only affect 1bpp mode
#define clearCount 4
#define setCount   4
/*******************************************************************************/


uint8_t line[200]; // Line data buffer

// Us delay that is not accurate
void Delay_Us(unsigned long x){
	unsigned long a;

	while (x--) {
		a = 17;
		while (a--) {
			asm ("nop");
		}
	}
}

void power_off(){
	// turn power off
	digitalWrite(EN_NEG_POWER,LOW);
	digitalWrite(EN_POS_POWER,LOW);
	digitalWrite(PIN_EN_POWER,HIGH);
	i2c_single(0,I2C_SHIFT_VDD);
}
void power_on(){
	// turn power on
	digitalWrite(EN_NEG_POWER,HIGH);
	digitalWrite(EN_POS_POWER,HIGH);
	digitalWrite(PIN_EN_POWER,LOW);
	i2c_single(1,I2C_SHIFT_VDD);
}

void gpio_init(){
	#define PIN_CL 0
	#define PIN_EN_POS_POWER 2
	#define PIN_EN_NEG_POWER 3
	#define PIN_EN_POWER 4
	#define PIN_LE 5
	#define PIN_OE 6
	#define PIN_I2C_SDA 7
	#define PIN_I2C_SCL 8

	pinMode(PIN_CL,OUTPUT);
	pinMode(PIN_EN_POS_POWER,OUTPUT);
	pinMode(PIN_EN_NEG_POWER,OUTPUT);
	pinMode(PIN_EN_POWER,OUTPUT);
	pinMode(PIN_LE,OUTPUT);
	pinMode(PIN_OE,OUTPUT);
	pinMode(PIN_I2C_SDA,OUTPUT);
	pinMode(PIN_I2C_SCL,OUTPUT);

	// turn power off
	power_off();
}

void i2c_init(){
	#define I2C_ADR 0b0100000
	Wire.begin();
	Wire.beginTransmission(I2C_ADR);
	Wire.send(6); // config register 1 and following 2
	Wire.send(0); // set all io0 to output
	Wire.send(0); // set all io1 to output
	Wire.endTransmission();
}

uint8_t i2c_shadow_0 = 0;
uint8_t i2c_shadow_1 = 0;

	#define I2C_SHIFT_SPH 0
	#define I2C_SHIFT_GMODE 1
	#define I2C_SHIFT_SPV 2
	#define I2C_SHIFT_CKV 3
	#define I2C_SHIFT_VDD 4

void i2c_single(bool v,uint8_t shift){
	if(v & !(i2c_shadow_1 & (1<<shift))){
		i2c_shadow_1 |= (1<<shift);
		i2c_send_1();
	} else if(!v &(i2c_shadow_1 & (1<<shift))){
		i2c_shadow_1 &= ~(1<<shift);
		i2c_send_1();
	}
}

void i2c_send_0(){
	i2c_send_0(i2c_shadow_0);
}

void i2c_send_0( uint8_t data){
	Wire.beginTransmission(I2C_ADR);
	Wire.send(2); // output 1 register
	Wire.send(data); // set outputs
	Wire.endTransmission();
}

void i2c_send_1(){
	Wire.beginTransmission(I2C_ADR);
	Wire.send(3); // output 1 register
	Wire.send(i2c_shadow_1); // set outputs
	Wire.endTransmission();
}

#define EDP_LE_L	digitalWrite(PIN_LE,LOW);
#define EDP_LE_H	digitalWrite(PIN_LE,HIGH);
#define EDP_OE_L	digitalWrite(PIN_OE,LOW);
#define EDP_OE_H	digitalWrite(PIN_OE,HIGH);
#define EDP_CL_L	digitalWrite(PIN_CL,LOW);
#define EDP_CL_H	digitalWrite(PIN_CL,HIGH);

void epd_init(void){
	i2c_init();
	gpio_init();

	i2c_single(1,I2C_SHIFT_GMODE);
	power_off();

	EPD_LE_L
	EPD_CL_L
	EPD_OE_L

	i2c_single(1,I2C_SHIFT_SPH);
	i2c_single(1,I2C_SHIFT_SPV);

	EPD_CKV_L
}

void EPD_send_row_data(uint8_t* pArray){

	// why latch here?
	// compare: http://essentialscrap.com/eink/waveforms.html
	// possibly to interleave in monochrome mode, latch and OE during sending next data?
	// this would be tecchnically latch the prev line
	EPD_LE_H
	EPD_CL_H
	EPD_CL_L
	EPD_CL_H
	EPD_CL_L

	EPD_LE_L
	EPD_CL_H
	EPD_CL_L
	EPD_CL_H
	EPD_CL_L
	// hmm

	// and now write it to the screen while we transfer next?
	EPD_OE_H

	// SPH->L start of next frame
	EPD_SPH_L
	// move data to shift register
	for (uint8_t i = 0; i < 200; i++) {
		i2c_send_0(pArray[i]);
		EPD_CL_H
		// DelayCycle(1);
		EPD_CL_L
	}
	// end of data
	EPD_SPH_H

	// one CL according t docu
	EPD_CL_H
	EPD_CL_L
	EPD_CL_H
	EPD_CL_L


	// what about gmode?
	// gate driver: CKV->L end of writing PREVIOUS frame to screen
	EPD_CKV_L
	// source driver: OE->L end of writing PREVIOUS frame to screen
	EPD_OE_L

	// needed?
	EPD_CL_H
	EPD_CL_L
	EPD_CL_H
	EPD_CL_L

	// CKV off pretty short?
	EPD_CKV_H
} // EPD_Send_Row_Data

void EPD_SkipRow(void){
	memset(line,0x00,200);
	EPD_send_row_data(line);
} // EPD_SkipRow

void EPD_Send_Row_Data_Slow(u8 * pArray, unsigned char timingA, unsigned char timingB){
	unsigned char i;

	// different mode .. now we controll timing for THIS line in THIS call
	// OE->H == write to screen .. hmm ..
	EPD_OE_H
	// send signal that we're about to send data
	EPD_SPH_L

	// actually send the data
	for (i = 0; i < 200; i++) {
		i2c_send_0(pArray[i]);
		EPD_CL_H
		// DelayCycle(1);
		EPD_CL_L
	}

	// sph->h end of latching
	EPD_SPH_H

	// one clock cycles needed here but .. hey
	EPD_CL_H
	EPD_CL_L
	EPD_CL_H
	EPD_CL_L

	// latch data .. again onlyvone needed ?
	EPD_LE_H
	EPD_CL_H
	EPD_CL_L
	EPD_CL_H
	EPD_CL_L

	EPD_LE_L
	EPD_CL_H
	EPD_CL_L
	EPD_CL_H
	EPD_CL_L

	// remember: OE has been active for a long time, so the source driver is waiting

	// CKV -> H will activate the GATE driver ... the longer we leave it like this the darker the pixel gets
	EPD_CKV_H
	DelayCycle(timingA); // important, while CKV on: write to screen
	EPD_CKV_L
	// make sure we have it low for some time, otherwise we have residual charge ?
	DelayCycle(timingB); // also important .. if too short: leaking

	// stop source driver, this frame is over
	EPD_OE_L

	// needed ?
	EPD_CL_H
	EPD_CL_L
	EPD_CL_H
	EPD_CL_L
} // EPD_Send_Row_Data_Slow

void EPD_Vclock_Quick(void){
	unsigned char i;

	for (i = 0; i < 2; i++) { // why two and why inverted ? is there a polarity pin? RL?
		EPD_CKV_L
		DelayCycle(20);
		EPD_CKV_H // or is it kind of don't care as long as we have some rising edges
		DelayCycle(20);
	}
}

void EPD_Start_Scan(void){
	EPD_SPV_H
	EPD_Vclock_Quick();
	EPD_SPV_L
	EPD_Vclock_Quick();
	EPD_SPV_H
	EPD_Vclock_Quick();
}

void EPD_Clear(void){
	unsigned short line, frame, i;

	for (frame = 0; frame < FRAME_INIT_LEN; frame++) {	// 33/21 byte aka
		EPD_Start_Scan();
		for (line = 0; line < 600; line++) {
			for (i = 0; i < 200; i++) line[i] = wave_init[frame];
			EPD_Send_Row_Data(line);
		}
		EPD_Send_Row_Data(line);
	}
}

void EPD_EncodeLine_Pic(u8 * new_pic, u8 frame){ // Encode data for grayscale image
	int i, j;
	unsigned char k, d;

	j = 0;
	for (i = 0; i < 200; i++) {
		d = 0;
		k = new_pic[j++];
		if ((k & 0x0F) > frame) d |= 0x10;
		if ((k >> 4) > frame) d |= 0x40;
		k = new_pic[j++];
		if ((k & 0x0F) > frame) d |= 0x01;
		if ((k >> 4) > frame) d |= 0x04;
		line[i] = d;
	}
}

void EPD_DispPic(){ // Display image in grayscale mode
	unsigned short frame;
	signed long line;
	unsigned long i;
	unsigned char * ptr;

	ptr = (unsigned char *) EPD_BG;

	for (frame = 0; frame < 15; frame++) {
		EPD_Start_Scan();
		for (i = 0; i < 200; i++) line[i] = 0x00; // 0x00 == no-action, why 70 line no action?
		for (line = 0; line < 70; line++) {
			EPD_Send_Row_Data_Slow(line, timA[frame], timB);
		}

		for (line = (530 - 1); line >= 0; line--) {
			EPD_EncodeLine_Pic(ptr + line * 400, frame);
			EPD_Send_Row_Data_Slow(line, timA[frame], timB);
		}

		EPD_Send_Row_Data(line); // needed? the slow shouldnt, or?
	}
}

// Clear image in monochrome mode
void EPD_ClearScr(unsigned int startLine, unsigned int lineCount){
	unsigned short frame;
	signed short line;
	unsigned long i;
	unsigned char * ptr;
	unsigned long skipBefore, skipAfter;

	ptr = EPD_FB;

	skipBefore = 600 - startLine - lineCount;
	skipAfter  = startLine;

	for (frame = 0; frame < clearCount; frame++) {
		EPD_Start_Scan();
		for (line = 0; line < skipBefore; line++) {
			EPD_EncodeLine_From(ptr);
			EPD_SkipRow();
		}
		for (line = (lineCount - 1); line >= 0; line--) {
			EPD_EncodeLine_From(ptr + (line + startLine) * 100);
			EPD_Send_Row_Data(line);
		}
		EPD_Send_Row_Data(line);
		for (line = 0; line < skipAfter; line++) {
			EPD_EncodeLine_From(ptr);
			EPD_SkipRow();
		}
		EPD_Send_Row_Data(line);
	}

	for (frame = 0; frame < 4; frame++) {
		EPD_Start_Scan();
		for (line = 0; line < skipBefore; line++) {
			EPD_SkipRow();
		}
		for (line = (lineCount - 1); line >= 0; line--) {
			for (i = 0; i < 200; i++) line[i] = 0xaa;
			EPD_Send_Row_Data(line);
		}
		EPD_Send_Row_Data(line);
		for (line = 0; line < skipAfter; line++) {
			;
			EPD_SkipRow();
		}
		EPD_Send_Row_Data(line);
	}
} // EPD_ClearScr
