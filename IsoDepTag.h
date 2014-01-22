/*
 * IsoDepTag.h
 *
 *  Created on: 2013. 12. 26.
 *      Author: innocentevil
 */

#ifndef ISODEPTAG_H_
#define ISODEPTAG_H_

#include "NFC.h"
#include "NdefMessage.h"

namespace NFC {

#define TX_BUFFER_SIZE (uint16_t) 0x1FF
#define RX_BUFFER_SIZE (uint16_t) 0x1FF

#define APDU_CMD_READ_BINARY (uint8_t) 0xB0
#define APDU_CMD_WRITE_BINARY (uint8_t) 0xD0
#define APDU_CMD_UPDATE_BINARY (uint8_t) 0xD6
#define APDU_CMD_ERASE_BINARY (uint8_t) 0x0E
#define APDU_CMD_READ_RECORD (uint8_t) 0xB2
#define APDU_CMD_WRITE_RECORD (uint8_t) 0xD2
#define APDU_CMD_APPEND_RECORD (uint8_t) 0xE2
#define APDU_CMD_UPDATE_RECORD (uint8_t) 0xDC
#define APDU_CMD_GET_DATA (uint8_t) 0xCA
#define APDU_CMD_PUT_DATA (uint8_t) 0xDA
#define APDU_CMD_SELECT_FILE (uint8_t) 0xA4
#define APDU_CMD_VERIFY (uint8_t) 0x20

#define MODE_READ_PUBLIC 0xF000
#define MODE_READ_PRIVATE 0x0F00
#define MODE_WRITE_PUBLIC 0x00F0
#define MODE_WRITE_PRIVATE 0x000F

class IsoApdu {
public:
	class IsoApduListener {
	public:
		virtual void onApduReceived(const IsoApdu* cmd) = 0;
	};
	IsoApdu(uint8_t* apdufrm, uint8_t flen, bool hasData);
	virtual ~IsoApdu();
	uint8_t* getApduData() const;
	uint16_t getApduDataSize() const;
	bool isNdefAppSelect() const;
	bool isCCSelect() const;

	static void parse(uint8_t* buff, uint16_t len, IsoApduListener* listener);
	uint8_t cla;
	uint8_t ins;
	uint8_t p1;
	uint8_t p2;
	uint8_t elen;

private:
	static uint8_t CMD_NDEF_APP_SELECT[7];
	static uint8_t CMD_CAP_CONT_SELECT[2];

	uint8_t datlen;
	uint8_t* data;
	typedef struct _apdu_head {
		uint8_t cla;
		uint8_t ins;
		uint8_t pa1;
		uint8_t pa2;
	} Apdu_Header;
};
class IsoDepTag;
class IsoDepApp {
public:
	IsoDepApp();
	virtual ~IsoDepApp();
	virtual void onInitiatorDetected(IsoDepTag& tag) = 0;
	virtual uint16_t onNdefRead(uint16_t offset, uint8_t el,
			uint8_t* response) = 0;
	virtual bool onNdefWrite(uint16_t offset, uint16_t len, uint8_t* data) = 0;
	virtual uint16_t getMaxFileSize(void) = 0;
	virtual uint16_t getAccessMode(void) = 0;

};

class IsoDepTag: public IsoApdu::IsoApduListener {
public:
	IsoDepTag(NFC* nfcHw);
	virtual ~IsoDepTag();

	/**
	 *
	 */
	void setNdefMsg(NdefMessage& msg);
	bool startIsoDepTag();
	void onApduReceived(const IsoApdu* apdu);


private:
    static uint8_t CAP_CONTAINER_FILE[];
	class DefaultDepAppImpl: public IsoDepApp {
	public:
		DefaultDepAppImpl(NdefMessage& msg);
		virtual ~DefaultDepAppImpl();
		uint32_t setNdefMsg(NdefMessage& msg);
		void onInitiatorDetected(IsoDepTag& tag);
		uint16_t onNdefRead(uint16_t offset, uint8_t el, uint8_t* response);
		bool onNdefWrite(uint16_t offset, uint16_t len, uint8_t* data);
		uint16_t getMaxFileSize(void);
		uint16_t getAccessMode(void);
	private:
		NdefFile* nFile;
	};
        NdefRecord** rcd;
	NFC* ulhw;
	IsoDepApp* defaultImpl;
	IsoDepApp* ovrImpl;
	static uint8_t MIFARE_PARAM[6];
	static uint8_t FELICA_PARAM[18];
	static uint8_t NFCID[10];

	uint8_t txBuf[TX_BUFFER_SIZE];
	uint8_t rxBuf[RX_BUFFER_SIZE];
	uint16_t fid;

	bool onFileSelected(const IsoApdu* apdu);
	uint16_t onReadAccess(uint16_t offset, uint8_t el, uint8_t* resBuff);
	bool onWriteAccess(const IsoApdu* apdu);
	bool sendAckApduWithData(uint8_t* data, uint16_t len);
	bool sendAckApdu();
	uint8_t listenRATS();
};

} /* namespace NFC */
#endif /* ISODEPTAG_H_ */
