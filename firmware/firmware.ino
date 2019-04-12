/*
 * File: firmware.ino
 * Created: 3/11/19
 * Authors: Zelin Liu
 * Content: Main file for Arduino firmware
 *
 *
 * */


#include "helper.c"
#include <Adafruit_NeoPixel.h>
#define PIN_LED 7

// this is used to identify the communication
#define HEADER_BYTES 37740 //0x936C
#define HEADER_BYTES_LOW_BYTE 0x6C 
#define HEADER_BYTES_HIGH_BYTE 0x93

#define HEADER_SIZE 6

#define ARDUINO_SERIAL_RX_BUFFER_SIZE 64

#define TIMEOUT_TIME_0 1000

uint16_t LED_index_array[]={
		0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,
		29,  28,  27,  26,  25,  24,  23,  22,  21,  20,  19,  18,  17,  16,  15,
		30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,
		59,  58,  57,  56,  55,  54,  53,  52,  51,  50,  49,  48,  47,  46,  45,
		60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,
		89,  88,  87,  86,  85,  84,  83,  82,  81,  80,  79,  78,  77,  76,  75,
		104, 103, 102, 101, 100, 99,  98,  97,  96,  95,  94,  93,  92,  91,  90,
		105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
		134, 133, 132, 131, 130, 129, 128, 127, 126, 125, 124, 123, 122, 121, 120,
		135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149,
		164, 163, 162, 161, 160, 159, 158, 157, 156, 155, 154, 153, 152, 151, 150,
		165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
		194, 193, 192, 191, 190, 189, 188, 187, 186, 185, 184, 183, 182, 181, 180,
		195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209,
		224, 223, 222, 221, 220, 219, 218, 217, 216, 215, 214, 213, 212, 211, 210

};




/*
 * This function cleans receive buffer for Serial
 * Parameter: None
 * Return: None
 */
void CleanInBuffer(){
	while(Serial.available()>=0){
		Serial.read();
	}
}

/*
 * This function computes the fletcher16 check sum of given data, then sends it over serial.
 * Parameter: a pointer to the data, length of the data
 * Return: true if 2 bytes of data is sent, false otherwise
 */
bool ReturnChecksum(unsigned char *data, uint16_t len){
	uint16_t checksum = fletcher16(data, len);
	char checksum_array[2];
	checksum_array[0] = (checksum >> 8) & 0xFF;
	checksum_array[1] = checksum & 0xFF;
	return (2 == Serial.write(checksum_array,2));
}	

/*
 * This function waits until either time out or until there is enough bytes in receive buffer, if there enough then will read into buffer
 * Parameter: a pointer to the buffer, number of bytes to be read, timeout in miliseconds
 * Return: true n bytes of data is read into buffer
 */
bool ReadNBytes(char* buff, uint8_t n, uint32_t timeout){
	uint32_t timer_0 = millis();
	while(Serial.available()<n){
		//		if(millis() - timer_0 > timeout){return false;}
	} // if not able to receive enough for header
	for(uint8_t i = 0; i < n; i++){
		buff[i] = Serial.read();
	}
	return true;
}

/*
 * This function reads header
 * Parameter: a pointer to total_size and batch_size as they are the output
 * Return: true if valid header size and header ID byte are encoutnered, and a checksum is returned succesfully
 */
bool ReadHeader(uint16_t* total_size, uint16_t* batch_size){
	char* header = (char*)malloc(HEADER_SIZE*sizeof(char));
	if(!ReadNBytes(header, HEADER_SIZE, TIMEOUT_TIME_0)){
		free(header);
		return false;
	} // What to do if read failed
	if(CHARA2SHORT(header) != HEADER_BYTES){
		free(header);
		return false;
	} // What if ID byte is not good?
	*total_size = CHARA2SHORT(header+2);
	*batch_size = CHARA2SHORT(header+4);
	if(!ReturnChecksum(header, HEADER_SIZE)){
		free(header);
		return false;
	}
	free(header);
	return true;
}

/*
 * This function reads body
 * Parameter: total number of bytes of the transmission, size of each batch, pointer to destination of thee read
 * Return: true on success and false on failure
 */
