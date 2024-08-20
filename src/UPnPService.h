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

#ifndef UPNP_SERVICE_H
#define UPNP_SERVICE_H

#include <Arduino.h>
#include <ctype.h>
#include <CommonUtil.h>

/** Leelanau Software Company namespace 
*  
*/
namespace lsc {

#define TARGET_SIZE    32
#define NAME_SIZE      32

typedef std::function<void(void)> CallbackFunction;

/**
 *   Macro to define Runtime Type Identification and UPnP Device Type
 *   Note that the static upnpType() is tied to the class and virtual getType() is tied to the instance of an Object
 *   the same way that classType() is tied to the class and isClassType() tied to the instance
 */
#define DEFINE_RTTI     private: static const ClassType  _classType;                                                           \
                        public:  static const ClassType* classType()                 {return &_classType;}                     \
                        public:  virtual void*           as(const ClassType* t)      {return((isClassType(t))?(this):(NULL));} \
                        private: static const char*      _upnpType;                                                            \
                        public:  static const char*      upnpType()                  {return _upnpType;}                       \
                        public:  virtual const char*     getType()                   {return upnpType();}                      \
                        public:  virtual boolean         isType(const char* t)       {return(strcmp(t,getType()) == 0);}  

/**
 *   Define type check for classes derived from a single Base Class
 */
#define DERIVED_TYPE_CHECK(BaseClass) public: virtual boolean isClassType( const ClassType* t) {return (_classType.isClassType(t) || BaseClass::isClassType(t));}

/**
 *   Note that isClassType() could be defined for multiple inheritance, however the as() operator will not work correctly when the second (or subsequent) class
 *   has one or more virtual methods. The virtual methods will not properly resolve.  This RTTI subsystem is therefore to be used ONLY in single inheritance
 *   class hierarchys. Multiple inheritance type check would look like:
 *   virtual boolean   isClassType( const ClassType* t) {return (_classType.isClassType(t) || ClassName1::isClassType(t) || ... || ClassNameN::isClassType(t));}
 */

/**
 *   Define type check for base classes
 *   Note: This should only be necessary for classes that are NOT subclasses of UPnPObject
 */
#define BASE_TYPE_CHECK  public: virtual boolean isClassType( const ClassType* t) {return _classType.isClassType(t);}

/**
 *   Used in macros INITIALIZE_UPnP_DEVICE_TYPE and INITIALIZE_UPnP_SERVICE_TYPE below
 */
#define EXPAND_UPNP_TYPE(sep,urnStr,domain,deviceStr,type,ver) #urnStr#sep#domain#sep#deviceStr#sep#type#sep#ver;

/**
 *  Macros combine initialization of RTTI and UPnP types
 */
#define INITIALIZE_DEVICE_TYPES(className,domain,type,ver)  const ClassType className::_classType = ClassType(); \
                                                            const char*     className::_upnpType = EXPAND_UPNP_TYPE(:,urn,domain,device,type,ver);
#define INITIALIZE_SERVICE_TYPES(className,domain,type,ver) const ClassType className::_classType = ClassType(); \
                                                            const char*     className::_upnpType = EXPAND_UPNP_TYPE(:,urn,domain,service,type,ver);

/**
 *   Copy construction and assignment are not allowed
 */
#define DEFINE_EXCLUSIONS(className)      className(const className&)= delete;      \
                                          className& operator=(const className&)= delete;


/**
 *    Macro to define typesafe cast of parent
 */
#define GET_PARENT_AS(T) ((getParent()!=NULL)?(getParent()->as(T)):(NULL))

class UPnPService;
class UPnPDevice;
class RootDevice;

class ClassType {
  public:
    ClassType() {_typeID=++_numTypes;}

    int         typeID() const               {return _typeID;}
    
    boolean     isClassType( const ClassType* t) const {return ((t!=NULL)?(this->typeID() == t->typeID()):(false));}

