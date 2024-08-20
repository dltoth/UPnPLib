/**
 * 
 *  ssdp Library
 *  Copyright (C) 2023  Daniel L Toth
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published 
 *  by the Free Software Foundation, either version 3 of the License, or any 
 *  later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *  
 *  The author can be contacted at dan@leelanausoftware.com  
 *
 */

/**
 *  This protocol is not strictly SSDP, but rather an abbreviated version of the protocol with the following goals:
 *  (1) Reduce chattiness of standard UPnP/SSDP by only responding to known search requests
 *  (2) Provide enough information to populate a device hierarchy of the environment and
 *  (3) Allow query to see if root devices are still available on the network
 *  (4) Find instances of a specific Device (or Service) type on the network
 *  Search requests are sent out over the multicast address 239.255.255.250 port 1900 and responses are sent to the unicast
 *  IP address and port of the request.
 *  
 *  SSDP is chatty and could easily consume a small device responding to unnecessary requests. To this end we add a custom Search 
 *  Target header, ST.LEELANAUSOFTWARE.COM described below. Search requests without this header are silently ignored. This abreviated 
 *  protocol does not advertise on startup or shutdown, thus avoiding a flurry of unnecessary UPnP activiy. Devices respond ONLY to specific 
 *  queries, and ignore all other SSDP requests.
 *  
 *  In order to succinctly describe device hierarchy, we add a custom response header, DESC.LEELANAUSOFTWARE.COM. In this implementation
 *  of UPnP, RootDevices can have UPnPServices and UPnPDevices, and UPnPDevices can only have UPnPServices.The maximum number of embedded 
 *  devices (or services) is restricted 8, thus limiting the device hierarchy. We also add a custom field descriptor, puuid, which refers 
 *  to the parent uuid of a given UPnPDevice (or UPnPService). The DESC header field, defined below, can implicitly refer to a either a 
 *  RootDevice, an embedded UPnPDevice, or a UPnPService. When coupled with the USN, a complete device description in context is given.
 * 
 *  Header fields have the form :name:value: where the :name: is described below, and value is given by the specific device or service.
 *  
 *  Unique Service Name - USN is always uuid:device-UUID::urn:domain-name:device:deviceType:ver for a device (or Rootdevice) or
 *                                      uuid:device-UUID::urn:domain-name:service:serviceType:ver for a service
 *
 *  Device Description  - DESC.LEELANAUSOFTWARE.COM is :name:displayName:devices:num-devices:services:num-services: for Root Device
 *                                                     :name:displayName:services:num-services:puuid:parent-uuid: for an embedded Device and
 *                                                     :name:displayName:puuid:parent-uuid: for a Service 
 *                        where displayName is for display on a user interface, num-services is the number of services, num-devices is the number of 
 *                        embedded devices, and puuid is the uuid of the parent device. 
 *                        Note that implementations should not depend on the order of these fields, but rather look for the position of :name: and the
 *                        following : to determine the displayName and so on. If the :puuid: field is not present, the response is from a RootDevice.
 *                        It is considered an error if a :puuid: field is present and a :devices: field is also present. In this case the response should be
 *                        considered a UPnPDevice and :num-devices: should be ignored. Also, if the USN indicates a UPnPService, and a :devices: or :services: 
 *                        field is present, it is considered an error and should be ignored.                        
 * 
 ** ST Search Target and USN Response values 
 *  
 *  (1)    M-SEARCH Request header:
 *         ST:  upnp:rootdevice        Responds once for each root device
 *         ST.LEELANAUSOFTWARE.COM:  ssdp:all (or empty). If ssdp:all, responds for each embedded device and once for each service for each Root Device
 *                                     If empty, responds for each root device only. This is a required header
 *         
 *         Response Headers: 
 *         ST:  upnp:rootdevice
 *         USN: uuid:device-UUID::urn:domain-name:device:deviceType:ver for a device or
 *              uuid:device-UUID::urn:domain-name:service:serviceType:ver for a service
 *         DESC.LEELANAUSOFTWARE.COM: :name:displayName:devices:num-devices:services:num-services for a RootDevice or
 *                                    :name:displayName:services:num-services:puuid:parent-uuid for an embedded device or
 *                                    :name:displayName:puuid:parent-uuid: for a UPnPService
 *              
 *  (2)    M-SEARCH Request Header:
 *         ST:  uuid:device-UUID       Devices respond if either Root or an embedded Device have a matching uuid 
 *         ST.LEELANAUSOFTWARE.COM:  ssdp:all (or empty). If ssdp:all, responds for each embedded device (for RootDevices) and 
 *                                     once for each service. If empty, responds for the matching device only. This is 
 *                                     a required header.
 *         
 *         Response Headers:
 *         ST:  uuid:device-UUID for Device with matching uuid
 *         USN: uuid:device-UUID::urn:domain-name:device:deviceType:ver for a device or
 *              uuid:device-UUID::urn:domain-name:service:serviceType:ver for a service
 *         DESC.LEELANAUSOFTWARE.COM: :name:displayName:devices:num-devices:services:num-services for the RootDevice or
 *                                    :name:displayName:services:num-services:puuid:parent-uuid for an embedded device or
 *                                    :name:displayName:puuid:parent-uuid: for a UPnPService
 *                                      
 *  (3)    M-SEARCH Request Header:
 *         ST: urn:domain-name:device:deviceType:ver      for Device search or
 *             urn:domain-name:service:serviceType:ver    for service search
 *                                     Responses are sent for each matching Device (or Service)
 *                           
 *         ST.LEELANAUSOFTWARE.COM:  Required header, value not used
 *         
 *         Response Headers:
 *         ST:  urn:domain-name:device:deviceType:ver   for Device with matching device-type or
 *              urn:domain-name:service:serviceType:ver for Service with matchint type
 *         USN: uuid:device-UUID::urn:domain-name:device:deviceType:ver for a device or
 *              uuid:device-UUID::urn:domain-name:service:serviceType:ver for a service
 *         DESC.LEELANAUSOFTWARE.COM: name:displayName:devices:num-devices:services:num-services for the RootDevice or
 *                                      name:displayName:services:num-services:puuid:parent-uuid for an embedded device or
 *                                      name:displayName:puuid:parent-uuid: for a UPnPService
 *                                      
 *   SSDP Device Response:
 *      HTTP/1.1 200 OK
 *      CACHE-CONTROL: max-age = 1800
 *      LOCATION: Device URL
 *      ST: ST from M-SEARCH request
 *      USN: device USN
 *      DESC.LEELANAUSOFTWARE.COM: name:displayName:devices:num-devices:services:num-services for the RootDevice or
 *                                   name:displayName:services:num-services:puuid:parent-uuid for an embedded device
 *   
 *   SSDP Service Response:
 *      HTTP/1.1 200 OK
 *      CACHE-CONTROL: max-age = 1800
 *      LOCATION: Service URL relative to the Device URL
 *      ST: ST from M-SEARCH request
 *      USN: service USN
 *      DESC.LEELANAUSOFTWARE.COM: name:displayName:puuid:parent-uuid:
  
 */

