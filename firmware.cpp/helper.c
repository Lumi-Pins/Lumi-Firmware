#include <Arduino.h>

/*
 * This function flashes on-board LED at 0.5 Hz
 * Parameter: None
 * Return: None
 */
void longf(){   
	digitalWrite(LED_BUILTIN, LOW);
	delay(1000);
	digitalWrite(LED_BUILTIN, HIGH);
	delay(1000);
	digitalWrite(LED_BUILTIN, LOW);

}

/*
 * This function flashes on-board LED at 5 Hz
 * Parameter: None
 * Return: None
 */
void shortf(){
	digitalWrite(LED_BUILTIN, LOW);
	delay(100);
	digitalWrite(LED_BUILTIN, HIGH);
	delay(100);
	digitalWrite(LED_BUILTIN, LOW);

}

/*
 * This function computes fletcher 16 checksum
 * Parameter: a pointer to the data to be calculated, length of the data
 * Return: the 16-bit fletcher16 checksum
 */
uint16_t  fletcher16(unsigned char *data, uint16_t len){
	uint16_t sum1 = 0;
	uint16_t sum2 = 0;
	for(uint16_t i =0; i < len; ++i){
		sum1 = (sum1 + data[i]) % 255;
		sum2 = (sum2 + sum1) % 255;
	}
	return (sum2 << 8)| sum1;
}

/*
 * This function turns 2 bytes of binary data to a uint16_t
 * Parameter: A pointer to binary data
 * Return: the uint16_t result
 */
uint16_t CHARA2SHORT(char* src){
	return (uint16_t)(
			((uint8_t)src[0]) << 8 |
			((uint8_t)src[1]));
}
