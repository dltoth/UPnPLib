/**
 * 
 *  UPnPLib Library
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

#include <UPnPLib.h>


/**
 *   Simple example to make a device visible on a local network using SSDP with a custom HTML UI. 
*    RootDevice will respond to SSDP queries and provide the location of its HTML UI
 */


#define AP_SSID "MySSID"
#define AP_PSK  "MyPSK"

#define SERVER_PORT 80

/**
 *   Conditional compilation for either ESP8266 or ESP32
 */
#ifdef ESP8266
#include <ESP8266WiFi.h>
#define           BOARD "ESP8266"
#elif defined(ESP32)
#include <WiFi.h>
#define          BOARD "ESP32"
#endif

const char html_template[]   PROGMEM = "<!DOCTYPE html><html><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
                                       "<head><link rel=\"stylesheet\" type=\"text/css\" href=\"/styles.css\"></head>"
                                       "<body style=\"font-family: Arial\">"
                                         "<H3 align=\"center\"> %s </H3><br><br>"
                                         "<div align=\"center\">"
                                            "<br><br><p align=\"center\">Hello from Root Device</a></p><br>"
                                         "</div>"
                                       "</body></html>";

/**
 *   Device hierarchy will consist of a single RootDevice (root)
 */
WebContext       ctx;
RootDevice       root;
SSDP             ssdp;

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.flush();
  Serial.println();
  Serial.printf("Starting UPnPDevice Test for Board %s\n",BOARD);

  WiFi.begin(AP_SSID,AP_PSK);
  Serial.printf("Connecting to Access Point %s\n",AP_SSID);
  while(WiFi.status() != WL_CONNECTED) {Serial.print(".");delay(500);}

  Serial.printf("WiFi Connected to %s with IP address: %s\n",WiFi.SSID().c_str(),WiFi.localIP().toString().c_str());

  ctx.begin(SERVER_PORT);
  Serial.printf("Web Server started on %s:%d/\n",WiFi.localIP().toString().c_str(),ctx.getLocalPort());


  root.setDisplayName("Root Device");
  root.setTarget("root");  
  root.setup(&ctx);

/**
 *  Print UPnPDevice info to Serial
 */
  UPnPDevice::printInfo(&root);  

/**
 *  Define a display handler for the RootDevice that provides an HTML UI
 */
  DisplayHandler rootDisplay = [](UPnPDevice* d,WebContext* svr){
   char buffer[DISPLAY_SIZE];
   int size = sizeof(buffer);
   int pos = 0;
   char path[100];
   RootDevice* rootPtr = d->asRootDevice();
   if( rootPtr != NULL ) {
     pos = formatBuffer_P(buffer,size,pos,html_template,rootPtr->getDisplayName());   
     svr->send(200,"text/html",buffer);
   }
  };
  root.setDisplayHandler(rootDisplay);
  root.setRootDisplayHandler(rootDisplay);

}

void loop() {
  ctx.handleClient();
  ssdp.doSSDP();
  root.doDevice();
}