  private:
    static int   _numTypes;
    int          _typeID;
};

/** UPnPObject class definition.
 *  UPnPObject Class members are:
 *     _target       := The relative URL for this service. The complete URL can be constructed as "/rootTarget/deviceTarget/serviceTarget"
 *                      or "/rootTarget/serviceTarget"
 *     _parent       := A pointer to the UPnPDevice containing this service
 *     _displayName  := Name of Object for display purposes
 *    
 *  Static Members defined in the macro DEFINE_RTTI 
 *     _classType    := Bespoke RTTI class type and associated methods    
 *                        static const ClassType* classType()                      {return &_classType;}
 *                        virtual boolean         isClassType( const ClassType* t) {return (_classType.isClassType(t) || UPnPDevice::isClassType(t) || ...);}
 *                        virtual void*           as(const ClassType* t)           {return((isClassType(t))?(this):(NULL));}
 *     _upnpType     := UPnP Device (Service) type and associated methods
 *                        static const char*      upnpType()                  {return _upnpType;} 
 *                        virtual const char*     getType()                   {return upnpType();}   
 *                        virtual boolean         isType(const char* t)       {return(strcmp(t,getType()) == 0);}  
 *
 *     
 *
 */
 
class UPnPObject {

   public:

     UPnPObject();
     UPnPObject(const char* target) {setTarget(target);}
     virtual ~UPnPObject() {}

     void           setTarget(const char* target);
     void           setDisplayName(const char* name);
    
     const char*    getTarget()           {return _target;}
     const char*    getDisplayName()      {return _displayName;}
     UPnPObject*    getParent()           {return _parent;}
     UPnPDevice*    parentAsDevice()      {return ((_parent!=NULL)?(_parent->asDevice()):(NULL));}
     boolean        hasParent()           {return getParent() != NULL;}
     RootDevice*    rootDevice();
     void           getVersion(char buffer[], size_t size);                           // Fill buffer with the version from getType()
     void           getDomain(char buffer[], size_t size);                            // Fill buffer with the domain from getType()
     void           getUPnPType(char buffer[], size_t size);                          // Fill buffer with the device (service) type from getType()
     void           getPath(char buffer[], size_t size);                              // Returns a relative target path from root, including this getTarget()
     void           handlerPath(char buffer[], size_t size, const char* handlerName); // Concatenate handlerName to path

     static void    encodePath(char buffer[], size_t size, const char* path);         // URL Encode path into buffer. Replaces '/' with "%2F"
       
     public:
     DEFINE_RTTI;
     BASE_TYPE_CHECK;
     
     virtual RootDevice*     asRootDevice()                        = 0;
     virtual UPnPService*    asService()                           = 0;
     virtual UPnPDevice*     asDevice()                            = 0;
     virtual void            location(char buffer[], int buffSize, IPAddress addr) = 0;
     DEFINE_EXCLUSIONS(UPnPObject);         

   protected:
     char                  _target[TARGET_SIZE];
     char                  _displayName[NAME_SIZE];
     UPnPObject*           _parent = NULL;

     void               setParent(UPnPObject* parent)  {_parent = parent;}

};

/** UPnPService Class Definition
 *  UPnPService is set up to hanele HTTP requests to the target /rootTarget/deviceTarget/serviceTarget
 *  Service implementations can either subclass UPnPService and override handleRequest(), or set a
 *  handler function from the parent UPnPDevice, for example see the GetConfiguration service.
 */

class UPnPService : public UPnPObject {
     public:
     UPnPService() : UPnPObject("service") {setDisplayName("Service");}
     UPnPService(const char* target) : UPnPObject(target) {setDisplayName("Service");};
     virtual ~UPnPService() {}
     
     void            setHttpHandler(HandlerFunction h)    {_handler = h;}
     virtual void    handleRequest(WebContext* svr)       {_handler(svr);}

  
/**
 *   Macro to define the following Runtime and UPnP Type Info:
 *     private: static const ClassType  _classType;             
 *     public:  static const ClassType* classType();   
 *     public:  virtual void*           as(const ClassType* t);
 *     public:  virtual boolean         isClassType( const ClassType* t);
 *     private: static const char*      _upnpType;                                      
 *     public:  static const char*      upnpType()                  
 *     public:  virtual const char*     getType()                   
 *     public:  virtual boolean         isType(const char* t)       
 */
     DEFINE_RTTI;
     DERIVED_TYPE_CHECK(UPnPObject);

     virtual RootDevice*      asRootDevice()  {return NULL;}
     virtual UPnPDevice*      asDevice()      {return NULL;}
     virtual UPnPService*     asService()     {return this;}
     virtual void             location(char buffer[], int buffSize, IPAddress addr);
     virtual void             setup(WebContext* svr);

     HandlerFunction          _handler = [](WebContext* svr) {};

     friend class             UPnPDevice;

/**
 *   Copy construction and destruction are not allowed
 */
     DEFINE_EXCLUSIONS(UPnPService);         
};

} // End of namespace lsc

#endif