bool ReadBody(uint16_t total_size, uint16_t batch_size, char** dst){
	char* batch = (char*) malloc((batch_size+2)*sizeof(char));

	uint16_t total_batch_num = 1 + ((total_size -  1) / batch_size); // ceiling division
	for(uint16_t j = 0; j< total_batch_num;j++){
		if(!ReadNBytes(batch, batch_size+2, TIMEOUT_TIME_0)){
			free(batch);
			return false;
		}
		uint16_t batch_num = CHARA2SHORT(batch);
		if(batch_num == !j){
			free(batch);
			return false;}
		memcpy(*dst+batch_size*j, batch+2, batch_size);
		if(!ReturnChecksum((unsigned char*)batch, batch_size+2)){

			free(batch);
			return false;
		}
	}
	free(batch);
	return true;
}

/*
 * This function reads exchanges final checksum, calculate cs A, receives cs B, return cs of A, then compare A to B
 * Parameter: a pointer to the file, length of the file
 * Return: true un success, false on failure
 */
bool FinalChecksum(char ** dst, uint16_t len){
	uint16_t self_checksum = fletcher16(*dst, len);
	char self_checksum_array[2];
	self_checksum_array[0] = (self_checksum >> 8) & 0xFF;
	self_checksum_array[1] = self_checksum & 0xFF;
	char * received_checksum_a = (char*) malloc(sizeof(char)*2);

	if(!ReadNBytes(received_checksum_a, 2, TIMEOUT_TIME_0)){
		free(received_checksum_a); 
		return false;
	}

	if(!ReturnChecksum((unsigned char*)self_checksum_array, 2)){
		free(received_checksum_a);
		return false;
	}
	uint16_t received_checksum = CHARA2SHORT(received_checksum_a);
	free(received_checksum_a);
	return (self_checksum == received_checksum);
}

/*
 * This function wraps all the other functions together, you can simply call this function to do the handshake and the transmission
 * Parameter: a pointer to the destination of the read, a pointer to a total size variable which is to be set by this function
 * Return: true un success, false on failure
 * Note: this fucntion allocates the pointer, please remember to free it later OK?
 */
bool GetBoardData(char** dst, uint16_t* total_size){ // remember to free dst after you have used it. if returned true
	uint16_t batch_size;

	if(!ReadHeader(total_size, &batch_size)){
		return false;
	}

	*dst = (char*)realloc(*dst, sizeof(char) * (*total_size));

	if(!ReadBody(*total_size, batch_size, dst)){
		free(*dst);
		return false;
	}

	if(!FinalChecksum(dst, *total_size)){
		free(*dst);
		return false;
	}

	return true;
}


/*
 * This function shows LEDs using color chosen by switches
 * Parameter: number of LEDs, pointer to switch array, pointer to board color data
 * Return: None
 */
void showLED(uint16_t num_led, bool* sw, char* board_array){
	uint32_t half= num_led*4;
	for(uint16_t i = 0; i < num_led; i++){
		if(!sw[i]){
			strip.setPixelColor(LED_index_array[i], (uint8_t)board_array[i*4+0],(uint8_t)board_array[i*4+1],(uint8_t)board_array[i*4+2]);
		}else{
			strip.setPixelColor(LED_index_array[i], (uint8_t)board_array[half+i*4+0],(uint8_t)board_array[half+i*4+1],(uint8_t)board_array[half+i*4+2]);
		}
	}
	strip.show();
}



/*
 * This function is the main function of the execution
 * Parameter: None
 * Return: None
 */
void setup() {

	Serial.begin(9600);
	strip.begin();
	pinMode(LED_BUILTIN, OUTPUT);
	bool* switches;

	uint16_t total_size;
	char * board_array;

	while(!GetBoardData(&board_array, &total_size)){} // if not getting data then do nothing
	uint16_t num_led = total_size/8;
	switches = (bool*)realloc(switches, sizeof(bool) * (num_led)); // allocate switches
	Adafruit_NeoPixel strip = Adafruit_NeoPixel(num_led, PIN_LED, NEO_GRB + NEO_KHZ800);
	while(true){
		if(Serial.available() >= HEADER_SIZE){
			if(GetBoardData(&board_array, &total_size)){
				num_led = total_size/8;
				switches = (bool*)realloc(switches, sizeof(bool) * (num_led)); // allocate switches
				strip = Adafruit_NeoPixel(num_led, PIN_LED, NEO_GRB + NEO_KHZ800);
			}
		}
		ReadSwitches(switches);
		showLED(num_led, switches, board_array);
		delay(113);
	}
}

void loop() {

}
