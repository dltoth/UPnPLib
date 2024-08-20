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

#include "UPnPService.h"
/** Leelanau Software Company namespace 
*  
*/
namespace lsc {

/**
 *  Static initializers for runtime type identification
 */
int ClassType::_numTypes = 0;

/**
 *   Static initialization for UPnP device type
 */
INITIALIZE_SERVICE_TYPES(UPnPService,LeelanauSoftware-com,Basic,1.0.0);
INITIALIZE_DEVICE_TYPES(UPnPObject,LeelanauSoftware-com,Object,1.0.0);

UPnPObject::UPnPObject() {
  _target[0]      = '\0';
  strlcpy(_displayName," ", sizeof(_displayName));  // Display name defaults to blank
}

void UPnPObject::setDisplayName(const char* name) {strlcpy(_displayName, name, sizeof(_displayName));}

/** 
 *  Target is the relative URL for this Object (RootDevice, Device, or Service). The complete URL can be constructed as 
 *  "/rootTarget/deviceTarget/serviceTarget" or "/rootTarget/serviceTarget". The complete URL is used at RootDevice setup
 *  in setting HTTP handlers, so target must be set prior to RootDevice setup.
 */
void UPnPObject::setTarget(const char* target) {
  if( target[0] == '/' ) strlcpy(_target, target+1, sizeof(_target));
  else strlcpy(_target, target, sizeof(_target));
}

RootDevice* UPnPObject::rootDevice() {
  UPnPObject* p = getParent();
  UPnPObject* result = this;
  while( p != NULL ) {
    result = p;
    p = result->getParent();
  }
  return result->asRootDevice();
}


/**  Version is the 4'th token in the UPnP Type string, copy it into buffer.
 *   UPnP device Type is of the form urn:domain-name:device:deviceType:ver
 *   and UPnP service type is of the form urn:domain-name:service:serviceType:ver
 */
void UPnPObject::getVersion(char buffer[], size_t size) {
   buffer[0]  = 0;
   URNTokenIterator it(getType());
   URNToken tkn = it[4];
   tkn.getToken(buffer,size);
}

/**  Domain is the 1'th token in the UPnP Type string, copy it into buffer.
 *   UPnP device Type is of the form urn:domain-name:device:deviceType:ver
 *   and UPnP service type is of the form urn:domain-name:service:serviceType:ver
 */
void UPnPObject::getDomain(char buffer[], size_t size) {
   buffer[0]  = 0;
   URNTokenIterator it(getType());
   URNToken tkn = it[1];
   tkn.getToken(buffer,size);
}

/**  Domain is the 3'rd token in the UPnP Type string, copy it into buffer.
 *   UPnP device Type is of the form urn:domain-name:device:deviceType:ver
 *   and UPnP service type is of the form urn:domain-name:service:serviceType:ver
 */
void UPnPObject::getUPnPType(char buffer[], size_t size) {
   buffer[0]  = 0;
   URNTokenIterator it(getType());
   URNToken tkn = it[3];
   tkn.getToken(buffer,size);
}

void UPnPObject::getPath(char buffer[], size_t size) {
  buffer[0] = '\0';
  if( getParent() != NULL ) {
    UPnPObject* d = getParent();
    if( d->getParent() != NULL ) {                                                      // Object is a UPnPService
       UPnPObject* r = d->getParent();
       snprintf(buffer,size,"/%s/%s/%s",r->getTarget(),d->getTarget(),getTarget());     // Location is /rootTarget/deviceTarget/serviceTarget
    }
    else snprintf(buffer,size,"/%s/%s",d->getTarget(),getTarget());                     // Object is a UPnPDevice, location is /rootTarget/deviceTarget
  }
  else snprintf(buffer,size,"/%s",getTarget());                                         // Object is a RootDevice, location is /rootTarget
}

void UPnPObject::handlerPath(char buffer[], size_t bufferSize, const char* handlerName) {
  getPath(buffer,bufferSize);
  int len = strlen(buffer);
  int size = bufferSize - len;
  snprintf(buffer+len,size,"/%s",handlerName);
}

/** % encodes ULR string
 *   / encodes to %2F
 *   ? encodes to %3F
 *   = encodes to %3D
 *   & encodes to %26
 */
void UPnPObject::encodePath(char buffer[], size_t bufferSize, const char* path) {
  int pathSize = strlen(path);
  int j = 0;
  for(int i=0; (j<bufferSize)&&(i<pathSize); i++) {
     if( path[i] == '/' )     {buffer[j++]='%';buffer[j++]='2';buffer[j++]='F';}
     else if(path[i] == '?')  {buffer[j++]='%';buffer[j++]='3';buffer[j++]='F';}
     else if(path[i] == '=')  {buffer[j++]='%';buffer[j++]='3';buffer[j++]='D';}     
     else if(path[i] == '&')  {buffer[j++]='%';buffer[j++]='2';buffer[j++]='6';}
     else if(path[i] == '+')  {buffer[j++]='%';buffer[j++]='2';buffer[j++]='0';}
     else buffer[j++] = path[i];
  }
  if( j<bufferSize ) buffer[j] = '\0';
  else buffer[bufferSize-1] = '\0';
}

void UPnPService::location(char buffer[], int buffSize, IPAddress ifc) {
  UPnPObject* p = getParent();
  if( p != NULL ) {
    p->location(buffer,buffSize,ifc);
    int pos = strlen(buffer);
    int size = buffSize-pos;
    snprintf(buffer+pos,size,"/%s",getTarget());
  }
  else snprintf(buffer, buffSize,"/%s",getTarget());
}

void  UPnPService::setup(WebContext* svr) {
  char pathBuffer[100];
  getPath(pathBuffer,100);
  svr->on(pathBuffer,[this](WebContext* svr){this->handleRequest(svr);});
}

} // End of namespace lsc
