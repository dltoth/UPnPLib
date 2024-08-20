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
 * ssdp.h
 *
 *  This protocol is not strictly SSDP, but rather an abbreviated version of the protocol with the following goals:
 *  (1) Reduce chattiness of standard UPnP/SSDP by only responding to known search requests
 *  (2) Provide enough information to populate a device hierarchy of the environment and
 *  (3) Allow query to see if root devices are still available on the network
 *  (4) Find instances of a specific Device (or Service) type on the network
 *  Search requests are sent out over the multicast address 239.255.255.250 port 1900 and responses are sent to the unicast
 *  IP address and port of the request.
 * 
 *  Unique Service Name - USN is always uuid:device-UUID::urn:domain-name:device:deviceType:ver for a device or
 *                                      uuid:device-UUID::urn:domain-name:service:serviceType:ver:serviceID for a service
 *                                      where serviceID is a unique service identifier within it's device
 *  Unique Parent Name  - UPN.LEELANAUSOFTWARE.COM is always the USN of the parent of an embedded device or
 *                                      upnp:rootdevice if the device is a root device
 */
 
#ifndef SSDP_H
#define SSDP_H

#include <ctype.h>
#include "UPnPBuffer.h"

#ifdef ESP8266
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif

#include <WiFiUdp.h>
#include "UPnPDevice.h"

/** Leelanau Software Company namespace 
*  
*/
namespace lsc {

#define LOC_SIZE 32

#ifndef NULL
#define NULL 0
#endif

#ifndef EMPTY_STRING
#define EMPTY_STRING {""}
#endif

#define UDP_PORT   1900                // local UDP port to listen on

typedef enum {
  SSDP_OK = 0,
  SSDP_ERR_UDP = 1,
  SSDP_ERR_SEND = 2,
  SSDP_ERR_ST = 3
} SSDPResult;

typedef std::function<void(UPnPBuffer*)> SSDPHandler;

class SSDP {

  public:
  SSDP();
  virtual ~SSDP() {_udp.stop();_mUdp.stop();}
  
  void         begin(RootDevice* root);                  // RootDevice to handle search requests
  void         doSSDP();                                 // Read both Unicast and Multicast UDP channels and respond accordingly
  int          getUDPPort();                             // Return unicast UDP channel port
  int          getMulticastPort();                       // Return Multicast UDP channel port
  
  static boolean   isLocalIP(IPAddress addr);            // Return true if addr is on the localIP network
  static boolean   isSoftAPIP(IPAddress addr);           // Return true if addr is on the softAPIP network
  static IPAddress interfaceAddress(IPAddress addr);     // Return the network interface (either local or softAP) of addr

/**
 *  Send an SSDP Search request and parse responses for timeout milliseconds.
 *  Each response is handed to an SSDPHandler for processing.
 *  Input Parameters:
 *     ST      - Search Target MUST be one of the following:
 *                 upnp:rootdevice
 *                 uuid:Device-UUID                              For example - uuid: b2234c12-417f-4e3c-b5d6-4d418143e85d
 *                 urn:domain-name:device:deviceType:ver         For example - urn:LEELANAUSOFTWARE-com:device:SoftwareClock:1
 *                 urn:domain-name:service:serviceType:ver       For example - urn:LEELANAUSOFTWARE-com:service:GetDateTime:1
 *     handler - An SSDPHandler function called on each response to the request
 *     ifc     - The network interface to bind the request to (either WiFi.localIP() or WiFi.softAPIP())
 *     timeout - Listen for responses for timeout milliseconds and then return to caller. If ST is uuid:Devce-UUID, processing returns
 *               after the specific device responds or timeout expires, otherwise processing returns after timeout milliseconds.
 *     ssdpAll - Applies only to upnp:rootdevice searches, if true, ALL RootDevices, embedded UPnPDevices, 
 *               and UPnPServices respond, otherwise only RootDevices respond.
 */
  static SSDPResult      searchRequest(const char* ST, SSDPHandler handler, IPAddress ifc, int timeout=2000, boolean ssdpAll=false);

/**
 *  Set/Get/Check Logging Level. Logging Level can be NONE, INFO, FINE, and FINEST
 */
  static void             logging(LoggingLevel level)             {_logging = level;}
  static LoggingLevel     logging()                               {return _logging;}
  static boolean          loggingLevel(LoggingLevel level)        {return(logging() >= level);}

  private:
  RootDevice*                _root;                      // RootDevice to expose through SSDP
  WiFiUDP                    _mUdp;                      // Multicast Discovery
  WiFiUDP                    _udp;                       // Unicast Discovery and resopnse
  static LoggingLevel        _logging;
  
  std::function<void(void)>  _postHandler = []{};


  void      doChannel(WiFiUDP& channel);                                                          // Check for incoming search requests and respond
  void      setPostHandler(std::function<void(void)> handler) {_postHandler = handler;}           // Set post response handler
  boolean   readChannel(WiFiUDP& channel);                                                        // Read bytes from channel, returns true if response required
  void      postAllResponse(UPnPDevice* d, const char* st, IPAddress remoteAddr, int port );      // post search response for all embedded devices and services
  void      postAllMatching(UPnPDevice* d, const char* st, IPAddress remoteAddr, int port );      // post search response for matching devices and services
  void      postAllReverse(UPnPDevice* d, const char* st, IPAddress remoteAddr, int port );       // post search all response in reverse
  void      postDeviceResponse(UPnPDevice* d, const char* st, IPAddress remoteAddr, int port );   // post search response for device, returns USN
  void      postServiceResponse(UPnPService* s, const char* st, IPAddress remoteAddr, int port ); // post search response for service

};

} // End of namespace lsc

#endif
