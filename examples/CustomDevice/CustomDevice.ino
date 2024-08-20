/**
 * 
 *  UPnPLib Library
 *  Copyright (C) 2024  Daniel L Toth
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
#include "CustomDevice.h"

/**
 *   Simple example to make a device visible on a local network using SSDP with a custom HTML UI. 
 *   RootDevice will respond to SSDP queries and provide the location of its HTML UI
 */

#define AP_SSID "MySSID"
#define AP_PSK  "MyPSK"

#define SERVER_PORT 80

/**
 *   Device hierarchy will consist of a RootDevice (root) and a single CustomDevice (d)
 */
WebContext       ctx;
RootDevice       root;
CustomDevice     d;
SSDP             ssdp;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.flush();
  Serial.println();
  Serial.printf("Starting CustomDevice\n");

  WiFi.begin(AP_SSID,AP_PSK);
  Serial.printf("Connecting to Access Point %s\n",AP_SSID);
  while(WiFi.status() != WL_CONNECTED) {Serial.print(".");delay(500);}

  Serial.printf("WiFi Connected to %s with IP address: %s\n",WiFi.SSID().c_str(),WiFi.localIP().toString().c_str());

  ctx.begin(SERVER_PORT);
  Serial.printf("Web Server started on %s:%d/\n",WiFi.localIP().toString().c_str(),ctx.getLocalPort());

/**
 *  Initialize SSDP services
 */
  ssdp.begin(&root);

  root.setDisplayName("Root Device");
  root.setTarget("root");  
  root.setup(&ctx);
  root.addDevice(&d);

/**
 *  Print UPnPDevice info to Serial
 */
  UPnPDevice::printInfo(&root);  

}

void loop() {
  ctx.handleClient();
  ssdp.doSSDP();
  root.doDevice();
}

