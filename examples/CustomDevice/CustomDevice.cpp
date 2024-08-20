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

#include "CustomDevice.h"
const char html_template[]        PROGMEM = "<br><br><p align=\"center\">Custom Device Display</p><br>";
const char root_html_template[]   PROGMEM = "<br><br><p align=\"center\">Custom Device Root Display</p><br>";
INITIALIZE_STATIC_TYPE(CustomDevice);
INITIALIZE_UPnP_TYPE(CustomDevice,urn:CompanyName-com:device:CustomDevice:1);

int CustomDevice::formatContent(char buffer[], int size, int pos) {return formatBuffer_P(buffer,size,pos,html_template,getDisplayName());}
int CustomDevice::formatRootContent(char buffer[], int size, int pos) {return formatBuffer_P(buffer,size,pos,root_html_template,getDisplayName());}
