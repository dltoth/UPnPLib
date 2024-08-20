/**
 * 
 *  UPnPDevice Library
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

#include "UPnPDevice.h"

/** Leelanau Software Company namespace 
*  
*/
namespace lsc {
  
/**
 *  Static initializers for runtime type identification and UPnP device type
 */
INITIALIZE_DEVICE_TYPES(UPnPDevice,LeelanauSoftware-com,Basic,1.0.0);
INITIALIZE_DEVICE_TYPES(RootDevice,LeelanauSoftware-com,RootDevice,1.0.0);

void getRandomBytes(unsigned char *a, int len) {for(int i=0; i<len; i++ ) a[i] = (unsigned char) rand()%255;}

void generateUUID(char s[UUID_SIZE])
{
  unsigned char uuid[16];
  getRandomBytes(uuid,16);
  
// Set UUID version to 4 --- truly random generation
  uuid[6] = (uuid[6] & 0x0F) | 0x40;
// Set the UUID variant to DCE 
  uuid[8] = (uuid[8] & 0x3F) | 0x80;

// A UUID string is of the form:
// xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
// where x is a hex digit
  sprintf(s,"%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",uuid[0],uuid[1],uuid[2],uuid[3],uuid[4],uuid[5],uuid[6],uuid[7],uuid[8],uuid[9],uuid[10],uuid[11],uuid[12],uuid[13],uuid[14],uuid[15]);
  s[UUID_SIZE-1] = '\0';
}

/** Return true if the UUID is valid.
 *  A valid UUID is of the form:
 *  xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
 *  where x is a hex digit
 * 
 */
bool isValidUUID(String uuidStr)
{
  const char* uuid = uuidStr.c_str();
  if( uuidStr.length() != UUID_SIZE-1 ) return false;

  for (int i = 0; i < UUID_SIZE-1; i++) {
    if (i == 8 || i == 13 || i == 18 || i == 23) {
      if (uuid[i] != '-')
        return false;
    } else if (!isxdigit(uuid[i])) {
      return false;
    }
  }
  return true;
}

UPnPDevice::UPnPDevice() {
  _uuid[0] = '\0';
  setDisplayName("Device");
}

UPnPDevice::UPnPDevice(const char* target) : UPnPObject(target) {
  _uuid[0] = '\0';
  setDisplayName("Device");
}

/**
 *  Default display on RootDevice is an app button that links to device display
 */
int UPnPDevice::formatRootContent(char buffer[], int size, int pos) {
  char pathBuff[100];
  getPath(pathBuff,100);
  pos = formatBuffer_P(buffer,size,pos,app_button,pathBuff,getDisplayName()); 
  return pos;     
}

void UPnPDevice::display(WebContext* svr) {
  if( _displayHandler != NULL ) _displayHandler(this,svr);
  else {
     char buffer[DISPLAY_SIZE];
     int size = sizeof(buffer);
     int pos = formatHeader(buffer,size,getDisplayName());
     pos = formatContent(buffer,size,pos);
     formatTail(buffer,size,pos);
     svr->send(200,"text/html",buffer);
  }
}

void UPnPDevice::setup(WebContext* svr) {
  char pathBuffer[100];
  pathBuffer[0] = '\0';
  getPath(pathBuffer,100);
  svr->on(pathBuffer,[this](WebContext* svr){this->display(svr);});
  for( int i=0; i<numServices(); i++ ) {service(i)->setup(svr);}
}

/** Set UUID to uuid if uuid is valid
 *  returns true if uuid is valid and false otherwise
 */
boolean UPnPDevice::setUUID( String uuid ) {
  if( isValidUUID(uuid) ) {
    strlcpy(_uuid, uuid.c_str(), sizeof(_uuid));
    return true;
  }
  else return false;
}

/** Add a UPnPService to this device
 *  If a target hasn't been set yet, set a default target as "serviceN" where N is it's position in the _services array
 * 
 */
void UPnPDevice::addService(UPnPService* svc) {
  if( svc != NULL ) {
     if(_numServices < MAX_SERVICES ) {
         if( strlen(svc->_target) == 0 ) sprintf(svc->_target,"service%d",_numServices);
         _services[_numServices++] = svc;
         svc->setParent(this);
/**
 *     Late binding setup. If this device has already been added to a RootDevice, and setup() has 
 *     already been called on that RootDevice, any added service must also be setup();
 */
       if(rootDevice() != NULL) {
           if( rootDevice()->getContext() != NULL ) svc->setup(rootDevice()->getContext());
       }
     }
  }
}

void UPnPDevice::location(char buffer[], int buffSize, IPAddress ifc) {
  UPnPObject* p = getParent();
  if( p != NULL ) {
    p->location(buffer,buffSize,ifc);
    int pos = strlen(buffer);
    int size = buffSize-pos;
    snprintf(buffer+pos,size,"/%s",getTarget());
  }
  else snprintf(buffer, buffSize,"/%s",getTarget());
}

uint64_t getChipID() {
  uint64_t result = 0;
#ifdef ESP32
  result = ESP.getEfuseMac();
#elif defined(ESP8266)
  result = (uint64_t)ESP.getChipId();
#endif
  return result;
}

void UPnPDevice::printInfo(UPnPDevice* d) {
  RootDevice* r = (RootDevice*)(d->asRootDevice());
  if( r != NULL ) Serial.printf("RootDevice %s:\n   UUID: %s\n   Type: %s\n",d->getDisplayName(),d->uuid(),d->getType());
  else Serial.printf("%s:\n   UUID: %s\n   Type: %s\n",d->getDisplayName(),d->uuid(),d->getType());
  char buffer[128];
  d->location(buffer,128,WiFi.localIP());
  Serial.printf("   Location is %s\n",buffer);
  if( d->numServices() > 0 ) Serial.printf("   %s Services:\n",d->getDisplayName());
  else Serial.printf("   %s has no Services\n",d->getDisplayName());
  for(int i=0; i<d->numServices(); i++) {
    UPnPService* s = d->service(i);
    buffer[0] = '\0';
    s->location(buffer,128,WiFi.localIP());
    Serial.printf("      %s:\n         Type: %s\n         Location is %s\n",s->getDisplayName(),s->getType(),buffer);
  }
  if( r != NULL ) {
    if( r->numDevices() > 0 ) Serial.printf("%s Devices:\n",r->getDisplayName());
    else Serial.printf("%s has no Devices\n",r->getDisplayName());
    for(int i=0; i<r->numDevices(); i++) printInfo(r->device(i));    
  } 
}

RootDevice::RootDevice() : UPnPDevice("root") {
  srand(getChipID());
  generateUUID(_uuid);
  setDisplayName("Root Device");
}

RootDevice::RootDevice(const char* target) : UPnPDevice(target) {
  srand(getChipID());
  generateUUID(_uuid);
  setDisplayName("Root Device");
}

/**
 *  Format content for display at root target
 */
int RootDevice::formatContent(char buffer[], int size, int pos) {
  char pathBuff[100];
  for( int i=0; (i<_numDevices) && (size>0); i++ ) {
    UPnPDevice* d = device(i);
    if( d != NULL ) {
       d->getPath(pathBuff,100);
       pos = formatBuffer_P(buffer,size,pos,app_button,pathBuff,d->getDisplayName());
    }
  }
  return pos;
}

int RootDevice::formatRootContent(char buffer[], int size, int pos ) {
/** 
 *  Add Content from each embedded device
 */
  for( int i=0; (i<numDevices()) && (pos<size); i++ ) {
    UPnPDevice* d = device(i);
    if( d != NULL ) pos = d->formatRootContent(buffer,size,pos);
  }

/**
 *   Add a "This Device" button
 */
  char buff[100];
  getPath(buff,100);  
  pos = formatBuffer_P(buffer,size,pos,app_button,buff,"This Device"); 

  return pos;
}

void RootDevice::displayRoot(WebContext* svr) {  
  Serial.printf("RootDevice::rootDisplay called");
  if( _rootDisplayHandler != NULL ) _rootDisplayHandler(this,svr);
  else {
    char buffer[DISPLAY_SIZE];
    int size = sizeof(buffer);
    int pos = formatHeader(buffer,size,getDisplayName());        //Add HTML Header Title with Display Name
    pos = formatRootContent(buffer,size,pos);                    // Add root display content
    formatTail(buffer,size,pos);                                 // Add the HTML tail
    svr->send(200,"text/html",buffer);
  }
}

void RootDevice::setup(WebContext* svr) {
  UPnPDevice::setup(svr);
  _context = svr;
  char pathBuffer[50];
  svr->on("/styles.css",[this](WebContext* s){this->styles(s);});
  svr->on("/",[this](WebContext* s){this->displayRoot(s);});
  for( int i=0; i<_numDevices; i++ )   {device(i)->setup(svr);}
}

/** Add a UPnPDevice to this root device
 *  If a target hasn't been set on the device, set a default target as "deviceN" where N is it's position in the _devices 
 *  array. If context has been set on RootDevice, then setup has been called. Devices added after RootDevice setup also  
 *  have to be setup().
 */
void RootDevice::addDevice(UPnPDevice* dvc) {
  if( dvc != NULL ) {
     if(_numDevices < MAX_DEVICES) {
       if( strlen(dvc->_target) == 0 ) sprintf(dvc->_target,"device%d",_numDevices);
       if( strlen( dvc->_uuid ) == 0 ) generateUUID(dvc->_uuid);
       _devices[_numDevices++] = dvc;
       dvc->setParent(this);
/**
 *     Late binding setup. Setup() has already been called on this RootDevice so any device added
 *     must also be setup();
 */
       if(getContext() != NULL) dvc->setup(getContext());
     }
  }
}

/**
 *  If RootDevice is ClassType t, RootDevice is returned. Otherwise, searches embedded devices for a UPnPDevice
 *  of ClassType t and returns the first matching device. If none are found, returns NULL.
 */
UPnPDevice* RootDevice::getDevice(const ClassType* t) {
  UPnPDevice* result = NULL;
  if(this->as(t) != NULL) result = this;
  else {
     for( int i=0; (i<numDevices()) && (result == NULL); i++ ) {if(device(i)->as(t) != NULL) result = device(i);}
  }
  return result;
}

/**
 *  If the input uuid matches this RootDevice uuid, return this RootDevice, otherwise search
 *  embedded devices for a match. Returns NULL if none are found.
 */
UPnPDevice* RootDevice::getDevice(const char* u) {
  UPnPDevice* result = NULL;
  if(this->isDevice(u)) result = this;
  else {
     for( int i=0; (i<numDevices()) && (result == NULL); i++ ) {if(device(i)->isDevice(u)) result = device(i);}
  }
  return result;
}

void RootDevice::doDevice() {for( int i=0; i<numDevices(); i++ ) {device(i)->doDevice();}}

void RootDevice::rootLocation(char buffer[], int buffSize, IPAddress ifc) {
  snprintf(buffer,buffSize,"http://%s:%d/",ifc.toString().c_str(),serverPort());
}

void RootDevice::location(char buffer[], int buffSize, IPAddress ifc) {
  snprintf(buffer,buffSize,"http://%s:%d/%s",ifc.toString().c_str(),serverPort(),getTarget());
}

} // End of namespace lsc
