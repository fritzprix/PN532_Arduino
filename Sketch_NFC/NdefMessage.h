/*
 * NdefMessage.h
 *
 *  Created on: 2013. 12. 27.
 *      Author: innocentevil
 */

#ifndef NDEFMESSAGE_H_
#define NDEFMESSAGE_H_

#include "NdefRecord.h"
#include "Arduino.h"


namespace NFC {
class NdefMessage;

/**
 * this class Simple Ndef Message Wrapper. class name make one think it has file like interface though, it has
 */
class NdefFile{
public:
	NdefFile(NdefMessage& msg);
	virtual ~NdefFile();

	/*
	 * NDEF File has NLEN Field in its entry though, the size returned doesn't take it.
	 *
	 *
	 * @return size of NDEF Message in byte
	 *
	 */
	uint32_t getSizeInByte();

	/*
	 * @param:rbuf byte buffer to be written
	 * @param:size
	 * @return data left to read in byte
	 */
	uint32_t read(uint8_t* rbuf,uint16_t size);

	/*
	 * @param:nmsg new NDEF Message
	 * @return index of Written NDEF Message
	 */
	uint32_t write(NdefMessage* nmsg);
        uint32_t write(const uint8_t* ndata,const uint32_t len);
	void seek(uint16_t offset);

	void print();
private:
	uint32_t size;
	uint8_t* fileimg;
	uint8_t* EoF;
	uint8_t* cPos;
};

class NdefMessage {
public:
	NdefMessage(NdefRecord** rcds,int size);
	NdefMessage();
	NdefMessage(const NdefMessage& org);
	virtual ~NdefMessage();
	uint32_t write(uint8_t* out);
	const NdefRecord* getRecord(uint8_t idx);
	/*
	 * @return sum of each length of record
	 */
	uint32_t getSizeInByte();
	uint8_t length();
private:
	int size;
	NdefRecord** rcds;
};

} /* namespace NFC */
#endif /* NDEFMESSAGE_H_ */

