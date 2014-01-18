/*
 * IsoDepTag.cpp
 *
 *  Created on: 2013. 12. 26.
 *      Author: innocentevil
 */

#include "IsoDepTag.h"

#define DBG 0

namespace NFC {

#define FID_CC (uint16_t) 0xE103
#define CMD_RATS (uint8_t) 0xE0

#define LOGE(x) {Serial.print(F("Error @ IsoDepTag.cpp : "));Serial.println(F(x));}
#define LOGEN(x,r) {Serial.print(F("Error @ IsoDepTag.cpp : "));Serial.print(F(x));Serial.println(r,HEX);}
#define LOGD(x) {Serial.print(F("Debug @ IsoDepTag.cpp : "));Serial.println(F(x));}
#define LOGDN(x,r) {Serial.print(F("Debug @ IsoDepTag.cpp : "));Serial.print(F(x));Serial.println(r,HEX);}
#define PRINT_ARRAY(l,x,s) {Serial.print("\n");Serial.print(F(l));for(int i = 0;i < s;i++){Serial.print("\t");Serial.print(x[i],HEX);}Serial.print("\n");}
#define MAX_TRY 5

#define IS_DATAFREE_PACKET(x) (x == APDU_CMD_READ_BINARY)

uint8_t IsoDepTag::MIFARE_PARAM[6] = { 0x04, 0x00, // ATQA : Mifare Classic 1K => Type 4 Tag
		0x01, 0x04, 0xF3, // NFCID1
		0x20 //SAK : ISO/IEC 14443-4 PICC
		};

uint8_t IsoDepTag::FELICA_PARAM[18] = { 0x01, 0xFE, 0x0F, 0xBB, 0xBA, 0xA6,
		0xC9, 0x89, //NFCID2
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //PAD
		0xFF, 0xFF //POL_RES
		};

uint8_t IsoDepTag::NFCID[10] = { 0x01, 0xFE, 0x0F, 0xBB, 0xBA, 0xA6, 0xC9, 0x89,
		0x00, 0x00 };

uint8_t IsoDepTag::DefaultDepAppImpl::CAP_CONTAINER_FILE[] = { 0x00, 0x0F, // length of cap container file
		0x20, // Version 2_0
		0x00, 0x7F, // Max. R-APDU Size : 127 byte
		0x00, 0x7F, // Max. C-APDU Size : 127 byte
		0x04, // Start of NDEF File Control TLV T = 0x04
		0x06, // TLV length = 0x06
		0xE1, 0x04, // NDEF File ID
		0x04, 0x00, // Max NDEF File size
		0x00, // Read Access possible
		0x00, // write aceess possible
		};

uint8_t IsoApdu::CMD_NDEF_APP_SELECT[7] = { 0xD2, 0x76, 0x00, 0x00, 0x85, 0x01,
		0x01 };

uint8_t IsoApdu::CMD_CAP_CONT_SELECT[2] = { 0xE1, 0x03 };

void IsoApdu::parse(uint8_t* buff, uint16_t len, IsoApduListener* listener) {
	uint8_t insType = *(uint8_t*) (buff + 3);
	if (IS_DATAFREE_PACKET(insType)) {
		IsoApdu capdu(buff + 2, len - 2, false);
#if DBG
		LOGD("Parsing Short type APDU Complete!");
#endif
		listener->onApduReceived(&capdu);
	} else {
		IsoApdu capdu(buff + 2, len - 2, true);
#if DBG
		LOGD("Parsing Normal type APDU Complete!");
#endif
		listener->onApduReceived(&capdu);
	}
}

IsoDepTag::IsoDepTag(NFC* hw) {
	ovrImpl = NULL;
	if (hw == NULL) {
		LOGE("NFC Hw not initialized");
		return;
	}
	ulhw = hw;
	uint8_t buf[255] = { 0, };
	//Initialize PN532 as a target which supports Iso-Dep Tag type (NFC Tag Type-4)
	/**
	 * To configure pn532 as a picc which support NFC Type Tag, PICC Emulation Mode "Must be" enabled
	 * otherwise it'll send error code when any command which is relevant to Picc Operation is received.
	 * @param : Auto ATR_RES | ISO_14443-4 Picc Emulation
	 *
	 */
	rcd = new NdefRecord*[5];
	rcd[0] = NdefRecord::createTextNdefRecord("This is driver of PN532 NFC IC", "en",NdefRecord::UTF8);
        rcd[1] = NdefRecord::createTextNdefRecord("Author : ldw123", "en",NdefRecord::UTF8);
        rcd[2] = NdefRecord::createTextNdefRecord("Version : 0.1", "en",NdefRecord::UTF8);
        rcd[3] = NdefRecord::createTextNdefRecord("Date : 2014.1.17", "en",NdefRecord::UTF8);
     //   rcd[4] = NdefRecord::createEmptyRecord();
	NdefMessage msg(rcd, 4);
	defaultImpl = new DefaultDepAppImpl(msg);
	LOGD("IsoDep Default App Initiated");
	if (IS_ERROR(ulhw->setParameter(1 << 2 | 1 << 5))) {
		LOGE("Fail to config Pn532 as PICC Target");
		return;
	}
	LOGD("Set Param Complete");
	if (IS_ERROR(ulhw->SAMConfiguration(0x01, 0xF, true))) {
		LOGE("PN532 fails to enter to normal state");
	}
	LOGD("Set SAM Configure");
}

IsoDepTag::~IsoDepTag() {
	delete defaultImpl;
	ulhw = NULL;
	if (ovrImpl != NULL) {
		delete ovrImpl;
	}
	if(rcd != NULL){
		delete[] rcd;
	}
}

bool IsoDepTag::sendAckApdu() {
	uint8_t Ack[] = { 0x90, 0x00 };
	PRINT_ARRAY("R-APDU Send : ", Ack, 2);
	if (IS_ERROR(ulhw->tgSetData(Ack, 2))) {
		return false;
	}
	return true;
}

uint8_t IsoDepTag::listenRATS() {
	NfcTargetConfig config(1 << 2 | 1 << 0, MIFARE_PARAM, FELICA_PARAM, NFCID);
	if (IS_ERROR(ulhw->tgInitAsTarget(&config, rxBuf))) {
		return 0xFF;
	}
	return rxBuf[2] == CMD_RATS ? rxBuf[3] : 0xFF;
}

bool IsoDepTag::startIsoDepTag() {
	uint8_t rats = listenRATS();
	if (rats == 0xFF) {
		LOGE("RATS(Request for Answer to select) is not recevied");
		return false;
	}
	if (ovrImpl != NULL) {
		ovrImpl->onInitiatorDetected(*this);
	} else {
		defaultImpl->onInitiatorDetected(*this);
	}
	uint16_t rxresult = 0;
	while (!IS_ERROR((rxresult = ulhw->tgGetData(rxBuf)))) {
		IsoApdu::parse(rxBuf, GET_VALUE(rxresult), this);
	}
	return true;
}

void IsoDepTag::setNdefMsg(NdefMessage& msg) {
	DefaultDepAppImpl* biapp = (DefaultDepAppImpl*) defaultImpl;
	biapp->setNdefMsg(msg);
}

void IsoDepTag::onApduReceived(const IsoApdu* apdu) {
	Serial.print("Apdu Received  :   ");
	uint16_t readresult = 0;
	switch (apdu->ins) {
	case APDU_CMD_SELECT_FILE:
		Serial.println(" File Select");
		if (onFileSelected(apdu)) {
			sendAckApdu();
		}
		break;
	case APDU_CMD_READ_BINARY:
		// read Binary command
		Serial.println(" Read Binary");
		readresult = onReadAccess(apdu->p1 << 8 | apdu->p2, apdu->elen, txBuf);
		Serial.print("Read Binary Result :  ");
		Serial.println(readresult, HEX);
		if (!IS_ERROR(readresult)) {
			LOGE("No Read Error");
			sendAckApduWithData(txBuf, GET_VALUE(readresult));
		}
		break;
	case APDU_CMD_UPDATE_BINARY:
		// update Binary command
		Serial.println(" Update Binary");
		if (onWriteAccess(apdu)) {
			sendAckApduWithData(NULL, 0);
		}
		break;

	}
}

bool IsoDepTag::onFileSelected(const IsoApdu* apdu) {
	LOGD("On File selected");
	if (apdu->isNdefAppSelect()) {
		return true;
	}
	if (apdu->getApduDataSize() == 2) {
		uint8_t* fid = apdu->getApduData();
		if (ovrImpl != NULL) {
			return ovrImpl->onNdefFileSelected(fid[0] << 8 | fid[1], *this);
		} else {
			return defaultImpl->onNdefFileSelected(fid[0] << 8 | fid[1], *this);
		}
	}
	return false;
}

uint16_t IsoDepTag::onReadAccess(uint16_t offset, uint8_t el,
		uint8_t* resBuff) {
	if (ovrImpl != NULL) {
		return NFC_SUCCESS | ovrImpl->onNdefRead(offset, el, resBuff);
	} else {
		return defaultImpl->onNdefRead(offset, el, resBuff);
	}
}

bool IsoDepTag::onWriteAccess(const IsoApdu* apdu) {
	uint8_t* len = apdu->getApduData();
	if (ovrImpl != NULL) {
		return ovrImpl->onNdefWrite(apdu->p1 << 8 | apdu->p2,
				len[0] << 8 | len[1], len + 2);
	} else {
		return defaultImpl->onNdefWrite(apdu->p1 << 8 | apdu->p2,
				len[0] << 8 | len[1], len + 2);
	}
}

bool IsoDepTag::sendAckApduWithData(uint8_t* data, uint16_t len) {
	if (len > 0) {
		memcpy(txBuf, data, len);
	}
	txBuf[len] = 0x90;
	txBuf[len + 1] = 0x00;
	if (IS_ERROR(ulhw->tgSetData(txBuf,len + 2))) {
		return false;
	}
	PRINT_ARRAY("Send R-APDU : ", txBuf, len + 2);
	return true;
}

IsoApdu::IsoApdu(uint8_t* apdufrm, uint8_t flen, bool hasData) {
	Apdu_Header* header = (Apdu_Header*) ((apdufrm));
	cla = header->cla;
	ins = header->ins;
	p1 = header->pa1;
	p2 = header->pa2;
	if (hasData) {
		if (apdufrm[4] != 0) {
			if ((flen - 4) == (apdufrm[4] + 1)) {
				//No Le field
				datlen = apdufrm[4];
				data = new uint8_t[datlen];
				memcpy(data, apdufrm + 5, datlen);
				elen = 0;
			} else if ((flen - 4) > (apdufrm[4] + 1)) {
				//Le field
				datlen = apdufrm[4];
				data = new uint8_t[datlen];
				memcpy(data, apdufrm + 5, datlen);
				elen = apdufrm[4 + datlen];
			} else {
				LOGE("Frame Size is too small");
			}
		} else {
			//No Data
			datlen = 0;
			data = NULL;
			elen = 0;
		}
	} else {
		datlen = 0;
		data = NULL;
		if (flen > 4) {
			elen = apdufrm[4];
		} else {
			elen = 0;
		}
	}
}

IsoApdu::~IsoApdu() {
	if (data != NULL)
		delete[] data;
}

uint8_t* IsoApdu::getApduData() const {
	return data;
}

uint16_t IsoApdu::getApduDataSize() const {
	return datlen;
}

bool IsoApdu::isNdefAppSelect() const {
	return memcmp(data, CMD_NDEF_APP_SELECT, datlen) == 0;
}

bool IsoApdu::isCCSelect() const {
	return memcmp(data, CMD_CAP_CONT_SELECT, datlen) == 0;
}

IsoDepApp::IsoDepApp() {
}

IsoDepApp::~IsoDepApp() {
}

IsoDepTag::DefaultDepAppImpl::DefaultDepAppImpl(NdefMessage& msg) {
	this->nFile = new NdefFile(msg);
}

IsoDepTag::DefaultDepAppImpl::~DefaultDepAppImpl() {
	if (this->nFile != NULL) {
		delete nFile;
	}
}

uint32_t IsoDepTag::DefaultDepAppImpl::setNdefMsg(NdefMessage& msg) {
	this->nFile->write(&msg);
	return 0;
}

void IsoDepTag::DefaultDepAppImpl::onInitiatorDetected(IsoDepTag& tag) {
	LOGD("Initiator Detected");
}

bool IsoDepTag::DefaultDepAppImpl::onNdefFileSelected(uint16_t fid,
		IsoDepTag& tag) {
	//Nothing to do
	this->fid = fid;
#if DBG
	LOGDN("Selected NDEF File id : ", fid);
#endif
	return true;

}

uint16_t IsoDepTag::DefaultDepAppImpl::onNdefRead(uint16_t offset, uint8_t el,
		uint8_t* outbuff) {
#if DBG
	LOGDN("NDEF read Offset  : ", offset);
	LOGDN("NDEF read Size    : ", el);
	LOGDN("NDEF read File id : ", fid);
#endif
	if (fid == FID_CC) {
		memcpy(outbuff, CAP_CONTAINER_FILE, el);
		return NFC_SUCCESS | el;
	} else {
		if (nFile) {
			nFile->seek(offset);
			return NFC_SUCCESS | nFile->read(outbuff, el);
		} else {
			return NFC_ERROR;
		}
	}
	return NFC_ERROR;
}

bool IsoDepTag::DefaultDepAppImpl::onNdefWrite(uint16_t offset, uint16_t len,
		uint8_t* data) {
#if DBG
	LOGDN("NDEF write Offset  : ",offset);
	LOGDN("NDEF write length  : ",len);
	LOGDN("NDEF write File id : ",fid);
	PRINT_ARRAY("NDEF written data : ",data,len);
#endif
	return true;
}

uint8_t IsoDepTag::DefaultDepAppImpl::getMsgLen() {
	return nFile->getSizeInByte();
}

/* namespace NFC */

}
/* namespace NFC */
