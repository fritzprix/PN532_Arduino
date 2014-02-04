/*
 * NdefRecord.h
 *
 *  Created on: 2013. 12. 27.
 *      Author: innocentevil
 */

#ifndef NDEFRECORD_H_
#define NDEFRECORD_H_

#include "inttypes.h"

namespace NFC {
#define NDEF_FLAG_MSK_MB (uint8_t) 1 << 7
#define NDEF_FLAG_MSK_ME (uint8_t) 1 << 6
#define NDEF_FLAG_MSK_CF (uint8_t) 1 << 5
#define NDEF_FLAG_MSK_SR (uint8_t) 1 << 4
#define NDEF_FLAG_MSK_IL (uint8_t) 1 << 3
#define NDEF_FLAG_MSK_TNF (uint8_t) 7

#define TNF_EMPTY 0x00
#define TNF_WELL_KNOWN 0x01
#define TNF_MEDIA 0x02
#define TNF_URI 0x03
#define TNF_EXTERNAL 0x04
#define TNF_UNKNOWN 0x05
#define TNF_UNCHANGED 0x06

#define URI_HTTP_WWW ((uint8_t) 0x01) // http://www.
#define URI_HTTPS_WWW ((uitn8_t) 0x02)  // https://www.
#define URI_HTTP ((uint8_t) 0x03)    // http://
#define URI_HTTPS ((uint8_t) 0x04)   // https://
#define URI_TEL ((uint8_t) 0x05)  //tel:
#define URI_MAILTO ((uint8_t) 0x06)  // mailto:
#define URI_FTP_ANONYM ((uint8_t) 0x07)  // ftp://anonymous:anonymous@
#define URI_FTP_FTP_DOT ((uint8_t) 0x08) // ftp://ftp.
#define URI_FTPS ((uint8_t) 0x09)  // ftps://
#define URI_SFTP ((uint8_t) 0x0A)  // sftp://
#define URI_SMB ((uint8_t) 0x0B)   // smb://
#define URI_NFS ((uint8_t) 0x0C)   // nfs://
#define URI_FTP ((uint8_t) 0x0D)   // ftp://
#define URI_DAV ((uint8_t) 0x0E)   // dav://
#define URI_NEWS ((uint8_t) 0x0F)  // news:
#define URI_TELNET ((uint8_t) 0x10) // telnet://
#define URI_IMAP ((uint8_t) 0x11)  //  imap:
#define URI_RTSP ((uint8_t) 0x12)  //  rtsp://
#define URI_URN ((uint8_t) 0x13)  // urn:
#define URI_POP ((uint8_t) 0x14)  // pop:
#define URI_SIP ((uint8_t) 0x15)  // sip:
#define URI_SIPS ((uint8_t) 0x16)  // sips:
#define URI_TFTP ((uint8_t) 0x17)  // tftp:
#define URI_BTSPP ((uint8_t) 0x18) // btspp://
#define URI_BTL2CAP ((uint8_t) 0x19) // btl2cap://
#define URI_BTGOEP ((uint8_t) 0x1A) // btgoep://
#define URI_TCPOBEX ((uint8_t) 0x1B) // tcpobex://
#define URI_IRDAOBEX ((uint8_t) 0x1C) // irdaobex://
#define URI_FILE ((uint8_t) 0x1D) // file://


#define RCD_TYPE_SHORT 0x0F
#define RCD_TYPE_NORMAL 0xF0

typedef struct _sr_record_wid{
	uint8_t flag;
	uint8_t tlen;
	uint8_t plen;
	uint8_t ilen;
}NdefRecordShort;



typedef struct _nr_record_wid{
	uint8_t flag;
	uint8_t tlen;
	uint8_t plen[4];
	uint8_t ilen;
}NdefRecordNormal;



typedef struct _ndef_init_type{
	uint8_t idlen;
	uint8_t tlen;
	uint32_t plen;
	uint8_t* id;
	uint8_t* type;
	uint8_t* pload;
	uint8_t tnf;
}NdefInitType;


class NdefRecord {
public:
	NdefRecord(NdefInitType* init,uint8_t flag = NDEF_FLAG_MSK_SR);
	NdefRecord(uint8_t* record);
	NdefRecord(const NdefRecord& other);
	NdefRecord();
	virtual ~NdefRecord();
	uint16_t writeRecord(uint8_t* buf);
	uint32_t getPayloadLength() const;
	const uint8_t* const getPayload();
	uint8_t getTnf();
	const uint8_t* const getType();
	uint8_t getTypeLength() const;
	const uint8_t* const getId();
	uint8_t getIdLength() const;
	bool isChunked();
	bool isFirst();
	bool isLast();
	bool isShort();
	bool hasIdField() const;
	uint8_t clearFlag(uint8_t flb);
	uint8_t setFlag(uint8_t flb);
	uint32_t getRecordSize();
        typedef enum _t_code_ {
		UTF8,UTF16
	}TextEncodeType;

	static NdefRecord* createTextRecord(const char* text,const char* locale,TextEncodeType tt);
	static NdefRecord* createUriRecord(const uint8_t uriId,const char* uristring);
        static NdefRecord* createAndroidApplicationRecord(const char* fullqaulname);
	static NdefRecord* createEmptyRecord();
	static NdefRecord* parse(uint8_t* rawbytes);

	NdefRecord& operator=(const NdefRecord& rho);

	static bool isBigEndian();
private:
	static uint8_t RTD_SMART_POSTER[];
	static uint8_t RTD_TEXT[];
	static uint8_t RTD_URI[];
	static uint8_t RTD_GENERIC_CONTROL[];
	static uint8_t RTD_HANDOVER_REQ[];
	static uint8_t RTD_HANDOVER_SEL[];
	static uint8_t RTD_HANDOVER_CARRIER[];
	static uint8_t RTD_SIGNATURE[];
        static char AAR_TYPE[];
	static bool IS_BENDIAN;

	bool isBlankRecord();

	uint8_t rcdType;
	void* header;
	uint8_t* payload;
	uint8_t* type;
	uint8_t* id;

};



} /* namespace NFC */
#endif /* NDEFRECORD_H_ */