/** Implementation Notes:
 *   1. RootDevice responds with location http://AA.BB.CC.DD:port
 *   2. UPnPDevice responds with location http://AA.BB.CC.DD:port/rootTarget/deviceTarget
 *   3. UPnPService responds with location http://AA.BB.CC.DD:port/rootTarget/deviceTarget/serviceTarget 
 */
 
/** PROGMEM Functions:
 *  
 *  int memcmp_P(const void* buf1, PGM_VOID_P buf2P, size_t size);
 *  void* memccpy_P(void* dest, PGM_VOID_P src, int c, size_t count);
 *  void* memmem_P(const void* buf, size_t bufSize, PGM_VOID_P findP, size_t findPSize);
 *  void* memcpy_P(void* dest, PGM_VOID_P src, size_t count);
 *  char* strncpy_P(char* dest, PGM_P src, size_t size);
 *  char* strcpy_P(dest, src)
 *  char* strncat_P(char* dest, PGM_P src, size_t size);
 *  char* strcat_P(dest, src)
 *  int strncmp_P(const char* str1, PGM_P str2P, size_t size);
 *  int strcmp_P(str1, str2P)
 *  int strncasecmp_P(const char* str1, PGM_P str2P, size_t size);
 *  int strcasecmp_P(str1, str2P)
 *  size_t strnlen_P(PGM_P s, size_t size);
 *  size_t strlen_P(strP)
 *  char* strstr_P(const char* haystack, PGM_P needle);
 *  int printf_P(PGM_P formatP, ...);
 *  int sprintf_P(char *str, PGM_P formatP, ...);
 *  int snprintf_P(char *str, size_t strSize, PGM_P formatP, ...);
 *  int vsnprintf_P(char *str, size_t strSize, PGM_P formatP, va_list ap);
 */
 
