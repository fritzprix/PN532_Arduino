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
#define PRINT_ARRAY(l,x,s) {Serial.print("\n");Serial.print(F(l));for(int i = 0;i < s;i++){Serial.print("\t");Serial.print((char)x[i]);Serial.print(" (");Serial.print(x[i],HEX);Serial.print(")");}Serial.print("\n");}
#define MAX_TRY 5

#define GET_RMODE(x) (x & 0xFF00)
#define GET_WMODE(x) (x & 0xFF)

#define IS_DATAFREE_PACKET(x) (x == APDU_CMD_READ_BINARY)
/* 
    MIFARE Parameter for initiating PN532 IC as PICC NFC TAG Type 4
	
 */
uint8_t IsoDepTag::MIFARE_PARAM[6] = { 0x04, 0x00, // ATQA : Mifare Classic 1K => Type 4 Tag
		0x01, 0x04, 0xF3, // NFCID1
		0x20 //SAK : ISO/IEC 14443-4 PICC
		};
/*   
    Felica Parameter for initiating PN532 IC as Target
	None of values is actually used,so any change of value doesn't affect operation of Tag at all	
 */
uint8_t IsoDepTag::FELICA_PARAM[18] = { 0x01, 0xFE, 0x0F, 0xBB, 0xBA, 0xA6,
		0xC9, 0x89, //NFCID2
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //PAD
		0xFF, 0xFF //POL_RES
		};

/*
	  NFCID for initiating PN532 IC as Target
	  It also seems not to be used in Tag Type 4 operation
*/
uint8_t IsoDepTag::NFCID[10] = { 0x01, 0xFE, 0x0F, 0xBB, 0xBA, 0xA6, 0xC9, 0x89,
		0x00, 0x00 };

