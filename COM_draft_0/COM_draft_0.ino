// **NOTE**: does not support batch_size not a divisor of total_size...yet
// TODO:: implement timeout from both sides

#include "helper.c"

#define HEADER_BYTES 37740 //0x936C
#define HEADER_BYTES_LOW_BYTE 0x6C 
#define HEADER_BYTES_HIGH_BYTE 0x93

#define FINISHER_BYTES 51555 //0xC963
#define FINISHER_BYTES_LOW_BYTE 0x63 
#define FINISHER_BYTES_HIGH_BYTE 0xC9

#define HEADER_SIZE 6

#define ARDUINO_SERIAL_RX_BUFFER_SIZE 64

//#define ATTEMPTS_HEADER 1
//#define ATTEMPTS_BODY 3
//#define ATTEMPTS_FINSHER 3

#define TIMEOUT_TIME_0 1000




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
 * Parameter: 
 * Return:
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
 * Parameter: 
 * Return:
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


bool GetBoardData(char** dst, uint16_t* total_size){ // remember to free dst after you have used it. if returned true
	uint16_t batch_size;
	
	if(!ReadHeader(total_size, &batch_size)){
		return false;
	}
	
	*dst = (char*)malloc(sizeof(char) * (*total_size));

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






void setup() {
	Serial.begin(9600);
	pinMode(LED_BUILTIN, OUTPUT);

	uint16_t total_size;
  	char * board_array;

	while(Serial.available()<=0){}


    if(GetBoardData(&board_array, &total_size)){
        longf();
        longf();
        longf();
        longf();
        longf();
        longf();
        longf();
    }else{
      shortf();
      shortf();
      shortf();shortf();
      shortf();
      shortf();shortf();shortf();shortf();shortf();shortf();
      }

	while(true){

}}
