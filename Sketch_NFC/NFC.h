/*
 * NFC.h
 *
 *  Created on: 2013. 12. 14.
 *      Author: innocentevil
 */

#ifndef NFC_H_
#define NFC_H_

#include <Arduino.h>
#include <inttypes.h>
#include "Array.h"

namespace NFC {

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




#define NFC_ERROR (uint16_t) 0xF000
#define NFC_SUCCESS (uint16_t) 0x0F00

#define IS_ERROR(x) (x & NFC_ERROR) > 0
#define IS_SUCCESS(x) (x & NFC_SUCCESS) > 0
#define GET_VALUE(x) ((uint8_t) 0xff & (uint8_t) x)
#define PRINT_ARRAY(x,y) {for(int i = 0;i < y;i++){Serial.print("\t");Serial.print(x[i],HEX);}Serial.println();}



typedef struct _target_binfo {
	uint8_t brRx;
	uint8_t brTx;
	uint8_t type;
} TargetBriefInfo;

typedef struct _target_finfo{
	uint8_t nfcid[10];
	uint8_t did;
	uint8_t brTx;
	uint8_t brRx;
	uint8_t tIndex;
	uint8_t pp;
	uint8_t* gbyte;
	uint8_t gblen;
}TargetFullInfo;

class NFCFrame {
public:
	NFCFrame() :
			len(0), payload(NULL), lchk(0), tfi(0) {
	}
	virtual ~NFCFrame();
	bool inflate(uint8_t* header);
	bool isValidHeader();
	uint8_t getPayloadLength() const;
	uint8_t setPayload(uint8_t* payload);
	const uint8_t* getPayload() const;
private:
	uint8_t len;
	uint8_t* payload;
	uint8_t lchk;
	uint8_t tfi;

protected:
};


class NfcTargetConfig{

public:
	NfcTargetConfig(NfcTargetConfig& conf);
	NfcTargetConfig(uint8_t mode,uint8_t* mifareP,uint8_t* felicaP,uint8_t* nfcId);
	virtual ~NfcTargetConfig();
	uint8_t setGeneralData(const uint8_t* data,uint8_t len);
	uint8_t setHistData(const uint8_t* data,uint8_t len);
	uint8_t write(uint8_t* dest);
private:
	Array<uint8_t> buff;
	uint8_t gbLen;
	uint8_t* gbytes;
	uint8_t histLen;
	uint8_t* hbytes;
};

class NFC {
public:

	NFC(uint8_t ss);
	virtual ~NFC();

	uint16_t getSAMStatus();
	uint16_t setParameter(uint8_t flags);

	uint16_t SAMConfiguration(uint8_t mode, uint8_t timeout, bool irq_enable);
	uint16_t pdwon(uint8_t wakeupflag, bool irqenable);

	/**
	 * Wrapping getgeneral status cmd
	 * return target information currently handled by pn532
	 */
	Array<TargetBriefInfo>* getTargets();
	/**
	 * get most recent error code
	 */
	uint16_t getErrorCode();
	/**
	 * check whether there is external rf field or not
	 */
	bool checkExternField();


	uint16_t readRegister(uint16_t addr);
	uint16_t writeRegister(uint16_t addr,uint8_t val);
	uint16_t setAutoRFCA(bool b);
	uint16_t setRFEnable(bool b);



	uint16_t RFConfiguration(uint8_t cfgtype, uint8_t* args,uint8_t alen);
	uint16_t inJumpForDEP(uint8_t mode, uint8_t baudrate, uint8_t next,uint8_t* parm,uint16_t len,uint8_t* res);
	uint16_t inJumpForPSL(uint8_t mode, uint8_t baudrate, uint8_t next,uint8_t* parm,uint16_t len,uint8_t* res);
	uint16_t inListPassiveTarget(uint8_t maxtg, uint8_t type, const uint8_t* initialData, uint8_t len,uint8_t* res);
	uint16_t inATR(uint8_t tgNum, uint8_t next, const uint8_t* indata, uint8_t ilen,uint8_t* outdata);
	uint16_t inPSL(uint8_t tg, uint8_t brTxi,uint8_t brRxi);
	uint16_t inDataExchange(uint8_t tg, uint8_t* dataout, uint16_t outlen, uint8_t* res);
	uint16_t inCommThru(uint8_t* dataout,uint8_t outlen, uint8_t* res);
	uint16_t inDeselect(uint8_t tg);
	uint16_t inRelease(uint8_t tg);
	uint16_t inSelect(uint8_t tg);
	uint16_t inAutoPoll(uint8_t pollMax, uint8_t period, uint8_t* types,uint8_t len,uint8_t* res);
	uint16_t tgInitAsTarget(NfcTargetConfig* tgConfig,uint8_t* res);
	uint16_t tgInitAsTarget(uint8_t* res);
	uint16_t tgSetGeneralBytes(uint8_t* gbytes,uint8_t len);
	uint16_t tgGetData(uint8_t* res);
	uint16_t tgSetData(uint8_t* data,uint8_t len);
	uint16_t tgSetMetaData(uint8_t* mdata,uint8_t len);
	uint16_t tgGetInitiatorCommand(uint8_t* cmd);
	uint16_t tgResponseToInitiator(uint8_t* data,uint8_t len);
	uint16_t tgGetTargetStatus(uint8_t* res);
	bool performDeviceTest();

protected:
private:
	uint8_t NFC_SS;

	static uint8_t PARAM_MIFARE[6];
	static uint8_t PARAM_FELICA[18];
	static uint8_t NFCID[10];


	static uint8_t ACK_FRAME[];
	static uint8_t NACK_FRAME[]; // = { 0x00, 0x00, 0xff, 0xff, 0x00, 0x00 };
	static uint8_t PREAMBLE[]; // = { 0x00, 0xff };
	static uint8_t POSTAMBLE[]; // = { 0x00 };
	uint8_t* nfcBuff;
	uint16_t diagnose(uint8_t type, uint8_t len, const uint8_t* in_parm,
			uint8_t* buf, uint16_t timeout);
	uint32_t getFirmwareVersion();
	uint16_t getGeneralStatus(uint8_t* readbuf);
	uint16_t readRegister(uint16_t* addrs, uint8_t* outbuff, uint8_t len);
	uint16_t writeRegister(uint16_t* addrs, uint8_t* invals, uint8_t len);
	uint16_t sendCommandCheck(uint8_t* payload, uint8_t len);
	uint16_t sendNormalFrame(uint8_t* payload, uint8_t len);
	uint16_t sendExtFrame(uint8_t* payload, uint16_t len);
	uint16_t readDataFrame(NFCFrame* outfram, uint16_t timeout);
	uint16_t checkAck();
	uint8_t Rf_Field;

	bool bWaitReady(uint32_t timeout);
};

} /* namespace Eagle */
#endif /* NFC_H_ */

