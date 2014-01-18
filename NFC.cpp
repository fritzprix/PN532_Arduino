/*
 * NFC.cpp
 *
 *  Created on: 2013. 12. 14.
 *      Author: innocentevil
 */
#include "NFC.h"
#include <SPI.h>
#include "ShiftBuffer.h"
#include <mem.h>

#define DBG 0

namespace NFC {

uint8_t NFC::ACK_FRAME[] = { 0x00, 0x00, 0xff, 0x00, 0xff, 0x00 };
uint8_t NFC::NACK_FRAME[] = { 0x00, 0x00, 0xff, 0xff, 0x00, 0x00 };
uint8_t NFC::PREAMBLE[] = { 0x00, 0xff };
uint8_t NFC::POSTAMBLE[] = { 0x00 };


uint8_t NFC::PARAM_MIFARE[6] = { 0x44, 0x00, // ATQA : MifareClassic 1K
		0x01, 0x41, 0x51, 0x20 };

uint8_t NFC::PARAM_FELICA[18] = { 0x01, 0xFE, 0x0F, 0xBB, 0xBA, 0xA6, 0xC9,
		0x89, // POL_RES
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF };

uint8_t NFC::NFCID[10] = { 0x01, 0xFE, 0x0F, 0xBB, 0xBA, 0xA6, 0xC9, 0x89,
		0x00, 0x00 };

#define NFC_BUFFER_SIZE 0x1FF
#define NFC_VERSION (uint32_t) (0x32 << 24 | 1 << 16 | 6 << 8 | 7)

#define RESULT_SUCCESS (uint8_t)1
#define RESULT_FAILURE (uint8_t)0

#define NFC_ACK (uint8_t) 2
#define NFC_NACK (uint8_t) 1

#define NORMAL_PACKET_HEADER_LEN 6
#define NORMAL_PACKET_FOOTER_LEN 2

#define NFC_DATA_WRITE (uint8_t) 0x01
#define NFC_STATUS_READ (uint8_t) 0x02
#define NFC_DATA_READ (uint8_t) 0x03

#define PN532_PREAMBLE (uint8_t) 0
#define PN532_POSTAMBLE (uint8_t) 0
#define TFI_HOST_TO_PN532 (uint8_t) 0xD4
#define TFI_PN532_TO_HOST (uint8_t) 0xD5
#define PN532_NORMAL_STARTCODE0 (uint8_t) 0x00
#define PN532_NORMAL_STARTCODE1 (uint8_t) 0xFF

#define PN532_NORMAL_HEADER_SIZE (uint8_t) 6
#define PN532_NORMAL_FOOTER_SIZE (uint8_t) 2

/***
 * Diagnose Test Type
 *
 */
#define DIAG_COMMLINE (uint8_t)0x00
#define DIAG_ROM (uint8_t) 0x01
#define DIAG_RAM (uint8_t) 0x02
#define DIAG_POLL (uint8_t) 0x04
#define DIAG_ECHO (uint8_t) 0x05
#define DIAG_ATR (uint8_t) 0x06
#define DIAG_ANTENNA (uint8_t) 0x07

/***
 * Command List
 *
 */

#define CMD_PN532_GETVERSION (uint8_t) 0x02
#define CMD_PN532_DIAGNOSE (uint8_t) 0x00
#define CMD_PN532_GETSTATUS (uint8_t) 0x04
#define CMD_PN532_RDREG (uint8_t) 0x06
#define CMD_PN532_WRREG (uint8_t) 0x08
#define CMD_PN532_SET_BR (uint8_t) 0x10
#define CMD_PN532_SET_PARM (uint8_t) 0x12
#define CMD_PN532_SAM_CONF (uint8_t) 0x14
#define CMD_PN532_PDOWN (uint8_t) 0x16
#define CMD_PN532_RF_CONF (uint8_t) 0x32
#define CMD_PN532_BE_INIT_FOR_DEP (uint8_t) 0x56
#define CMD_PN532_BE_INIT_FOR_PSL (uint8_t) 0x46
#define CMD_PN532_DET_PASSIVE_TARGET (uint8_t) 0x4A
#define CMD_PN532_IN_ATR (uint8_t) 0x50
#define CMD_PN532_IN_PSL (uint8_t) 0x4E
#define CMD_PN532_IN_DEP (uint8_t) 0x40
#define CMD_PN532_IN_COMM_THR (uint8_t) 0x42
#define CMD_PN532_IN_DESELECT (uint8_t) 0x44
#define CMD_PN532_IN_RELEASE (uint8_t) 0x52
#define CMD_PN532_IN_SELECT (uint8_t) 0x54
#define CMD_PN532_IN_AUTO_POL (uint8_t) 0x60
#define CMD_PN532_TG_INIT (uint8_t) 0x8C
#define CMD_PN532_TG_SET_GBYTE (uint8_t) 0x92
#define CMD_PN532_TG_GET_DATA (uint8_t) 0x86
#define CMD_PN532_TG_SET_DATA (uint8_t) 0x8E
#define CMD_PN532_TG_SET_MDATA (uint8_t) 0x94
#define CMD_PN532_TG_GET_INIT_CMD (uint8_t) 0x88
#define CMD_PN532_TG_RES_TO_INIT (uint8_t) 0x90
#define CMD_PN532_TG_GET_STATUS (uint8_t) 0x8A

/***
 * Baud Rate List
 */

#define BAUD_9_2K 0x00
#define BAUD_19_2K 0x01
#define BAUD_38_4K 0x02
#define BAUD_57_6K 0x03
#define BAUD_115_2K 0x04
#define BAUD_230_4K 0x05
#define BAUD_460_8K 0x06
#define BAUD_921_6K 0x07
#define BAUD_1288K 0x08

/***
 * RF Configuration type List
 */
#define RFCONF_FIELD 0x01
#define RFCONF_TIMING 0x02
#define RFCONf_COM_RTY 0x04
#define RFCONF_REQ_RTY 0x05
#define RFCONF_ASET_LSPEED_A 0x0A
#define RFCONF_ASET_LSPEED_B 0x0C
#define RFCONF_ASET_HSPEED 0x0B

/***
 * Macro Function
 */
#define DBGM(x) {Serial.println("");Serial.print(F("NFC :: "));Serial.println(F(x));}
#define UPPER(x) (uint8_t)((x >> 8) & (uint8_t) 0xFF)
#define LOWER(x) (uint8_t)(x & (uint8_t) 0xFF)
#define HAS_ERROR(x) (x & 0x3F) != 0
#define GET_ERROR(x) (x & 0x3F)

#define NFC_RDY 1

NFC::NFC(uint8_t ss) {

	pinMode(ss, OUTPUT);
	digitalWrite(ss,HIGH);
	SPI.begin();
	SPI.setClockDivider(SPI_CLOCK_DIV8);
	SPI.setDataMode(SPI_MODE0);
	SPI.setBitOrder(LSBFIRST);

	nfcBuff = new uint8_t[NFC_BUFFER_SIZE];
	Rf_Field = 0;

	delay(400);

	this->NFC_SS = ss;
	if (getFirmwareVersion() != NFC_VERSION) {
		DBGM("NFC Version doesn't Match");
	} else {
		DBGM("NFC Version Matches");
	}

	DBGM("Start Device Test");
	if(IS_ERROR(performDeviceTest())){
		DBGM("Device Test Fail");
	}

}

NFC::~NFC() {
	SPI.end();
	delete[] nfcBuff;
}

uint16_t NFC::diagnose(uint8_t type, uint8_t len, const uint8_t* in_parm,
		uint8_t* buf, uint16_t timeout) {
	nfcBuff[0] = CMD_PN532_DIAGNOSE;
	nfcBuff[1] = type;
	for (uint8_t i = 0; i < len; i++) {
		nfcBuff[i + 2] = in_parm[i];
	}
	if (sendCommandCheck(nfcBuff, len + 2) != NFC_SUCCESS) {
		DBGM("Diagnose command error");
		return NFC_ERROR; // return Error,if sending command failed
	}

#if DBG
	DBGM("Diagnose command is Sent Successfully!!");
#endif

	NFCFrame mframe;
	if (readDataFrame(&mframe, timeout) != NFC_SUCCESS) {
		DBGM("Diagnose Failed : no response from NFC IC");
		return NFC_ERROR;
	}
	uint8_t plen = mframe.getPayloadLength();
	const uint8_t* payload = mframe.getPayload();
	if(payload[0] == CMD_PN532_DIAGNOSE + 1){
		/**Valid Response**/
		memcpy(buf,payload,plen);
		return plen | NFC_SUCCESS;
	}
	return NFC_ERROR;
}

uint32_t NFC::getFirmwareVersion() {
	uint8_t cmd = CMD_PN532_GETVERSION;
	if (sendCommandCheck(&cmd, 1) != NFC_SUCCESS) {
		DBGM("Get Firmware command Failed");
		return NFC_ERROR;
	} else {
		DBGM("Get Firmware command Successful");
	}
	NFCFrame frame;
	if (readDataFrame(&frame, 500) == NFC_SUCCESS) {
		//Read Success
		const uint8_t* payload = frame.getPayload();
		if (payload[0] != 0x03)
			return NFC_ERROR;
		else {
			return payload[1] << 24 | payload[2] << 16 | payload[3] << 8
					| payload[4];
		}
	}
	return NFC_ERROR;

}

uint16_t NFC::getGeneralStatus(uint8_t* readbuf) {
	uint8_t cmd = CMD_PN532_GETSTATUS;
	if (sendCommandCheck(&cmd, 1) != NFC_SUCCESS) {
		DBGM("Get General Status is Fail");
		return NFC_ERROR;
	} else {
		DBGM("Get General Status is Successful");
	}
	NFCFrame mframe;
	if (readDataFrame(&mframe, 200) == NFC_SUCCESS) {
		const uint8_t* payload = mframe.getPayload();
		uint8_t plen = mframe.getPayloadLength();
		if (payload[0] != 0x05)
			return NFC_ERROR;
		else {
#if DBG
			PRINT_ARRAY(payload, plen);
#endif
			memcpy(readbuf, payload, plen);
			return plen | NFC_SUCCESS;
		}
		return NFC_ERROR;
	}
	return NFC_ERROR;
}

uint16_t NFC::readRegister(uint16_t* addrs, uint8_t* outbuff, uint8_t len) {
	nfcBuff[0] = CMD_PN532_RDREG;
	for (uint8_t i = 0; i < len; i++) {
		nfcBuff[1 + i * 2] = UPPER(addrs[2 * i]);
		nfcBuff[2 + i * 2] = LOWER(addrs[2 * i]);
	}
#if DBG
	DBGM("Send Read Register");
	PRINT_ARRAY(nfcBuff, len*2+1);
#endif
	if (IS_ERROR(sendCommandCheck(nfcBuff, len * 2 + 1))) {
		DBGM("Read Register Cmd Failed!");
		return NFC_ERROR;
	}
	NFCFrame mframe;
	if (IS_SUCCESS(readDataFrame(&mframe,200))) {
		const uint8_t* payload = mframe.getPayload();
		uint8_t plen = mframe.getPayloadLength();
		if (payload[0] == CMD_PN532_RDREG + 1) {
			DBGM("Print Array");
//			PRINT_ARRAY(payload, plen);
			memcpy(outbuff, payload, plen);
			return plen | NFC_SUCCESS;
		}
		return NFC_ERROR;
	}
	return NFC_ERROR;
}

uint16_t NFC::writeRegister(uint16_t* addrs, uint8_t* invals, uint8_t len) {
	nfcBuff[0] = CMD_PN532_WRREG;
	for (uint16_t i = 0; i < len; i++) {
		nfcBuff[1 + i * 3] = UPPER(addrs[2 * i]);
		nfcBuff[1 + i * 3 + 1] = LOWER(addrs[2 * i + 1]);
		nfcBuff[1 + i * 3 + 2] = invals[i];
	}
	if (IS_SUCCESS(sendCommandCheck(nfcBuff,len * 3 + 1))) {
		DBGM("Write Register is Successful");
	} else {
		DBGM("Write Register failed");
		return NFC_ERROR;
	}
	NFCFrame mframe;
	if (IS_SUCCESS(readDataFrame(&mframe,200))) {
		if (nfcBuff[0] == CMD_PN532_WRREG + 1)
			return NFC_SUCCESS;
		return NFC_ERROR;
	}
	return NFC_ERROR;
}



uint16_t NFC::readRegister(uint16_t addr) {
	uint8_t val[2] = {0,};
	if (IS_SUCCESS(readRegister(&addr,val,1))) {
#if DBG
		DBGM("Read Reg Successful!!");
#endif
		return val[1] | NFC_SUCCESS;
	}
#if DBG
	DBGM("Read Reg Failed");
#endif
	return NFC_ERROR;
}
uint16_t NFC::writeRegister(uint16_t addr, uint8_t val) {
	if (IS_SUCCESS(writeRegister(&addr,&val,1))) {
		return NFC_SUCCESS;
	}
	return NFC_ERROR;
}


uint16_t NFC::setParameter(uint8_t flags) {
	nfcBuff[0] = CMD_PN532_SET_PARM;
	nfcBuff[1] = flags;
	if (IS_ERROR(sendCommandCheck(nfcBuff,2))) {
		return NFC_ERROR;
	}
	NFCFrame mframe;
	if (IS_SUCCESS(readDataFrame(&mframe,200))) {
		const uint8_t* payload = mframe.getPayload();
		uint8_t plen = mframe.getPayloadLength();
		if (payload[0] == CMD_PN532_SET_PARM + 1) {
			return NFC_SUCCESS;
		}
		return NFC_ERROR;
	}
	return NFC_ERROR;
}

uint16_t NFC::SAMConfiguration(uint8_t mode, uint8_t timeout, bool irq_enable) {
	nfcBuff[0] = CMD_PN532_SAM_CONF;
	nfcBuff[1] = mode;
	nfcBuff[2] = timeout;
	nfcBuff[3] = irq_enable ? 0x01 : 0x0;
	if (IS_ERROR(sendCommandCheck(nfcBuff,4))) {
		return NFC_ERROR;
	}

	NFCFrame mframe;
	if (IS_SUCCESS(readDataFrame(&mframe,200))) {
		const uint8_t* payload = mframe.getPayload();
		uint8_t plen = mframe.getPayloadLength();
		if (payload[0] == CMD_PN532_SAM_CONF + 1) {
			return NFC_SUCCESS;
		}
		return NFC_ERROR;
	}
	return NFC_ERROR;
}

uint16_t NFC::pdwon(uint8_t wakeupflag, bool irqenable) {
	nfcBuff[0] = CMD_PN532_PDOWN;
	nfcBuff[1] = wakeupflag;
	nfcBuff[2] = irqenable ? 0x01 : 0x00;
	if (IS_ERROR(sendCommandCheck(nfcBuff,3))) {
		return NFC_ERROR;
	}

	NFCFrame mframe;
	if (IS_SUCCESS(readDataFrame(&mframe,200))) {
		const uint8_t* payload = mframe.getPayload();
		uint8_t plen = mframe.getPayloadLength();
		if (payload[0] == CMD_PN532_PDOWN + 1) {
#if DBG
			DBGM("NFC Dev(PN532) goes pdown Successfully!");
#endif
			return NFC_SUCCESS;
		}
		return NFC_ERROR;
	}
	return NFC_ERROR;
}

uint16_t NFC::RFConfiguration(uint8_t cfgtype, uint8_t* args, uint8_t alen) {
	nfcBuff[0] = CMD_PN532_RF_CONF;
	nfcBuff[1] = cfgtype;
	memcpy(nfcBuff + 2, args, alen);
#if DBG
	PRINT_ARRAY(nfcBuff,alen + 2);
#endif
	if (IS_ERROR(sendNormalFrame(nfcBuff,alen+2))) {
		DBGM("Send Cmd Failed @ RF Configuration");
		return NFC_ERROR;
	}
#if DBG
	DBGM("Send Cmd is Successful @ RF Configuration");
#endif
	NFCFrame mframe;
	if (IS_SUCCESS(readDataFrame(&mframe,200))) {
		const uint8_t* payload = mframe.getPayload();
		uint8_t plen = mframe.getPayloadLength();
		if (payload[0] == CMD_PN532_RF_CONF + 1) {
			DBGM("RF Configuration Successful");
			return NFC_SUCCESS;
		}
		DBGM("RF Configuration Failed : Time out or Error");
		return NFC_ERROR;
	}
#if DBG
		DBGM("RF Configuration Failed : Time out or Error");
#endif
	return NFC_ERROR;
}

uint16_t NFC::inJumpForDEP(uint8_t mode, uint8_t baudrate, uint8_t next,
		uint8_t* parm, uint16_t len, uint8_t* readbuf) {
	nfcBuff[0] = CMD_PN532_BE_INIT_FOR_DEP;
	nfcBuff[1] = mode;
	nfcBuff[2] = baudrate;
	nfcBuff[3] = next;
	memcpy(nfcBuff + 4, parm, len);
	if (IS_ERROR(sendCommandCheck(nfcBuff,nfcBuff[3] == 0? 4:4+len))) {
		return NFC_ERROR;
	}

	NFCFrame mframe;
	if (IS_SUCCESS(readDataFrame(&mframe,200))) {
		const uint8_t* payload = mframe.getPayload();
		uint8_t plen = mframe.getPayloadLength();
		if ((payload[0] == CMD_PN532_BE_INIT_FOR_DEP + 1) && !HAS_ERROR(payload[1])) {
#if DBG
			DBGM("Set PN532 as Initiator for DEP");
#endif
			memcpy(readbuf, payload, plen);
			return plen | NFC_SUCCESS;
		}
		return NFC_ERROR | GET_ERROR(payload[1]);
	}
	return NFC_ERROR;

}

uint16_t NFC::inJumpForPSL(uint8_t mode, uint8_t baudrate, uint8_t next,
		uint8_t* parm, uint16_t len, uint8_t* readbuf) {
	nfcBuff[0] = CMD_PN532_BE_INIT_FOR_PSL;
	nfcBuff[1] = mode;
	nfcBuff[2] = baudrate;
	nfcBuff[3] = next;
	memcpy(nfcBuff + 4, parm, len);
	if (IS_ERROR(sendCommandCheck(nfcBuff,4+len))) {
		return NFC_ERROR;
	}

	NFCFrame mframe;
	if (IS_SUCCESS(readDataFrame(&mframe,200))) {
		const uint8_t* payload = mframe.getPayload();
		uint8_t plen = mframe.getPayloadLength();
		if ((payload[0] == CMD_PN532_BE_INIT_FOR_PSL + 1) && !HAS_ERROR(payload[1])) {
#if DBG
			DBGM("Set PN532 as Initiator for PSL");
#endif
			memcpy(readbuf, payload, plen);
			return plen | NFC_SUCCESS;
		}
		return NFC_ERROR | GET_ERROR(payload[1]);
	}
	return NFC_ERROR;
}

uint16_t NFC::inListPassiveTarget(uint8_t maxtg, uint8_t type,
		const uint8_t* initialData, uint8_t len, uint8_t* res) {
	if (maxtg > 2 || !(maxtg > 0)) {
		return NFC_ERROR;
	}
	nfcBuff[0] = CMD_PN532_DET_PASSIVE_TARGET;
	nfcBuff[1] = maxtg;
	nfcBuff[2] = type;
	memcpy(nfcBuff + 3, initialData, len);
	if (IS_ERROR(sendCommandCheck(nfcBuff,len + 3))) {
		return NFC_ERROR;
	}
	NFCFrame mframe;
	if (IS_SUCCESS(readDataFrame(&mframe,200))) {
		const uint8_t* payload = mframe.getPayload();
		uint8_t plen = mframe.getPayloadLength();
		if (payload[0] == CMD_PN532_DET_PASSIVE_TARGET + 1) {
			memcpy(res, payload, plen);
			return plen | NFC_SUCCESS;
		}
	}
	return NFC_ERROR;
}

uint16_t NFC::inATR(uint8_t tgNum, uint8_t next, const uint8_t* indata,
		uint8_t ilen, uint8_t* outdata) {
	nfcBuff[0] = CMD_PN532_IN_ATR;
	nfcBuff[1] = tgNum;
	nfcBuff[2] = next;
	memcpy(nfcBuff + 3, indata, ilen);
	if (IS_ERROR(sendCommandCheck(nfcBuff,ilen + 3))) {
		return NFC_ERROR;
	}

	NFCFrame mframe;
	if (IS_SUCCESS(readDataFrame(&mframe,200))) {
		const uint8_t* payload = mframe.getPayload();
		uint8_t plen = mframe.getPayloadLength();
		if ((payload[0] == CMD_PN532_IN_ATR + 1) && !HAS_ERROR(payload[1])) {
			memcpy(outdata, payload, plen);
			return plen | NFC_SUCCESS;
		}
		return NFC_ERROR | GET_ERROR(payload[1]);
	}
	return NFC_ERROR;
}

uint16_t NFC::inPSL(uint8_t tg, uint8_t brTxi, uint8_t brRxi) {
	nfcBuff[0] = CMD_PN532_IN_PSL;
	nfcBuff[1] = tg;
	nfcBuff[2] = brTxi;
	nfcBuff[3] = brRxi;
	if (IS_ERROR(sendCommandCheck(nfcBuff,4))) {
		return NFC_ERROR;
	}

	NFCFrame mframe;
	if (IS_SUCCESS(readDataFrame(&mframe,200))) {
		const uint8_t* payload = mframe.getPayload();
		if ((payload[0] == CMD_PN532_IN_PSL + 1) && !HAS_ERROR(payload[1])) {
			return NFC_SUCCESS;
		}
#if DBG
		Serial.print(F("Error Code : "));
		Serial.println(payload[1] & 0x3f, HEX);
#endif
		return NFC_ERROR | GET_ERROR(payload[1]);
	}
	return NFC_ERROR;
}

uint16_t NFC::inDataExchange(uint8_t tg, uint8_t* dataout, uint16_t outlen,
		uint8_t* datain) {
	nfcBuff[0] = CMD_PN532_IN_DEP;
	nfcBuff[1] = tg;
	memcpy(nfcBuff + 2, dataout, outlen);
	if (IS_ERROR(sendCommandCheck(nfcBuff,outlen + 2))) {
		return NFC_ERROR;
	}
	NFCFrame mframe;
	if (IS_SUCCESS(readDataFrame(&mframe,200))) {
		const uint8_t* payload = mframe.getPayload();
		uint8_t plen = mframe.getPayloadLength();
		if ((payload[0] == CMD_PN532_IN_DEP + 1) && !HAS_ERROR(payload[1])) {
			memcpy(datain, payload, plen);
			return plen | NFC_SUCCESS;
		}
		return NFC_ERROR | GET_ERROR(payload[1]);
	}
	return NFC_ERROR;
}

uint16_t NFC::inCommThru(uint8_t* dataout, uint8_t outlen, uint8_t* datain) {
	nfcBuff[0] = CMD_PN532_IN_COMM_THR;
	memcpy(nfcBuff + 1, dataout, outlen);
	if (IS_ERROR(sendCommandCheck(nfcBuff,outlen + 1))) {
		return NFC_ERROR;
	}

	NFCFrame mframe;
	if (IS_SUCCESS(readDataFrame(&mframe,200))) {
		const uint8_t* payload = mframe.getPayload();
		uint8_t plen = mframe.getPayloadLength();
		if ((payload[0] == CMD_PN532_IN_COMM_THR + 1) && !HAS_ERROR(payload[1])) {
			memcpy(datain, payload, plen);
			return plen | NFC_SUCCESS;
		}
		return NFC_ERROR | GET_ERROR(payload[1]);
	}
	return NFC_ERROR;
}

uint16_t NFC::inDeselect(uint8_t tg) {
	nfcBuff[0] = CMD_PN532_IN_DESELECT;
	nfcBuff[1] = tg;
	if (IS_ERROR(sendCommandCheck(nfcBuff,2))) {
		return NFC_ERROR;
	}

	NFCFrame mframe;
	if (IS_SUCCESS(readDataFrame(&mframe,200))) {
		const uint8_t* payload = mframe.getPayload();
		if ((payload[0] == CMD_PN532_IN_DESELECT + 1) && !HAS_ERROR(payload[1])) {
			return NFC_SUCCESS;
		}
		return NFC_ERROR | GET_ERROR(payload[1]);
	}
	return NFC_ERROR;

}

uint16_t NFC::inRelease(uint8_t tg) {
	nfcBuff[0] = CMD_PN532_IN_RELEASE;
	nfcBuff[1] = tg;
	if (IS_ERROR(sendCommandCheck(nfcBuff,2))) {
		return NFC_ERROR;
	}

	NFCFrame mframe;
	if (IS_SUCCESS(sendCommandCheck(nfcBuff,2))) {
		const uint8_t* payload = mframe.getPayload();
		if ((payload[0] == CMD_PN532_IN_RELEASE + 1) && !HAS_ERROR(payload[1])) {
			return NFC_SUCCESS;
		}
		return NFC_ERROR | GET_ERROR(payload[1]);
	}
	return NFC_ERROR;
}

uint16_t NFC::inSelect(uint8_t tg) {
	nfcBuff[0] = CMD_PN532_IN_SELECT;
	nfcBuff[1] = tg;
	if (IS_ERROR(sendCommandCheck(nfcBuff,2))) {
		return NFC_ERROR;
	}

	NFCFrame mframe;
	if (IS_SUCCESS(sendCommandCheck(nfcBuff,2))) {
		const uint8_t* payload = mframe.getPayload();
		if ((payload[0] == CMD_PN532_IN_SELECT + 1) && !HAS_ERROR(payload[1])) {
			return NFC_SUCCESS;
		}
		return NFC_ERROR | GET_ERROR(payload[1]);
	}
	return NFC_ERROR;
}

uint16_t NFC::inAutoPoll(uint8_t pollMax, uint8_t period, uint8_t* types,
		uint8_t tlen, uint8_t* res) {
	nfcBuff[0] = CMD_PN532_IN_AUTO_POL;
	nfcBuff[1] = pollMax;
	nfcBuff[2] = period;
	memcpy(nfcBuff + 3, types, tlen);
	if (IS_ERROR(sendCommandCheck(nfcBuff,tlen + 3))) {
		return NFC_ERROR;
	}

	NFCFrame mframe;
	if (IS_SUCCESS(readDataFrame(&mframe,200))) {
		const uint8_t* payload = mframe.getPayload();
		uint8_t plen = mframe.getPayloadLength();
		if (payload[0] == CMD_PN532_IN_AUTO_POL + 1) {
			memcpy(res, payload, plen);
			return NFC_SUCCESS | plen;
		}
		return NFC_ERROR;
	}
	return NFC_ERROR;
}
/**
 @param modeFlag :
 @param mifareParams:
 @param
 */
uint16_t NFC::tgInitAsTarget(NfcTargetConfig* cnf, uint8_t* res) {
	nfcBuff[0] = CMD_PN532_TG_INIT;
	uint8_t len = cnf->write(nfcBuff + 1);
	if (IS_ERROR(sendCommandCheck(nfcBuff,len + 1))) {
#if DBG
		DBGM("SetAsTarget CMD failed");
#endif
		return NFC_ERROR;
	}
#if DBG
	DBGM("SetAsTarget CMD is successful");
#endif

	NFCFrame mframe;
	if (IS_SUCCESS(readDataFrame(&mframe,500))) {
		const uint8_t* payload = mframe.getPayload();
		uint8_t plen = mframe.getPayloadLength();
		if (payload[0] == 0x8D) {
			DBGM("Valid Frame");
			memcpy(res, payload, plen);
			return plen | NFC_SUCCESS;
		}
		return NFC_ERROR;
	}
	return NFC_ERROR;

}

uint16_t NFC::tgInitAsTarget(uint8_t* res){
	NfcTargetConfig conf(0x00,PARAM_MIFARE,PARAM_FELICA,NFCID);
	return tgInitAsTarget(&conf,res);
}

uint16_t NFC::tgSetGeneralBytes(uint8_t* gbytes, uint8_t len) {
	nfcBuff[0] = CMD_PN532_TG_SET_GBYTE;
	memcpy(nfcBuff + 1, gbytes, len);
	if (IS_ERROR(sendCommandCheck(nfcBuff,len + 1))) {
		return NFC_ERROR;
	}
	NFCFrame mframe;
	if (IS_SUCCESS(readDataFrame(&mframe,300))) {
		const uint8_t* payload = mframe.getPayload();
		if ((payload[0] == CMD_PN532_TG_SET_GBYTE + 1) && !HAS_ERROR(payload[1])) {
			return NFC_SUCCESS;
		}
		return GET_ERROR(payload[1]) | NFC_ERROR;
	}
	return NFC_ERROR;
}

uint16_t NFC::tgGetData(uint8_t* res) {
	nfcBuff[0] = CMD_PN532_TG_GET_DATA;
	if (IS_ERROR(sendCommandCheck(nfcBuff,1))) {
		return NFC_ERROR;
	}

	NFCFrame mframe;
	if (IS_SUCCESS(readDataFrame(&mframe,200))) {
		const uint8_t* payload = mframe.getPayload();
		uint8_t plen = mframe.getPayloadLength();
		if ((payload[0] == CMD_PN532_TG_GET_DATA + 1) && !HAS_ERROR(payload[1])) {
			memcpy(res, payload, plen);
			return plen | NFC_SUCCESS;
		}
		return NFC_ERROR | GET_ERROR(payload[1]);
	}
	return NFC_ERROR;
}

uint16_t NFC::tgSetData(uint8_t* data, uint8_t len) {
	nfcBuff[0] = CMD_PN532_TG_SET_DATA;
	memcpy(nfcBuff + 1, data, len);
	if (IS_ERROR(sendCommandCheck(nfcBuff,len + 1))) {
		return NFC_ERROR;
	}

	NFCFrame mframe;
	if (IS_SUCCESS(readDataFrame(&mframe,200))) {
		const uint8_t* payload = mframe.getPayload();
		if ((payload[0] == CMD_PN532_TG_SET_DATA + 1) && !HAS_ERROR(payload[1])) {
			return NFC_SUCCESS;
		}
		return NFC_ERROR | GET_ERROR(payload[1]);
	}
	return NFC_ERROR;
}

uint16_t NFC::tgSetMetaData(uint8_t* mdata, uint8_t len) {
	nfcBuff[0] = CMD_PN532_TG_SET_MDATA;
	memcpy(nfcBuff + 1, mdata, len);
	if (IS_ERROR(sendCommandCheck(nfcBuff,len + 1))) {
		return NFC_ERROR;
	}

	NFCFrame mframe;
	if (IS_SUCCESS(readDataFrame(&mframe,200))) {
		const uint8_t* payload = mframe.getPayload();
		if ((payload[0] == CMD_PN532_TG_SET_MDATA + 1) && !HAS_ERROR(payload[1])) {
			return NFC_SUCCESS;
		}
		return NFC_ERROR | GET_ERROR(payload[1]);
	}
	return NFC_ERROR;
}

uint16_t NFC::tgGetInitiatorCommand(uint8_t* cmd) {
	nfcBuff[0] = CMD_PN532_TG_GET_INIT_CMD;
	if (IS_ERROR(sendCommandCheck(nfcBuff,1))) {
		return NFC_ERROR;
	}

	NFCFrame mframe;
	if (IS_SUCCESS(readDataFrame(&mframe,200))) {
		const uint8_t* payload = mframe.getPayload();
		uint8_t plen = mframe.getPayloadLength();
		if ((payload[0] == CMD_PN532_TG_GET_INIT_CMD + 1)
				&& !HAS_ERROR(payload[1])) {
			memcpy(cmd, payload, plen);
			return plen | NFC_SUCCESS;
		}
		return NFC_ERROR | GET_ERROR(payload[1]);
	}
	return NFC_ERROR;
}

uint16_t NFC::tgResponseToInitiator(uint8_t* data, uint8_t len) {
	nfcBuff[0] = CMD_PN532_TG_RES_TO_INIT;
	memcpy(nfcBuff + 1, data, len);
	if (IS_ERROR(sendCommandCheck(nfcBuff,len + 1))) {
		return NFC_ERROR;
	}

	NFCFrame mframe;
	if (IS_SUCCESS(readDataFrame(&mframe,200))) {
		const uint8_t* payload = mframe.getPayload();
		if ((payload[0] == CMD_PN532_TG_RES_TO_INIT + 1)
				&& !HAS_ERROR(payload[1])) {
			return NFC_SUCCESS;
		}
		return NFC_ERROR | GET_ERROR(payload[1]);
	}
	return NFC_ERROR;
}

uint16_t NFC::tgGetTargetStatus(uint8_t* res) {
	nfcBuff[0] = CMD_PN532_TG_GET_STATUS;
	if (IS_ERROR(sendCommandCheck(nfcBuff,1))) {
		return NFC_ERROR;
	}

	NFCFrame mframe;
	if (IS_SUCCESS(readDataFrame(&mframe,200))) {
		const uint8_t* payload = mframe.getPayload();
		uint8_t plen = mframe.getPayloadLength();
		if ((payload[0] == CMD_PN532_TG_GET_STATUS + 1)
				&& !HAS_ERROR(payload[1])) {
			memcpy(res, payload, plen);
			return plen | NFC_SUCCESS;
		}
		return NFC_ERROR | GET_ERROR(payload[1]);
	}
	return NFC_ERROR;
}

/**
 *
 */
uint16_t NFC::sendNormalFrame(uint8_t* payload, uint8_t len) {
	digitalWrite(NFC_SS, LOW);
	delay(4);

	uint8_t pLen = len + 1;
	uint8_t chks = 0;
//	uint8_t chks = PN532_NORMAL_STARTCODE1;
	chks += TFI_HOST_TO_PN532;

	//Start Frame
	SPI.transfer(NFC_DATA_WRITE);
	SPI.transfer(PN532_PREAMBLE);
	SPI.transfer(PN532_NORMAL_STARTCODE0);
	SPI.transfer(PN532_NORMAL_STARTCODE1);
	SPI.transfer(pLen);
	SPI.transfer(~pLen + 1);
	SPI.transfer(TFI_HOST_TO_PN532);
	for (uint8_t i = 0; i < len; i++) {
		SPI.transfer(payload[i]);
		chks += payload[i];
	}
	SPI.transfer(~chks + 1);
	SPI.transfer(PN532_POSTAMBLE);
	digitalWrite(NFC_SS, HIGH);
	return NFC_SUCCESS;
}

uint16_t NFC::sendCommandCheck(uint8_t* payload, uint8_t len) {
	sendNormalFrame(payload, len);
	if (!bWaitReady(200)) {
#if DBG
		DBGM("NFC doesn't go to ready state after send command op.");
#endif
		return NFC_ERROR;
	}

#if DBG
	DBGM("NFC ready after command");
#endif
	switch (checkAck()) {
	case NFC_ACK:
		break;
	case NFC_ERROR:
		return NFC_ERROR;
	case NFC_NACK:
		DBGM("Nack is received @ sendCommandCheck");
		return NFC_ERROR;
	}
#if DBG
	DBGM("Ack is received @ sendCommandCheck");
#endif
	return NFC_SUCCESS;
}

uint16_t NFC::sendExtFrame(uint8_t* payload, uint16_t len) {

}

bool NFC::bWaitReady(uint32_t timeout) {
	digitalWrite(NFC_SS, LOW);
	delay(4);
	SPI.transfer(NFC_STATUS_READ);
	while ((SPI.transfer(0) != NFC_RDY) && timeout) {
		timeout -= 20;
		delay(20);
	}
	digitalWrite(NFC_SS, HIGH);
	return timeout > 0;
}

uint16_t NFC::readDataFrame(NFCFrame* outframe, uint16_t timeout) {
	digitalWrite(NFC_SS, LOW);
	delay(3);

	if (!bWaitReady(timeout)) {
#if DBG
		DBGM("Read Data operation is timed-out");
#endif
		return NFC_ERROR;
	}
	digitalWrite(NFC_SS, HIGH);
	delay(4);

	digitalWrite(NFC_SS, LOW);
	delay(4);
	Util::ShiftBuffer<uint8_t> shBuf(6);
	SPI.transfer(NFC_DATA_READ);
	while (!shBuf.match(PREAMBLE, 1, 2)) {
		shBuf.shift(SPI.transfer(0));
	}
	if (!outframe->inflate(shBuf.getBuffer())) {
		digitalWrite(NFC_SS,HIGH);
		delay(3);
		return NFC_ERROR;
	}

	uint8_t len = outframe->getPayloadLength();
	for (uint8_t i = 0; i < len + PN532_NORMAL_FOOTER_SIZE; i++) {
		nfcBuff[i] = SPI.transfer(0);
	}
	digitalWrite(NFC_SS, HIGH);
	uint8_t result = outframe->setPayload(nfcBuff);
	return result | NFC_SUCCESS;
}

uint16_t NFC::checkAck() {
	digitalWrite(NFC_SS, LOW);
	delay(3);
#if DBG
		Serial.println("CheckAck Start");
#endif

	Util::ShiftBuffer<uint8_t> shbuf(6);
#if DBG
		Serial.println("ShiftBuffer Initialized");
#endif
	SPI.transfer(NFC_DATA_READ);
	for (byte i = 0; i < 6; i++) {
		shbuf.shift(SPI.transfer(0));
	}
	if (shbuf.find((uint8_t*) ACK_FRAME, 6) > 0) {
#if DBG
		Serial.println("ACK Received");
#endif
		digitalWrite(NFC_SS, HIGH);
		delay(2);
		return NFC_ACK;
	} else if (shbuf.find((uint8_t*) NACK_FRAME, 6) > 0) {
#if DBG
		Serial.println("NACK Received");
#endif
		digitalWrite(NFC_SS, HIGH);
		delay(2);
		return NFC_NACK;
	}
#if DBG
	Serial.println("Neither ACK nor NACK");
#endif
	digitalWrite(NFC_SS, HIGH);
	delay(2);
	return NFC_ERROR;
}

bool NFC::performDeviceTest() {
	/**
	 * Communication Line Test
	 */
	nfcBuff[0] = 0xFF;
	if (diagnose(DIAG_COMMLINE, 1, nfcBuff, nfcBuff, 1000) > 2) {
#if DBG
		DBGM("Line Test OK");
#endif
	} else {
#if DBG
		DBGM("Line Test Not Ok");
#endif
		return false;
	}
	if (nfcBuff[GET_VALUE(diagnose(DIAG_ROM, 0, NULL, nfcBuff, 5000)) - 1] == 0x00) {
		DBGM("ROM Test OK");
	} else {
		DBGM("ROM Test Not Ok");
		return false;
	}
	if (nfcBuff[GET_VALUE(diagnose(DIAG_RAM, 0, NULL, nfcBuff, 5000)) - 1] == 0x00) {
		DBGM("RAM Test OK");
	} else {
		DBGM("RAM Test Not Ok");
		return false;
	}
	return true;

}

uint16_t NFC::setAutoRFCA(bool b) {
	Rf_Field &= ~0x02;
	if (b) {
		Rf_Field |= 0x02;
	}
	if (IS_ERROR(RFConfiguration(RFCONF_FIELD,&Rf_Field,1))) {
		return NFC_ERROR;
	}

	NFCFrame mframe;
	if (IS_SUCCESS(readDataFrame(&mframe,200))) {
		if (mframe.getPayload()[0] == 0x33) {
			return NFC_SUCCESS;
		}
	}
	return NFC_ERROR;
}

uint16_t NFC::setRFEnable(bool b) {
	Rf_Field &= ~0x01;
	if (b) {
		Rf_Field |= 0x01;
	}
	if (IS_ERROR(RFConfiguration(RFCONF_FIELD,&Rf_Field,1))) {
		return NFC_ERROR;
	}

	NFCFrame mframe;
	if (IS_SUCCESS(readDataFrame(&mframe,200))) {
		if (mframe.getPayload()[0] == 0x33) {
			return NFC_SUCCESS;
		}
	}
	return NFC_ERROR;
}

Array<TargetBriefInfo>* NFC::getTargets() {
	uint8_t res = getGeneralStatus(nfcBuff);
	uint8_t ntg = 0;
	if (res > 5) {
		ntg = nfcBuff[3];
#if DBG
		DBGM("Target is being handled");
#endif
		Array<TargetBriefInfo>* ret = new Array<TargetBriefInfo>(ntg);
		for (int i = 0; i < ret->length; i++) {
			ret->operator [](i).brRx = nfcBuff[3 + i * 4 + 2];
			ret->operator [](i).brRx = nfcBuff[3 + i * 4 + 3];
			ret->operator [](i).brRx = nfcBuff[3 + i * 4 + 4];
		}
		return ret;
	} else {
		DBGM("No Target is being handled");
		return NULL;
	}
}

uint16_t NFC::getErrorCode() {
	if (getGeneralStatus(nfcBuff) != NFC_ERROR) {
		return nfcBuff[1] | NFC_SUCCESS;
	}
	return NFC_ERROR;

}
bool NFC::checkExternField() {
	if (getGeneralStatus(nfcBuff) != NFC_ERROR) {
		return nfcBuff[2] == (uint8_t) 0x01;
	}
	return false;
}

uint16_t NFC::getSAMStatus() {
	uint8_t res = getGeneralStatus(nfcBuff);
	uint8_t ntg = 0;
	if (IS_SUCCESS(res)) {
		ntg = nfcBuff[3];
		return nfcBuff[3 + 4 * ntg + 1] | NFC_SUCCESS;
	}
	return NFC_ERROR;
}

uint8_t NFCFrame::getPayloadLength() const {
	return len - 1;
}

bool NFCFrame::inflate(uint8_t* header) {
	if (header[0] + header[1] + header[2] != 0xff) {
#if DBG
		Serial.println("Data doesn't conform frame structure");
#endif
		return false;
	}
	len = header[3];
	lchk = header[4];
	tfi = header[5];
	payload = new uint8_t[len - 1];
#if DBG
	DBGM("Data conform Normal Frame");
	Serial.print("Len : ");
	Serial.println(len, HEX);
	Serial.print("Len Chk  : ");
	Serial.println(lchk, HEX);
	Serial.print("TFI : ");
	Serial.println(tfi, HEX);
#endif
	return true;
}

bool NFCFrame::isValidHeader() {
	return (uint8_t)(len + lchk) == 0x00;
}

uint8_t NFCFrame::setPayload(uint8_t* data) {
	uint8_t dchk = tfi;
	for (uint8_t i = 0; i < len - 1; i++) {
		payload[i] = data[i];
		dchk += data[i];
	}
	if ((uint8_t)(data[len - 1] + dchk) != 0x00)
		return NFC_ERROR;
#if DBG
		return NFC_ERROR;
#endif
//	PRINT_ARRAY(data,len);
	return NFC_SUCCESS;
}

const uint8_t* NFCFrame::getPayload() const {
	return payload;
}

NFCFrame::~NFCFrame() {
	delete payload;
}

NfcTargetConfig::NfcTargetConfig(NfcTargetConfig& conf) :
		buff(35) {
	buff.seek(0);
	buff.write(conf.buff);
	gbLen = conf.gbLen;
	histLen = conf.histLen;
	if (gbLen > 0) {
		memcpy(gbytes, conf.gbytes, gbLen);
	}
	if (histLen > 0) {
		memcpy(hbytes, conf.hbytes, histLen);
	}
}

NfcTargetConfig::NfcTargetConfig(uint8_t mode, uint8_t* mifareP,
		uint8_t* felicaP, uint8_t* nfcId) :
		buff(35) {
	buff.seek(0);
	buff.write(mode);
	buff.write(mifareP, 6);
	buff.write(felicaP, 18);
	buff.write(nfcId, 10);
	gbLen = 0;
	histLen = 0;
	gbytes = NULL;
	hbytes = NULL;
}
NfcTargetConfig::~NfcTargetConfig() {
	if (gbytes != NULL) {
		delete[] gbytes;
	}
	if (hbytes != NULL) {
		delete[] hbytes;
	}
}

uint8_t NfcTargetConfig::setGeneralData(const uint8_t* data, uint8_t len) {
	gbytes = new uint8_t[len];
	gbLen = len;
	memcpy(gbytes, data, len);
	return gbLen;
}
uint8_t NfcTargetConfig::setHistData(const uint8_t* data, uint8_t len) {
	hbytes = new uint8_t[len];
	histLen = len;
	memcpy(hbytes, data, len);
	return histLen;
}
uint8_t NfcTargetConfig::write(uint8_t* dest) {
	buff.seek(0);
	uint8_t idx = buff.read(dest, 1 + 6 + 18 + 10);
	dest[idx++] = gbLen;
	if (gbLen > 0) {
		memcpy(dest + idx, gbytes, gbLen);
	}
	idx += gbLen;
	dest[idx++] = histLen;
	if (histLen > 0) {
		memcpy(dest + idx, hbytes, histLen);
	}
#if DBG
	Serial.print("Written Length : ");Serial.println(idx + histLen,DEC);
#endif
	return idx + histLen;
}

} /* namespace Eagle */

