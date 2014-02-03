
#include <SPI.h>
#include "NFC.h"
#include "IsoDepTag.h"


NFC::NFC* nfc_dev;
NFC::IsoDepTag* tag;


void setup(){

  Serial.begin(115200);
  Serial.println();
  Serial.println("---Start Initiate---");
  nfc_dev = new NFC::NFC(10);
  tag = new NFC::IsoDepTag(nfc_dev);
  NFC::NdefRecord* rcds[] = {
    NFC::NdefRecord::createTextRecord("Hello World","en",NFC::NdefRecord::UTF8)
  };
  NFC::NdefMessage message(rcds,1);
  tag->setNdefMsg(message);
}

void loop(){
  Serial.println(F("----------------Loop Begin-----------------"));
 if(tag->startIsoDepTag()){
   Serial.println("Valid Initiator Cmmand");
   delay(100);
 } else{
   delay(150);
 }
}