#include "ssdp.h"

namespace lsc {

const IPAddress SSDP_MULTICAST(239,255,255,250);
const long DELAY = 500;

// buffer for sending and receiving UDP data
#define TXN_BUFFER_SIZE    1536
#define ST_HEADER_SIZE     100
#define ST_LSC_HEADER_SIZE 20
#define SSDP_BUFFER_SIZE   1000

/** Response Templates
 *  
 */
const char  SERVICE_RESPONSE[]    PROGMEM = "HTTP/1.1 200 OK \r\n"
                                         "CACHE-CONTROL: max-age = 1800 \r\n"
                                         "LOCATION: %s\r\n"                                                          // Service Location
                                         "ST: %s\r\n"                                                                // Search Target
                                         "USN: uuid:%s::%s\r\n"                                                      // Parent Device uuid and Service type
                                         "DESC.LEELANAUSOFTWARE.COM: :name:%s:puuid:%s:\r\n\r\n\r\n";              // name and parent Device uuid

const char  DEVICE_RESPONSE[]     PROGMEM = "HTTP/1.1 200 OK \r\n"
                                         "CACHE-CONTROL: max-age = 1800 \r\n"
                                         "LOCATION: %s\r\n"                                                          // Device Location
                                         "ST: %s\r\n"                                                                // Search Target
                                         "USN: uuid:%s::%s\r\n"                                                      // uuid and device type
                                         "DESC.LEELANAUSOFTWARE.COM: :name:%s:services:%d:puuid:%s:\r\n\r\n\r\n";  // name, number of services, and parent uuid   

const char  ROOT_RESPONSE[]       PROGMEM = "HTTP/1.1 200 OK \r\n"
                                         "CACHE-CONTROL: max-age = 1800 \r\n"
                                         "LOCATION: %s\r\n"                                                           // Root Location
                                         "ST: %s\r\n"                                                                 // Search Target
                                         "USN: uuid:%s::%s\r\n"                                                       // uuid and device type
                                         "DESC.LEELANAUSOFTWARE.COM: :name:%s:devices:%d:services:%d:\r\n\r\n\r\n"; // Number of Devices and Number of Services 

const char SSDP_RootSearch[]      PROGMEM = "M-SEARCH * HTTP/1.1\r\n"
                                        "HOST: 239.255.255.250:1900\r\n"
                                        "MAN: ssdp:discover\r\n"
                                        "ST: upnp:rootdevice\r\n"
                                        "ST.LEELANAUSOFTWARE.COM: \r\n"
                                        "USER-AGENT: ESP8266 UPnP/1.1 LSC-SSDP/1.0\r\n\r\n";
const char SSDP_RootAllSearch[]   PROGMEM = "M-SEARCH * HTTP/1.1\r\n"
                                        "HOST: 239.255.255.250:1900\r\n"
                                        "MAN: ssdp:discover\r\n"
                                        "ST: upnp:rootdevice\r\n"
                                        "ST.LEELANAUSOFTWARE.COM: ssdp:all\r\n"
                                        "USER-AGENT: ESP8266 UPnP/1.1 LSC-SSDP/1.0\r\n\r\n";
const char SSDP_Search[]          PROGMEM = "M-SEARCH * HTTP/1.1\r\n"
                                        "HOST: 239.255.255.250:1900\r\n"
                                        "MAN: ssdp:discover\r\n"
                                        "ST: %s\r\n"
                                        "ST.LEELANAUSOFTWARE.COM: ssdp:all\r\n"
                                        "USER-AGENT: ESP8266 UPnP/1.1 LSC-SSDP/1.0\r\n\r\n";

/** Header field constants
 *  
 */
const char ST_LSC_HEADER[]       PROGMEM = "ST.LEELANAUSOFTWARE.COM";
const char ST_HEADER[]           PROGMEM = "ST";
const char USN_HEADER[]          PROGMEM = "USN";
const char ST_UPNP_ROOTDEVICE[]  PROGMEM = "upnp:rootdevice";
const char ST_UUID[]             PROGMEM = "uuid:";
const char ST_TYPE[]             PROGMEM = "urn:";
const char SSDP_ALL[]            PROGMEM = "ssdp:all";
const char DELIM[]               PROGMEM = "::";


/**
 *  The following functions are needed to mitigate the differences between ESP8266 UDP and ESP32 UDP.
 *  Other wiFi implementations may need to address other functions.
 */

void beginMulticast(WiFiUDP& channel) {
#ifdef ESP32
  channel.beginMulticast(SSDP_MULTICAST,UDP_PORT);
#else
  channel.beginMulticast(INADDR_ANY,SSDP_MULTICAST,UDP_PORT);
#endif
}

int getLocalPort(WiFiUDP& channel ) {
  int result = 0;
#ifdef ESP32
  result = 0;
#else
  result = channel.localPort();
#endif
return result;
}

void getUUID(char uuid[], int size, const char* st) {
   // Remove any leading blank chars
   const char* uuidBuff = st + 5;             
   while( *uuidBuff  == ' ' ) {uuidBuff++;} 
   strncpy(uuid,uuidBuff,size);  
}

LoggingLevel SSDP::_logging = NONE;

SSDP::SSDP() {}

int SSDP::getMulticastPort() {return UDP_PORT;}
int SSDP::getUDPPort() {return getLocalPort(_udp);}

/**
 *  Listening for SSDP search requests is done on the multicast channel, and replies are done on the
 *  unicast udp channel. Calling begin(0) allows udp to set an available port, which is sent on 
 */
void SSDP::begin(RootDevice* root) {
  _root = root;
  beginMulticast(_mUdp);
  _udp.begin(0);
}

void SSDP::doSSDP() {
  doChannel(_mUdp);
  doChannel(_udp);
}

/**
 *   Send an SSDP request and parse responses with SSDPHandler. Parse responses as long as they are viable, but
 *   don't wait any longer that timeout milliseconds for responses to come in.
 */
SSDPResult SSDP::searchRequest(const char* ST, SSDPHandler handler, IPAddress ifc, int timeout, boolean ssdpAll) {
  SSDPResult result = SSDP_OK;
  char txnBuffer[SSDP_BUFFER_SIZE];
  if( strcmp_P(ST,ST_UPNP_ROOTDEVICE) == 0) {
     if(ssdpAll) snprintf_P(txnBuffer,SSDP_BUFFER_SIZE,SSDP_RootAllSearch);
     else snprintf_P(txnBuffer,SSDP_BUFFER_SIZE,SSDP_RootSearch);
  }
  else if((strncmp_P(ST,ST_UUID,5) == 0) ) snprintf_P(txnBuffer,SSDP_BUFFER_SIZE,SSDP_Search,ST);
  else if((strncmp_P(ST,ST_TYPE,4) == 0))  snprintf_P(txnBuffer,SSDP_BUFFER_SIZE,SSDP_Search,ST);
  else result = SSDP_ERR_ST;

  if( result == SSDP_OK ) {
  
  WiFiUDP udp;
  int ok = 0;

#ifdef ESP8266
  udp.begin(0);
  ok = udp.beginPacketMulticast(IPAddress(239,255,255,250),1900,ifc);
#elif defined(ESP32)
  udp.begin(ifc,0);
  ok = udp.beginPacket(IPAddress(239,255,255,250),1900);
#endif

  if( ok != 1 ) {
    result = SSDP_ERR_UDP;
    if( loggingLevel(WARNING) ) Serial.printf("SSDP::searchRequest: Error on beginPacket\n");  
  }
  if( result == SSDP_OK ) {
    int len = strlen(txnBuffer);
    int sz = udp.write((unsigned char*)txnBuffer,len);
    ok = udp.endPacket();  
    if( ok != 1 ) {
      result = SSDP_ERR_SEND;
      if( loggingLevel(WARNING) ) Serial.printf("SSDP::searchRequest: Error on endPacket attempt to send %d bytes\n",len);
    }
    delay(500);
  }
  if( result == SSDP_OK ) {
      long timeStamp = millis();
      boolean done = false;
      while( (millis() - timeStamp < timeout) && !done ) {
         int packetSize = udp.parsePacket();
         if( packetSize > 0 ) {
           IPAddress remote = udp.remoteIP();
           txnBuffer[0] = 0;
           int available = udp.read(txnBuffer, SSDP_BUFFER_SIZE);
           txnBuffer[available] = 0;
           UPnPBuffer upnpBuff = UPnPBuffer(txnBuffer);
           if( upnpBuff.isSearchResponse() ) {
/**
 *           Reset the timestamp if we have an incomming response
 */
             timeStamp = millis();
           
/**
 *           The response MUST have an ST header and the ST header MUST match the search request
 */
             char st_header[ST_HEADER_SIZE];
             st_header[0] = '\0';
             if( upnpBuff.headerValue_P(ST_HEADER,st_header,ST_HEADER_SIZE) ) {
               if( strcmp(st_header,ST) == 0) {  
/**                
 *               All LSC Devices MUST have a DESC Header in the response
 */
                 char name[32];
                 if( upnpBuff.displayName(name,32) ) handler(&upnpBuff);
                 else if( loggingLevel(FINE) ) Serial.printf("SSDP::searchRequest: DESC Header not found\n");
               }
               else if( loggingLevel(FINE) ) Serial.printf("SSDP::searchRequest: Search Response %s does not match request %s\n",st_header,ST);
             }
           }
        }
        delay(100);
      }
      udp.stop();
    }
  }
  return result;
}


/**  Read UDP Channel and respond according to the ST and ST.LEELANAUSOFTWARE.COM headers  
 *   
 *     ST:  upnp:rootdevice        Responds once for each root device
 *     ST.LEELANAUSOFTWARE.COM:  ssdp:all (or empty)
 *     or
 *     ST:  uuid:root-device-UUID  Root Devices respond if uuid matches
 *     ST.LEELANAUSOFTWARE.COM:  Empty but required
 *         
 */

boolean SSDP::readChannel(WiFiUDP& channel) {
  boolean   result       = false;
  IPAddress remoteAddr   = channel.remoteIP();
  int       port         = channel.remotePort();

/** We use a post handler so we don't have to hold a both read buffer and write buffer in memory simultaneously.
 *  Post handler defaults to do nothing.
 */
  setPostHandler([]{});
  
//  read the packet into readBufffer
  char txnBuffer[TXN_BUFFER_SIZE + 1];
  txnBuffer[0] = 0;
  int available = channel.read(txnBuffer, TXN_BUFFER_SIZE);
  txnBuffer[available] = 0;
  UPnPBuffer buffer = UPnPBuffer(txnBuffer);

  if( buffer.isSearchRequest() ) {
    char st_lsc_header[ST_LSC_HEADER_SIZE];
    st_lsc_header[0] = '\0';
    if( buffer.headerValue_P(ST_LSC_HEADER,st_lsc_header,ST_LSC_HEADER_SIZE) ) {  // If the packet has an LSC header field
       char st_header[ST_HEADER_SIZE];
       st_header[0] = '\0';
       if( buffer.headerValue_P(ST_HEADER,st_header,ST_HEADER_SIZE) ) { // If the packet has an ST header field  
          if( strncmp_P(st_header,ST_UPNP_ROOTDEVICE,15) == 0 ) { // If this is a Root Device search
             result = true;
             if(strncmp_P(st_lsc_header,SSDP_ALL,8) == 0) setPostHandler([this,st_header,remoteAddr,port]{this->postAllResponse(_root,st_header,remoteAddr,port);});
             else setPostHandler([this,st_header,remoteAddr,port]{this->postDeviceResponse(_root,st_header,remoteAddr,port);});
           }
           else if( strncmp_P(st_header,ST_UUID,5) == 0 ) { // If this is a search by UUID
             char uuid[UUID_SIZE];
             // Remove any leading blank chars
             char* uuidBuff = st_header + 5;             
             while( *uuidBuff  == ' ') {uuidBuff++;} 
             strncpy(uuid,uuidBuff,UUID_SIZE);  
             UPnPDevice* device = _root->getDevice(uuid);
             if( device != NULL ) {
                result = true;
                if(strncmp_P(st_lsc_header,SSDP_ALL,8) == 0) setPostHandler([this,device,st_header,remoteAddr,port]{this->postAllResponse(device,st_header,remoteAddr,port);});
                else setPostHandler([this,device,st_header,remoteAddr,port]{this->postDeviceResponse(device,st_header,remoteAddr,port);});
             } 
             else if( loggingLevel(FINE) ) Serial.printf("SSDP::readChannel: device with uuid [%s] does not exist\n",uuid);    
          }
          else if(strncmp_P(st_header,ST_TYPE,4) == 0) { // If this is a search by device/service type
            result = true;      
            setPostHandler([this,st_header,remoteAddr,port]{this->postAllMatching(_root,st_header,remoteAddr,port);});
          }
       }
       else if( loggingLevel(FINE) ) Serial.printf("SSDP::readChannel: Packet does not have ST header\n");
    }
  }  
  return result;  
}

void SSDP::doChannel(WiFiUDP& channel) {
/**
 * if there's data available, read a packet. If a response is required, post it.
 */
  int packetSize = channel.parsePacket();
  boolean reply = false;
  if (packetSize) {
    reply = readChannel(channel);
  }
  if( reply ) {
    _postHandler();
  }
}

/**
 *      ST: 
 *      USN: service USN
 *      UPN.LEELANAUSOFTWARE.COM: service UPN
 *      DESC.LEELANAUSOFTWARE.COM: devices:num-devices:services:num-services on a device response and not present on a service response
 *      
 *   
 */
void SSDP::postDeviceResponse(UPnPDevice* d, const char* st, IPAddress remoteAddr, int port) {
  char txnBuffer[TXN_BUFFER_SIZE + 1];
  txnBuffer[0] = '\0';
  RootDevice* r = d->asRootDevice();
  UPnPDevice* p = d->parentAsDevice();

/**  
 *  Device location is set to the network adapter receiving the incoming request (either localIP or softAPIP)
 */
  IPAddress ifc = interfaceAddress(remoteAddr);
  
/**
 *   If this device is a RootDevice use the Root template, otherwise use the Device template
 *   Note that RootDevice location does not include the root target, so display will default to RootDevice::displayRoot()
 */
  char locBuff[128];
  locBuff[0] = '\0';
  if( r != NULL ) r->rootLocation(locBuff,128,ifc);
  else d->location(locBuff,128,ifc); 
  
  if( (r != NULL) ) 
    snprintf_P(txnBuffer,TXN_BUFFER_SIZE,ROOT_RESPONSE,locBuff,st,d->uuid(),d->getType(),d->getDisplayName(),r->numDevices(),r->numServices());  
  else if( p != NULL ) 
    snprintf_P(txnBuffer,TXN_BUFFER_SIZE,DEVICE_RESPONSE,locBuff,st,d->uuid(),d->getType(),d->getDisplayName(),d->numServices(),p->uuid());
  else 
    snprintf_P(txnBuffer,TXN_BUFFER_SIZE,ROOT_RESPONSE,locBuff,st,d->uuid(),d->getType(),d->getDisplayName(),0,d->numServices()); // Error state, non-root should have a parent

  int len = strlen(txnBuffer);
  int ok = _udp.beginPacket(remoteAddr, port);
  if( ok != 1 ) {
    if( loggingLevel(WARNING) ) Serial.printf("postDeviceResponse: Error on beginPacket\n");
  }
  int sz = _udp.write((unsigned char*)txnBuffer,len);
  ok = _udp.endPacket();
  if( ok != 1 ) {
    if( loggingLevel(WARNING) ) Serial.printf("postDeviceResponse: Error on endPacket attempt to send %d bytes\",len");
  }
  delay(500);
}

void SSDP::postServiceResponse(UPnPService* s, const char* st, IPAddress remoteAddr, int port ) {
/**  
 *  Service location is set to the network adapter receiving the incoming request (either localIP or softAPIP)
 */
  IPAddress ifc = interfaceAddress(remoteAddr);
  
  char txnBuffer[TXN_BUFFER_SIZE + 1];
  txnBuffer[0] = '\0';
  UPnPDevice* p = s->parentAsDevice();
  char locBuff[128];
  locBuff[0] = '\0';
  s->location(locBuff,128,ifc);
  if( p != NULL ) {
    snprintf_P(txnBuffer,TXN_BUFFER_SIZE,SERVICE_RESPONSE,locBuff,st,s->getType(),p->uuid(),s->getDisplayName(),p->uuid());
  }
  int len = strlen(txnBuffer);
 
  int ok = _udp.beginPacket(remoteAddr, port);
  if( ok != 1 ) {
    if( loggingLevel(WARNING) ) Serial.printf("postServiceResponse: Error on beginPacket\n");
  }
  int sz = _udp.write((unsigned char*)txnBuffer,len);
  ok = _udp.endPacket();  
  if( ok != 1 ) {
    if( loggingLevel(WARNING) ) Serial.printf("postServiceResponse: Error on endPacket attempt to send %d bytes\n",len);
  }
  delay(500);
}

void SSDP::postAllResponse(UPnPDevice* d, const char* st, IPAddress remoteAddr, int port ) {
  postDeviceResponse(d, st, remoteAddr, port );
  UPnPService** services = d->services();
  for(int i=0; i<d->numServices(); i++ ) {
    postServiceResponse(services[i],st,remoteAddr, port);
  }
  RootDevice* r = d->asRootDevice();
  if( r != NULL ) {
    UPnPDevice** devices = r->devices();
    for( int i=0; i<r->numDevices(); i++ ) {
      postAllResponse(devices[i],st,remoteAddr, port);
    }
  }
}

void SSDP::postAllMatching(UPnPDevice* d, const char* st, IPAddress remoteAddr, int port ) {
  if( loggingLevel(FINEST) ) {
    Serial.printf("SSDP::postAllMatching: Searching for device or service %s\n", st);
    Serial.printf("                       Device type is  %s\n", d->getType());
    if( d->isType(st) )  Serial.printf("                       Device type is a match, posting response\n");
    else  Serial.printf("                       Device type is NOT a match\n");
  }
  if(d->isType(st)) postDeviceResponse(d, st, remoteAddr, port );
  UPnPService** services = d->services();
  Serial.printf("                       Searching services for device type %s\n", d->getType());
  for(int i=0; i<d->numServices(); i++ ) {
    if( loggingLevel(FINEST) ) {
       Serial.printf("                            Service type is  %s\n", services[i]->getType());
       if( services[i]->isType(st) )  Serial.printf("                            Service type is a match, posting response\n");
       else  Serial.printf("                            Service type is NOT a match\n");
    }
    if( services[i]->isType(st) ) postServiceResponse(services[i],st,remoteAddr,port);
  }
  RootDevice* r = d->asRootDevice();
  if( r != NULL ) {
    UPnPDevice** devices = r->devices();
    for( int i=0; i<r->numDevices(); i++ ) {
      postAllMatching(devices[i],st,remoteAddr,port);
    }
  }
}

boolean SSDP::isLocalIP(IPAddress address) {
  IPAddress local_IP     = WiFi.localIP();
  IPAddress subnet       = WiFi.subnetMask();
  uint32_t  localIPMask  = ((uint32_t)local_IP)  & ((uint32_t)subnet);
  uint32_t  addr         = (uint32_t) address;
  return((addr&localIPMask)!=0);  
}

boolean SSDP::isSoftAPIP(IPAddress address) {
  IPAddress softAP_IP    = WiFi.softAPIP();
  IPAddress subnet       = WiFi.subnetMask();
  uint32_t  softAPIPMask = ((uint32_t)softAP_IP) & ((uint32_t)subnet);
  uint32_t  addr         = (uint32_t) address;
  return((addr&softAPIPMask)!=0); 
}

IPAddress SSDP::interfaceAddress(IPAddress address) {
  IPAddress local_IP     = WiFi.localIP();
  IPAddress softAP_IP    = WiFi.softAPIP();
  IPAddress subnet       = WiFi.subnetMask();
  uint32_t  localIPMask  = ((uint32_t)local_IP)  & ((uint32_t)subnet);
  uint32_t  softAPIPMask = ((uint32_t)softAP_IP) & ((uint32_t)subnet);
  uint32_t  addr         = (uint32_t) address;
  if((addr&localIPMask)!=0) return local_IP;
  else if((addr&softAPIPMask)!=0) return softAP_IP;
  else return IPADDR_ANY;
}

} // End of namespace lsc
