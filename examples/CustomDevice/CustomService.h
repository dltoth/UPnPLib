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

#ifndef CUSTOMSERVICE_H
#define CUSTOMSERVICE_H
#include <UPnPLib.h>

class CustomService : public UPnPService {
  public:
    CustomService() : UPnPService("customService") {setDisplayName("Custom Service");};
    CustomService(const char* target) : UPnPService(target) {setDisplayName("Custom Service");};
    virtual ~CustomService() {}

    DEFINE_RTTI;
    DERIVED_TYPE_CHECK(UPnPService);

/**
 *   Copy construction and destruction are not allowed
 */
     DEFINE_EXCLUSIONS(CustomService);         
};

#endif
