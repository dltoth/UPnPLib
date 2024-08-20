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

#ifndef UPNPBUFFER_H
#define UPNPBUFFER_H

#include <Arduino.h>
#include <ctype.h>

/** Leelanau Software Company namespace 
*  
*/
namespace lsc {
  
class UPnPBuffer {
  public:
  UPnPBuffer(const char* buff);                     // Construct with with null terminated packet buffer

//  Return false if header does not exist, otherwise return true with header value filled in buffer
    boolean headerValue(const char* header, char buffer[], size_t len); 
    boolean headerValue_P(PGM_P header, char buffer[], size_t len); 
    
    boolean displayName(char buffer[], size_t len); // Return true if DESC header is present and fill buffer with the :name: value                       
    
    boolean isSearchRequest();                      // Return true if this message is a Search Request
    boolean isSearchResponse();                     // Return true if this message is a Search Response

/** Line processing
 *  
 */
    int         maxLineLength();
    const char* getNextLine(const char* lineStart, char buffer[], size_t bufferLen);
    boolean     hasNextLine(const char* startLine);
                                             
  private:
    const char*   _buffer;
    int           _maxLen = 0;

    int           maxLen();

};

} // End of namespace lsc

#endif
