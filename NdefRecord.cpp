/*
 * NdefRecord.cpp
 *
 *  Created on: 2013. 12. 27.
 *      Author: innocentevil
 */

#include <Arduino.h>
#include "NdefRecord.h"
#include <mem.h>

#define LOGE(x) {Serial.print(F("Error @ NdefRecord.cpp : "));Serial.println(F(x));}
#define LOGEN(x,r) {Serial.print(F("Error @ NdefRecord.cpp : "));Serial.print(F(x));Serial.println(r,HEX);}
#define LOGD(x) {Serial.print(F("Debug @ NdefRecord.cpp : "));Serial.println(F(x));}
#define LOGDN(x,r) {Serial.print(F("Debug @ NdefRecord.cpp : "));Serial.print(F(x));Serial.println(r,HEX);}
#define PRINT_ARRAY(l,x,s) {Serial.print("\n");Serial.print(F(l));for(int i = 0;i < s;i++){Serial.print("\t");Serial.print(x[i],HEX);}Serial.print("\n");}

namespace NFC {

uint8_t NdefRecord::RTD_SMART_POSTER[] = { 'S', 'p' };
uint8_t NdefRecord::RTD_TEXT[] = { 'T' };
uint8_t NdefRecord::RTD_URI[] = { 'U' };
uint8_t NdefRecord::RTD_GENERIC_CONTROL[] = { 'G', 'c' };
uint8_t NdefRecord::RTD_HANDOVER_REQ[] = { 'H', 'r' };
uint8_t NdefRecord::RTD_HANDOVER_SEL[] = { 'H', 's' };
uint8_t NdefRecord::RTD_HANDOVER_CARRIER[] = { 'H', 'c' };
uint8_t NdefRecord::RTD_SIGNATURE[] = { 'S', 'g' };
char NdefRecord::AAR_TYPE[] = "android.com:pkg";
bool NdefRecord::IS_BENDIAN = isBigEndian();

NdefRecord* NdefRecord::createTextRecord(const char* text,
		const char* locale,TextEncodeType tt) {
	NdefInitType init;
        init.tlen = 1;
        init.idlen = 0;
	uint16_t tMsgLen = strcspn(text, "");
	uint16_t tLocLen = strcspn(locale, "");
        init.plen = tLocLen + tMsgLen + 1;
        init.pload = (uint8_t*) malloc(init.plen);
        init.type = (uint8_t*) malloc(init.tlen);
        *init.type = 'T';
	init.id = NULL;
	init.tnf = TNF_WELL_KNOWN;
	init.pload[0] = tt == UTF8 ? 1 << 1:1 << 7 | 1 << 1;
	memcpy(init.pload + 1, locale, tLocLen);
	memcpy(init.pload + tLocLen + 1, text, tMsgLen);
	return new NdefRecord(&init);
}

NdefRecord* NdefRecord::createUriRecord(const uint8_t uriId,
		const char* uristring) {
        NdefInitType init;
        init.plen = strcspn(uristring,"") + 1;
        init.idlen = 0;
        init.tlen = 1;
        init.pload = (uint8_t*) malloc(init.plen);
        init.pload[0] = uriId;
        memcpy(init.pload + 1,uristring,init.plen - 1);
        init.id = NULL;
        init.type = (uint8_t*) malloc(init.tlen);
        *init.type = 'U';
        init.tnf = TNF_WELL_KNOWN;
	return new NdefRecord(&init);
}

NdefRecord* NdefRecord::createEmptyRecord() {
	NdefInitType init;
	init.plen = 0;
	init.idlen = 0;
	init.tlen = 0;
	init.pload = NULL;
	init.id = NULL;
	init.type = NULL;
	init.tnf = TNF_EMPTY;
	return new NdefRecord(&init);
}


NdefRecord* NdefRecord::createAndroidApplicationRecord(const char* apppkgname){
        NdefInitType init;
        init.plen = strcspn(apppkgname,"");
        init.idlen = 0;
        init.tlen = strcspn(AAR_TYPE,"");
        init.pload = (uint8_t*) malloc(init.plen);
        init.type = (uint8_t*) malloc(init.tlen);
        init.id = NULL;
        memcpy(init.pload,apppkgname,init.plen);
        memcpy(init.type,AAR_TYPE,init.tlen);
        init.tnf = TNF_EXTERNAL;
        return new NdefRecord(&init);
}

NdefRecord* NdefRecord::parse(uint8_t* data){
	uint32_t offset = 0;
	NdefInitType init;
	init.plen = 0;
	init.tlen = 0;
	init.idlen = 0;
	init.id = NULL;
	init.type = NULL;
	init.pload = NULL;
	init.tnf = 0;
	uint8_t flag = data[offset++];
	bool hasID = (flag & NDEF_FLAG_MSK_IL) == NDEF_FLAG_MSK_IL;
	init.tnf = flag & NDEF_FLAG_MSK_TNF;
	init.tlen = data[offset++];
	if((flag & NDEF_FLAG_MSK_SR) == NDEF_FLAG_MSK_SR){
	     //Record Type Short
		 init.plen = data[offset++];
	}else{
	     //Record Type Normal
		 if(IS_BENDIAN){
         	 init.plen |= data[offset++] << 24;
			 init.plen |= data[offset++] << 16;
			 init.plen |= data[offset++] << 8;
			 init.plen |= data[offset++];
		}else{
		     memcpy(&init.plen,&data[offset],4);
			 offset += 4;
		}
	}
	if(hasID){
         init.idlen = data[offset++];
	}
	if(init.tlen > 0){
//	     init.type = new uint8_t[init.tlen];
             init.type = (uint8_t*) malloc(init.tlen);
             memcpy(init.type,&data[offset],init.tlen);
             offset += init.tlen;
	}
	if(hasID && (init.idlen > 0)){
//	     init.type = new uint8_t[init.idlen];
             init.id = (uint8_t*) malloc(init.idlen);
             memcpy(init.id,&data[offset],init.idlen);
             offset += init.idlen;
	}
	if(init.plen > 0){
//	     init.pload = new uint8_t[init.plen];
             init.pload = (uint8_t*) malloc(init.plen);
             memcpy(init.pload,&data[offset],init.plen);
             offset += init.plen;
	}else{
	     // Invalid Data
		 // or Parsing Error
	     return NULL;
	}
	return new NdefRecord(&init);
}


NdefRecord::NdefRecord(NdefInitType* init, uint8_t flag) {
    payload = NULL;
    type = NULL;
    id = NULL;
	if (init->plen > 0) {
                payload = (uint8_t*) malloc(init->plen);
		memcpy(payload, init->pload, init->plen);
	}
	if (init->tlen > 0) {
                type = (uint8_t*) malloc(init->tlen);
		memcpy(type, init->type, init->tlen);
	}
	if (init->plen < 0xFF) {
		rcdType = RCD_TYPE_SHORT;
                header = (void*) malloc(sizeof(NdefRecordShort));
		NdefRecordShort* dsHeader = (NdefRecordShort*) header;
		dsHeader->flag = flag | NDEF_FLAG_MSK_SR
				| (NDEF_FLAG_MSK_TNF & init->tnf);
		dsHeader->ilen = init->idlen;
		dsHeader->plen = init->plen;
		dsHeader->tlen = init->tlen;
		id = NULL;
		if (dsHeader->ilen > 0) {
			dsHeader->flag |= NDEF_FLAG_MSK_IL;
                        id = (uint8_t*) malloc(init->idlen);
			memcpy(id, init->id, dsHeader->ilen);
		}

	} else {
		// Normal Record Type
		rcdType = RCD_TYPE_NORMAL;
                header = (void*) malloc(sizeof(NdefRecordNormal));
		NdefRecordNormal* dnHeader = (NdefRecordNormal*) header;
		dnHeader->flag = flag | (NDEF_FLAG_MSK_TNF & init->tnf);
		dnHeader->ilen = init->idlen;
		uint8_t* pl = (uint8_t*) &init->plen;
		if (IS_BENDIAN) {
			dnHeader->plen[0] = pl[3];
			dnHeader->plen[1] = pl[2];
			dnHeader->plen[2] = pl[1];
			dnHeader->plen[3] = pl[0];
		} else {
			memcpy(dnHeader->plen, pl, sizeof(uint32_t));
		}
		dnHeader->tlen = init->tlen;
		id = NULL;
		if (dnHeader->ilen > 0) {
			dnHeader->flag |= NDEF_FLAG_MSK_IL;
                        id = (uint8_t*) malloc(init->idlen);
			memcpy(id, init->id, dnHeader->ilen);
		}
	}
        free(init->id);
        free(init->type);
        free(init->pload);
}

NdefRecord::NdefRecord(uint8_t* record) {
    type = NULL;
    payload = NULL;
    id = NULL;
    uint16_t offset = 0;
    if(*record & NDEF_FLAG_MSK_SR != 0){
        rcdType = RCD_TYPE_SHORT;
        NdefRecordShort* srcd = (NdefRecordShort*) malloc(sizeof(NdefRecordShort));
        header = (void*) srcd;
        srcd->flag = record[offset++];
        srcd->tlen = record[offset++];
        srcd->plen = record[offset++];
        if(hasIdField()){          
                srcd->ilen = record[offset++];                
        }
        if(srcd->tlen > 0){
            type = (uint8_t*) malloc(srcd->tlen);
            memcpy(type,&record[offset],srcd->tlen);           
            offset += srcd->tlen;
        }
        if(hasIdField() && srcd->ilen > 0){
            id = (uint8_t*) malloc(srcd->ilen);
            memcpy(id,&record[offset],srcd->ilen);
            offset += srcd->ilen;
        }
        if(srcd->plen > 0){
            payload = (uint8_t*) malloc(srcd->plen);
            memcpy(payload,&record[offset],srcd->plen);
        }
    }else{
        rcdType = RCD_TYPE_NORMAL;
        LOGD("Rcd Type : Normal");
        NdefRecordNormal* nrcd = (NdefRecordNormal*)malloc(sizeof(NdefRecordNormal));
        header = (void*) nrcd;
        nrcd->flag = record[offset++];
        nrcd->tlen = record[offset++];
        memcpy(nrcd->plen,&record[offset],4);
        offset += 4;
        if(hasIdField()){
            nrcd->ilen = record[offset++];
        }
        if(nrcd->tlen > 0){
            type = (uint8_t*) malloc(nrcd->tlen);
            memcpy(type,&record[offset],nrcd->tlen);
            offset += nrcd->tlen;
        }
        if(hasIdField() && nrcd->ilen > 0){
            id = (uint8_t*) malloc(nrcd->ilen);
            memcpy(id,&record[offset],nrcd->ilen);
            offset += nrcd->ilen;
        }
        if(nrcd->plen > 0){
            uint32_t psize = getPayloadLength();
            payload = (uint8_t*) malloc(psize);
            memcpy(payload,&record[offset],psize);
        }
    }
}

NdefRecord::~NdefRecord() {
        LOGD("Destructor invoked");
	if (rcdType == RCD_TYPE_SHORT) {
		free((NdefRecordShort*) header);
	} else {
		free((NdefRecordNormal*) header);
	}
        free(payload);
       	free(id);
      	free(type);
}

uint16_t NdefRecord::writeRecord(uint8_t* buf) {
        LOGD("Write Record");
	uint8_t headerSize = 0;
	uint8_t tSize = getTypeLength();
	uint8_t iSize = getIdLength();
	uint32_t pSize = getPayloadLength();
	bool hasId = hasIdField();
	uint16_t offset = 0;
	switch (rcdType) {
	case RCD_TYPE_SHORT:
		headerSize = sizeof(NdefRecordShort);
		break;
	case RCD_TYPE_NORMAL:
		headerSize = sizeof(NdefRecordNormal);
		break;
	}
	if (!hasId) {
		headerSize--;
	}
	memcpy(buf, header, headerSize);
	offset += headerSize;
	//Header Cpy complete
	if (tSize > 0) {
		memcpy(buf + offset, type, tSize);
		offset += tSize;
	}
	//Type Cpy complete
	if (hasId && (iSize > 0)) {
		memcpy(buf + offset, id, iSize);
		offset += iSize;
	}
	//Optional ID Field Copied
	if (pSize > 0) {
		memcpy(buf + offset, payload, pSize);
		offset += pSize;
	}
        LOGDN("Write Size : ",offset);
	return offset;
}

uint32_t NdefRecord::getPayloadLength() const {
	NdefRecordNormal* tnHeader = NULL;
	switch (rcdType) {
	case RCD_TYPE_NORMAL:
		tnHeader = (NdefRecordNormal*) header;
		if (IS_BENDIAN) {
			return tnHeader->plen[0] << 24 | tnHeader->plen[1] << 16
					| tnHeader->plen[2] << 8 | tnHeader->plen[3];
		}
		return *((uint32_t*) tnHeader->plen);
	case RCD_TYPE_SHORT:
		return ((NdefRecordShort*) header)->plen;
	}
	return 0;
}

const uint8_t* const NdefRecord::getPayload() {
	return payload;
}

uint8_t NdefRecord::getTnf() {
	uint8_t* flagv = (uint8_t*) ((((header))));
	return *flagv & NDEF_FLAG_MSK_TNF;
}

const uint8_t* const NdefRecord::getType() {
	return type;
}

uint8_t NdefRecord::getTypeLength() const {
	NdefRecordNormal* tnH = NULL;
	switch (rcdType) {
	case RCD_TYPE_NORMAL:
		tnH = (NdefRecordNormal*) header;
		return tnH->tlen;
	case RCD_TYPE_SHORT:
		return ((NdefRecordShort*) header)->tlen;
	}
	return 0;
}

const uint8_t* const NdefRecord::getId() {
	return id;
}

uint8_t NdefRecord::getIdLength() const {
	if (!hasIdField()) {
		return 0;
	}
	NdefRecordNormal* tnh = NULL;
	switch (rcdType) {
	case RCD_TYPE_NORMAL:
		tnh = (NdefRecordNormal*) header;
		return tnh->ilen;
	case RCD_TYPE_SHORT:
		return ((NdefRecordShort*) header)->ilen;
	}
	return 0;
}

bool NdefRecord::isChunked() {
	uint8_t* flagv = (uint8_t*) ((((header))));
	return (*flagv & NDEF_FLAG_MSK_CF) > 0;
}

bool NdefRecord::isFirst() {
	uint8_t* flagv = (uint8_t*) ((((header))));
	return (*flagv & NDEF_FLAG_MSK_MB) > 0;
}

bool NdefRecord::isLast() {
	uint8_t* flagv = (uint8_t*) ((((header))));
	return (*flagv & NDEF_FLAG_MSK_ME) > 0;
}

bool NdefRecord::isShort() {
	uint8_t* flag = (uint8_t*) ((((header))));
	return (*flag & NDEF_FLAG_MSK_SR) > 0;
}

NdefRecord::NdefRecord() {
	payload = NULL;
	type = NULL;
	id = NULL;
	rcdType = RCD_TYPE_SHORT;
	header = NULL;
}

bool NdefRecord::hasIdField() const {
	uint8_t* flag = (uint8_t*) ((((header))));
	return (*flag & NDEF_FLAG_MSK_IL) > 0;
}

bool NdefRecord::isBigEndian() {
	uint16_t tv = 0xFF;
	uint8_t* utv = (uint8_t*) ((((&tv))));
	return utv[0] == 0xFF;
}

uint32_t NdefRecord::getRecordSize() {
	uint32_t size = 0;
	switch (rcdType) {
	case RCD_TYPE_NORMAL:
		size += sizeof(NdefRecordNormal);
		break;
	case RCD_TYPE_SHORT:
		size += sizeof(NdefRecordShort);
		break;
	}
	if (!hasIdField())
		size--;
	else {
		size += getIdLength();
	}
	size += getTypeLength();
	size += getPayloadLength();
	return size;
}

uint8_t NdefRecord::clearFlag(uint8_t flb) {
	return *(uint8_t*) ((((header)))) &= ~flb;
}

uint8_t NdefRecord::setFlag(uint8_t flb) {
	return *(uint8_t*) ((((header)))) |= flb;
}

NdefRecord::NdefRecord(const NdefRecord& self) {
	//copy header
	//
	uint8_t tlen = self.getTypeLength();
	uint8_t ilen = self.getIdLength();
	uint8_t plen = self.getPayloadLength();
	rcdType = self.rcdType;
	switch (rcdType) {
	case RCD_TYPE_SHORT:
                header = malloc(sizeof(NdefRecordShort));
		memcpy(header, self.header, sizeof(NdefRecordShort));
		break;
	case RCD_TYPE_NORMAL:
                header = malloc(sizeof(NdefRecordNormal));
		memcpy(header, self.header, sizeof(NdefRecordNormal));
		break;
	}
	//copy type
        type = (uint8_t*) malloc(tlen);
	memcpy(type, self.type, tlen);
	//copy id if supported
	if (self.hasIdField() && (ilen > 0)) {
                id = (uint8_t*) malloc(ilen);
		memcpy(id, self.id, ilen);
	}
	//copy payload
        payload = (uint8_t*) malloc(plen);
	memcpy(payload, self.payload, plen);
}

/* namespace NFC */
NdefRecord& NdefRecord::operator =(const NdefRecord& rho) {
	if (this != &rho) {
		if (!isBlankRecord()) {
                        free(payload);
                        free(id);
                        free(type);
			switch (rcdType) {
			case RCD_TYPE_SHORT:
				free((NdefRecordShort*) header);
				break;
			case RCD_TYPE_NORMAL:
				free((NdefRecordNormal*) header);
				break;
			}
		}
		uint8_t tlen = rho.getTypeLength();
		uint8_t ilen = rho.getIdLength();
		uint16_t plen = rho.getPayloadLength();
		rcdType = rho.rcdType;
		switch (rcdType) {
		case RCD_TYPE_SHORT:
                        header = malloc(sizeof(NdefRecordShort));
			memcpy(header, rho.header, sizeof(NdefRecordShort));
			break;
		case RCD_TYPE_NORMAL:
                        header = malloc(sizeof(NdefRecordNormal));
			memcpy(header, rho.header, sizeof(NdefRecordNormal));
			break;
		}

                type = (uint8_t*) malloc(tlen);
		memcpy(type, rho.type, tlen);

		if (rho.hasIdField() && (ilen > 0)) {
                        id = (uint8_t*) malloc(ilen);
			memcpy(id, rho.id, ilen);
		}
                payload = (uint8_t*) malloc(plen);
		memcpy(payload, rho.payload, plen);
	}
	return *this;
}

bool NdefRecord::isBlankRecord() {
	return (payload == NULL) && (id == NULL) && (type == NULL);
}

}