/*
      Capability Container File 
	  -> this is requested from reader before read/write NDEF Message Content.
	     Max. NDEF File Size, Read/Write Access Mode will be overwritten by values returned 
		 by IsoDepApp Implementation
	  @ref : IsoDepApp::getMaxFileSize(void) / IsoDepApp::getAccessMode(void) = 0;
		 
*/
uint8_t IsoDepTag::CAP_CONTAINER_FILE[] = {
    0x00, 0x0F, // length of cap container file
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



/*   
    NDEF Application Select Pattern which is sent by NFC Tag Reader
    last 2 bytes are version modifier, 0x1 0x1 is indicate that current session is supporting Tag type 4 version 2.0
*/
uint8_t IsoApdu::CMD_NDEF_APP_SELECT[7] = { 0xD2, 0x76, 0x00, 0x00, 0x85, 0x01,
		0x01 };
/*
	File ID for Capability Container	
*/
uint8_t IsoApdu::CMD_CAP_CONT_SELECT[2] = { 0xE1, 0x03 };


/*
    Parses raw bytes array
*/
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


/*  
    Construct IsoDepTag 
	@param hw : Instantiated NFC 
*/
IsoDepTag::IsoDepTag(NFC* hw) {
	ovrImpl = NULL;
	if (hw == NULL) {
		LOGE("NFC H/W not initialized");
		return;
	}
	ulhw = hw;
	 /*Generate NDEF Records for default IsoDepApp Implementation*/
    NdefRecord* rcds[] = {NdefRecord::createTextNdefRecord("Date : 2014.1.17", "en",NdefRecord::UTF8),NdefRecord::createAndroidApplicationRecord("com.example.nfcreadwrite")};
    NdefMessage msg(rcds, 2);
	 /*Construct Default Implementation of IsoDepApp*/
	defaultImpl = new DefaultDepAppImpl(msg);

	/**
	 * To configure pn532 as a picc which support NFC Type Tag, PICC Emulation Mode "Must be" enabled
	 * otherwise it'll send error code when any command which is relevant to Picc Operation is received.
	 * @param : Auto ATR_RES | ISO_14443-4 Picc Emulation
	 *
	 */
	if (IS_ERROR(ulhw->setParameter(1 << 2 | 1 << 5))) {
		LOGE("Fail to config Pn532 as PICC Target");
		return;
	}
#if DBG 
        LOGD("Parameter Configured");
#endif 
	if (IS_ERROR(ulhw->SAMConfiguration(0x01, 0xF, true))) {
		LOGE("PN532 fails to enter to normal state");
	}
#if DBG
        LOGD("Configuration of SAM is done");
#endif 
}

/*
    Destructor of IsoDepTag
*/
IsoDepTag::~IsoDepTag() {
	delete defaultImpl;
	ulhw = NULL;
	if (ovrImpl != NULL) {
		delete ovrImpl;
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
		LOGE("RATS(Request for Answer to select) is not received");
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
#if DBG
        LOGD("C-APDU Received from Reader");
#endif
	uint16_t readresult = 0;
	switch (apdu->ins) {
	case APDU_CMD_SELECT_FILE:
	    #if DBG
		LOGD("TYPE : Select File");
		#endif
	    if (onFileSelected(apdu)) {
			sendAckApduWithData(NULL, 0);
		}
		break;
	case APDU_CMD_READ_BINARY:
	    #if DBG
		LOGD("TYPE : Read Binary");
		#endif
		// read Binary command
		readresult = onReadAccess(apdu->p1 << 8 | apdu->p2, apdu->elen, txBuf);
		if (!IS_ERROR(readresult)) {
#if DBG
			LOGD("Binary Read request is handled successfully");
#endif
			sendAckApduWithData(txBuf, GET_VALUE(readresult));
		}
		break;
	case APDU_CMD_UPDATE_BINARY:
		// update Binary command
		#if DBG
		LOGD("TYPE :  Binary Update ");
		#endif
		if (onWriteAccess(apdu)) {
			sendAckApduWithData(NULL, 0);
		}
		break;

	}
}

bool IsoDepTag::onFileSelected(const IsoApdu* apdu) {
	if (apdu->isNdefAppSelect()) {
		return true;
	}
	if (apdu->getApduDataSize() == 2) {
            uint8_t* data = apdu->getApduData();
            fid = data[0] << 8 | data[1];
			#if DBG
            LOGDN("FID : ",fid);
			#endif
            return true;
	}
	return false;
}

uint16_t IsoDepTag::onReadAccess(uint16_t offset, uint8_t el,
		uint8_t* resBuff) {
		    if(fid == FID_CC){
                    memcpy(resBuff,IsoDepTag::CAP_CONTAINER_FILE,0xF - 4);
                    uint16_t mxSize = defaultImpl != NULL? defaultImpl->getMaxFileSize() : ovrImpl->getMaxFileSize();
                    uint16_t mode = defaultImpl != NULL? defaultImpl->getAccessMode() : ovrImpl->getAccessMode();
                    resBuff[0xF - 4] = mxSize >> 8;
                    resBuff[0xF - 3] = mxSize & 0xFF;
                    resBuff[0xF - 2] = GET_RMODE(mode) == MODE_READ_PUBLIC? 0 : 1;
                    resBuff[0xF - 1] = GET_WMODE(mode) == MODE_WRITE_PUBLIC? 0 : 1;
#if DBG
                    LOGDN("READ Mode : ",GET_RMODE(mode));
                    LOGDN("READ Public : ",MODE_READ_PUBLIC);
                    PRINT_ARRAY("CC File : ",resBuff,0xF);
#endif
                    return NFC_SUCCESS | 0xF;
		    }else{
		        if (ovrImpl != NULL) {
                        return NFC_SUCCESS | ovrImpl->onNdefRead(offset, el, resBuff);
                } else {
                        return defaultImpl->onNdefRead(offset, el, resBuff);
                }
		    }
}

bool IsoDepTag::onWriteAccess(const IsoApdu* apdu) {
	uint8_t* data = apdu->getApduData();
	if (ovrImpl != NULL) {
		return ovrImpl->onNdefWrite(apdu->p1 << 8 | apdu->p2,
				apdu->getApduDataSize(), data);
	} else {
		return defaultImpl->onNdefWrite(apdu->p1 << 8 | apdu->p2,
				apdu->getApduDataSize(), data);
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
	#if DBG
	PRINT_ARRAY("Send R-APDU : ", txBuf, len + 2);
	#endif
	return true;
}

IsoApdu::IsoApdu(uint8_t* apdufrm, uint8_t flen, bool hasData) {
        data = NULL;
        PRINT_ARRAY("C-APDU : ",apdufrm,flen);
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
// No op.
}

IsoDepApp::~IsoDepApp() {
// nothing to free
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
	return this->nFile->write(&msg);
}

void IsoDepTag::DefaultDepAppImpl::onInitiatorDetected(IsoDepTag& tag) {
	LOGD("Initiator Detected");
}

uint16_t IsoDepTag::DefaultDepAppImpl::getMaxFileSize(void){
    return 0x0400;
}

uint16_t IsoDepTag::DefaultDepAppImpl::getAccessMode(void){
    return MODE_READ_PUBLIC | MODE_WRITE_PUBLIC;
}

uint16_t IsoDepTag::DefaultDepAppImpl::onNdefRead(uint16_t offset, uint8_t el,
		uint8_t* outbuff) {
#if DBG
	LOGDN("NDEF read Offset  : ", offset);
	LOGDN("NDEF read Size    : ", el);
#endif
    if (nFile) {
		nFile->seek(offset);
		return NFC_SUCCESS | nFile->read(outbuff, el);
	} else {
		return NFC_ERROR;
	}
	return NFC_ERROR;
}

bool IsoDepTag::DefaultDepAppImpl::onNdefWrite(uint16_t offset, uint16_t len,
		uint8_t* data) {
#if DBG
	LOGDN("NDEF write Offset  : ",offset);
	LOGDN("NDEF write length  : ",len);
	PRINT_ARRAY("NDEF written data : ",data,len);
#endif  
        if(len > 2){
          //NdefRecord* rxRcd = NdefRecord::parse(data + 2);
          //NdefMessage nmsg(&rxRcd,1);
          uint32_t wrsize =   nFile->write(data + 2,len - 2);
#if DBG   
          LOGDN("Written Size ",wrsize);
#endif
        }
	return true;
}


/* namespace NFC */

}
/* namespace NFC */
