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

#include "UPnPBuffer.h"

namespace lsc {

const char M_SEARCH_HEADER[]     PROGMEM = "M-SEARCH";
const char RESPONSE_HEADER[]     PROGMEM = "HTTP/1.1";
const char DESC_LSC_HEADER[]     PROGMEM = "DESC.LEELANAUSOFTWARE.COM";
const char END_OF_LINE[]         PROGMEM = "\r\n";


UPnPBuffer::UPnPBuffer(const char* buff) {
   // Remove any leading blanks
  const char* cbuff = buff;
  while( *cbuff == ' ' ) {cbuff++;}
  _buffer = cbuff;
  _maxLen = maxLen()+1;
}

/** Copies the header value corresponding to the inpput string header into
 *  the input buffer. At most len characters are copied including the ending '\0'
 *  character. Leading blanks are removed prior to coping.
 */
boolean UPnPBuffer::headerValue(const char* header, char buffer[], size_t len) {
  boolean result = false;
  int headerLen = strlen(header);
  int buffLen = maxLineLength();
  char lineBuff[buffLen];
  const char* lineStart = _buffer;

/** Search _buffer one line at a time, taking care to remove starting blanks.
 *  Note that on initialization, _buffer has no starting blanks and each header should match 
 *  from start of line, with no starting blanks.
 */
  while( hasNextLine( lineStart ) ) {
     lineStart = getNextLine(lineStart,lineBuff,buffLen);
     char* headerStart = strstr(lineBuff,header);
     
/**  If this line begins with the header string and blank or ':', copy header value into buffer
 *   
 */  
     if( headerStart != NULL ) {
       char* headerEnd = headerStart + headerLen;
       if( (headerStart == lineBuff) && ((*headerEnd == ' ')||(*headerEnd == ':')) ) {
          char* headerValue = strstr(headerStart,":");
          if( headerValue != NULL ) {
            result = true;
            headerValue++;
            while(*headerValue == ' ') {++headerValue;}
            buffer[0] = '\0';
            if( strlen(headerValue) > 0 ) {
               headerEnd = headerValue + strlen(headerValue);
               while( *headerEnd == ' ') {headerEnd--;}
               int hlen = headerEnd - headerValue + 1;     // +1 to include null termination on copy
               if( hlen > len ) hlen = len;
               strlcpy(buffer,headerValue,hlen);
            }
          }
       }
     }
  }
  
  return result;   
}

/**
 *  Runs through each line looking for max line length until "\r\n\r\n" is found.
 */
int  UPnPBuffer::maxLen() {
  int result = 0;
  const char* lineStart = _buffer;
  const char* lineEnd   = NULL;
  if( lineStart != NULL ) {
    lineEnd = strstr_P(lineStart,END_OF_LINE);
    int   lineLen   = 0;
    if( lineEnd != NULL ) lineLen = lineEnd - lineStart;
    else {
      lineLen = 0;
      result = strlen(lineStart);
    }
    while( lineLen > 0 ) {
      if( lineLen > result ) result = lineLen;
      lineStart = lineEnd + 2;
      lineEnd = strstr_P(lineStart,END_OF_LINE);
      if( lineEnd != NULL ) lineLen = lineEnd - lineStart;
      else {
        lineLen = strlen(lineStart);
        if( lineLen > result ) result = lineLen;
        lineLen = 0;
      }
    }
  }
  return result;
}

/** Copy the next line into buffer from lineStart up to but not including EOL.
 *  Returns the start of the next line (blanks removed) if EOL is found and NULL otherwise.
 */
const char* UPnPBuffer::getNextLine(const char* lineStart, char buffer[], size_t bufferLen) {
    const char* result = NULL;
    if( (buffer != NULL) && (lineStart != NULL ) ) {
      buffer[0] = '0';
      char* lineEnd = strstr_P(lineStart,END_OF_LINE);
      int   lineLen   = 0;
      if( lineEnd != NULL ) {
        result = lineEnd + 2;
        while(*result==' ') result++;
        lineLen = lineEnd - lineStart + 1;                // +1 to include null termination on copy
        if( bufferLen < lineLen ) lineLen = bufferLen;
        strlcpy(buffer,lineStart,lineLen);
      }
    }
    return result;
}

/** Returns true if a non-zero length line is available for reading.
 *  Searches for an EOL after startLine, and if found returns true if a call to getNextLine
 *  would return a non-zero length line.
 */
boolean  UPnPBuffer::hasNextLine(const char* lineStart) {
    boolean result = false;
    if( lineStart != NULL ) {
       char* lineEnd = strstr_P(lineStart,END_OF_LINE);
       result = ((lineEnd!=NULL)?((lineEnd-lineStart)>0):(false));
    }
    return result;
}

int   UPnPBuffer::maxLineLength() {return _maxLen;}

boolean UPnPBuffer::displayName(char buffer[], size_t len) {
  char headerBuffer[200];
  buffer[0] = '\0';
  boolean result = headerValue_P(DESC_LSC_HEADER,headerBuffer,200);
  if( result ) {
     char* start = strstr(headerBuffer,":name:");
     if( start != NULL ) {
         start += 6;
         char* end = strstr(start,":");
         if( end != NULL ) {
            int nameLen = end - start + 1;  // +1 to account for null termination
            if( nameLen < 0 ) nameLen = 0;
            if(nameLen > len ) nameLen = len -1;
            strlcpy(buffer,start,nameLen);
            buffer[nameLen-1] = '\0';
         }
     }
  }
  return result;
}

boolean UPnPBuffer::headerValue_P(PGM_P header, char buffer[], size_t len) {
  size_t dim = strlen_P(header)+1;
  char cheader[dim];
  strncpy_P(cheader,header,dim-1);
  cheader[dim-1] = '\0';
  return headerValue(cheader,buffer,len);
}

boolean UPnPBuffer::isSearchRequest()  {return (strncmp_P(_buffer,M_SEARCH_HEADER,8) == 0);}
boolean UPnPBuffer::isSearchResponse() {return (strncmp_P(_buffer,RESPONSE_HEADER,8) == 0);}

}
