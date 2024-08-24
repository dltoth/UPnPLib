# SystemClock #
 
Small electronic devices like Arduino have no global sense of Date or Time, only relative time in terms of milliseconds since boot. Internet
connected devices like ESP8266 and ESP32 have the capability to query a network time service to coordinate their internal millisecond timer
to an external service. <b>SystemClock</b> is a library for ESP8266 and ESP32 that provides this capability, synchronizing the internal
millisecond timer, <i>millis()</i>, to a Network Time Protocol (NTP) server. 

An NTP query supplies two pairs of unsigned 32-bit timestamps, each pair consisting of an unsigned seconds and unsigned fraction representing 
time since Jan 1, 1900 00:00:00 in Coordinated Universal Time (UTC). One pair marking time on the NTP server when the query arrived, and one 
pair marking time when the server response was transmitted. As described [in detail below](#ntp-background), a client device can use these timestamps with its 
on board millisecond timer to produce an NTP synchronized time.

## Basic Usage ##

The three main classes that make up this library can be used to develop an NTP synchronized system clock.

The <b><i>Instant</i></b> class represents a point on the NTP timescale described above as seconds since Jan 1, 1900 00:00:00 (See [NTP Background](#ntp-background)
for a detailed discussion on the NTP timescale). <i>Instant</i> has a complete set of operators for addition, subtraction, comparison, and division by integer, all
used in computing the NTP clock offset. <i>Instant</i> also has useful conversions between the NTP timescale and date/time, as well as formatted printing to 
a char buffer.

The <b><i>Timestamp</i></b> class is a combination of <i>Instant</i> with a millisecond timestamp. Together these two classes can be used to keep track of elapsed
time on a host system. 

The <b><i>NTPTime</i></b> class provides the interface to an NTP time service for synchronization. The following lines of code will create a timestamp,
synchronize it with NTP, and keep it updated: 

```
   #include <SystemClock.h>
   using namespace lsc;
   
   Instant      ref(0,JAN1_2024,0)                                 // Create an instant initialized to Jan 1, 2024 00:00:00
   Timestamp    start(ref);                                        // Create a Timestamp with the ref Instant
   Instant      ofst;                                              // NTP clock offset returned as FYI                 
   Timestamp    current = NTPTime::updateSysTime(start,ofst);      // Synchronize start time with NTP and return clock offset used in calculation 
```

The Timestamp <i>current</i> is now synchronized to NTP. At any time, to produce a current UTC date/time from the onboard millisecond timer:

```
   Instant      now = current.update().ntpTime();                  // Fold elapsed millis into current.ntpTime()
   Date         d   = now.toDate();                                // Convert to UTC Date
   Time         t   = now.toTime();                                // Convert to UTC Time
```

where <i>Date</i> and <i>Time</i> are structs defined in <i>Instant.h</i>.

To produce date/time in a specific timezone:

```
   now = current.update().ntpTime().toTimezone(-5.0);              // Produce now in EST
```

At any time, <i>current</i> can be synchronized with NTP as:

```
   current = NTPTime::updateSysTime(current,ofst);
```

In order to determine how far the on board millisecond timer has drifted from NTP, the clock offset can be computed as:

```
   Instant      offset  = NTPTime::ntpOffset(current);
   double       secs    = offset.sysTimed();
```

Note that once <i>current</i> has been aligned to NTP, the offset can be computed without calling <i>current.update()</i> first.

Complete system clock functionality is provided by the <i><b>SystemClock</b></i> class:

```
   #include <SystemClock.h>
   using namespace lsc;

   SystemClock c;
   c.tzOffset(-5.0);            // Set timezone to EST, default is UTC
   c.ntpSync(90);               // Synchronize with NTP every 90 minutes, default is 60
   Instant current = c.now()    // Get current EST time, updated from NTP as necessary
```

The <b>SystemClock</b> library will provide date and time in local time zone, periodically synchronizing the internal millisecond 
timer with an NTP server. 

The full set of classes in the SystemClock library include:

```
  SystemClock      := Provides an NTP synchronized clock for system time
  Instant          := A point on the NTP timescale implemented to replicate NTP 64-bit integer seconds and 
                      unsigned 32-bit fraction
  Timestamp        := An Instant stamped with an internal millisecond timestamp
  NTPTime          := Interface to NTP, providing clock offset for synchronization and update of system time
  Timer            := Measures elapsed time and performs a unit of work
```

<a name="ntp-background"></a>

## NTP Background ##

Network Time Protocol (NTP) is used for synchronizing clocks over the Internet. The wire protocol supplies two pairs of unsigned 32-bit 
timestamps, each pair consisting of an unsigned seconds and unsigned fraction representing time since Jan 1, 1900 00:00:00 in Coordinated 
Universal Time (UTC). The date Jan 1, 1900 00:00:00 is referred to as the <i>Prime Epoch</i>. Note that in representing seconds with only 
32 bits of precision, the timestamp will roll over every 136 years. For this reason, NTP also specifies a 128 bit Datestamp. A Datestamp 
has the form:
   
```
   [signed 32 bit era][unsigned 32 bit era offset][64 bit fraction]
```
      
The high order 32 bits of the seconds field is a signed integer <b><i>era</b></i>, where each era represents 2<sup>32</sup> seconds, 
or 136 years. Positive era for timestamps after the prime epoch, and negative values for before. The lower order 32 bits represent an unsigned
era offset. Era offset is always a positive number of seconds offset from the era, ranging fom 0 to 2<sup>32-1</sup>. Era 0 begins moving 
forward from Jan 1, 1900 00:00:00, and era -1 begins moving backward from that same point. So, the first second of era 0 is offset 1 on Jan 1, 
at 1900 00:00:01, and the first second of era -1 is offset 4294967295 on Dec 31 1899 at 23:59:59. System time on the NTP timescale is then 
defined as a 64 bit signed integer seconds and 64 bit fraction, spanning 584 billion years with attosecond accuracy<sup>[1](#references)</sup>. 

Positive values represent 
time after the prime epoch and negative values represent time prior to the epoch.  

Here are a few examples:
   
```
   Era    Offset          System Time          Date/Time
    0     3913056000       3913056000          Jan 1,  2024  00:00:00
    0     4294967295       4294967295          Feb 7,  2036  06:28:15
    1     0                4294967296          Feb 7,  2036  06:28:16
    2     0                8589934592          Mar 15, 2172  12:56:32   
   -1     4294967295      -1                   Dec 31, 1899  23:59:59
   -1     0               -4294967296          Nov 24, 1763  17:31:44
   -2     0               -8589934592          Oct 18, 1627  11:03:28 
```
 
Notes:
* The NTP Timestamp will roll over on Feb 7, 2036 at 06:28:15.
* The relationship between era, offset, and system time is as follows:

```
    system time  = era*(2^32) + era offset
    era          = (system time - offset)/(2^32)
    era offset   = system time - era*(2^32)
```

* Era and era offset can be computed directly from the system time as:

```
    remainder    = system time % 2^32
    era offset   = ((remainder<0)?(remainder+2^32):(remainder))
    era          = ((remainder<0)?((system time/2^32)-1):(system time/2^32))
```

The relationship between system time, era, era offset, and remainder is shown in the figure below.

````  
                                                                          |----offset--->|
            |---offset--->|<-remainder--|                |                |--remainder-->|
            |------------><------(system time < 0)-------|------(system time > 0)------->|
   _________+___________________________+_________________________________+_____________________+____________...
     (era-1)*2^32                    era*2^32      ...   0   ...       era*2^32          (era+1)*2^32
                      ...    era < 0                     |              era > 0    ...
````

The fractional part of an NTP timestamp is always positive (non-negative) and represents an offset from the system time. So, system time as a floating point is

```
    system time + fraction/2^32
```

The relationship between system time and fraction is shown in the figure below.

````  
            |                           <--fraction-->|
            |<--------- system time - 1 ------------->|<----- system time -----><--- fraction --->|
            |<------------(system time < 0)-----------|<-------------(system time > 0)------------|
   _____________________________________________________________________________________________________
                                                  ... 0 ...
````

### Clock Synchronization ###

NTP is a request/response protocol over UDP, where a client computer sends a request for timestamp to an NTP server. The NTP
response headers provide two 64 bit timestamps, each containing an unsigned 32 bit seconds from Jan 1, 1900 (00:00:00 UTC) and 
an unsigned 32 bit fraction. Call these timestamps t2 and t3 respectively where:
   
```
      t2 - the 32 bit timestamps (offset and fraction) on the NTP server that the request was received 
      t3 - the 32 bit timestamps (offset and fraction) on the NTP server that the response was sent
```

Note that the timestamps t2 and t3 are defined in terms of <b><i>era offset only</b></i> because the actual era is not transmitted 
from the NTP server. In order to express t3 and t3 in full system time, NTP server era must be determined. To this end, era offsets 
between client and server are examined to determine if client and server reside in the same or different eras.

While it is possible for two points on the NTP scale to be more than 68 years apart and reside in the same era, if we assume that 
system time on two clocks are within 68 years (half of an era, or 136 years) and respective era offsets are greater than 68 years, 
then clocks will necessarily straddle an era boundary. In clock synchronization we assume the client clock is initialized to within 
68 years of the server. So if 

```
     client offset - server offset > 68 years
```

the NTP server has rolled, and era on the server is 1 greater than client. Similarly, if

```
     client offset - server offset < -68 years
```

the client has rolled and era on the server is 1 less than client. Otherwise, client and server reside in the same era. Since era on the client
is known, and from the above, era on the server can be derived. So now define the Instants T2 and T3 as:

```
      T2 - the Instant (era, offset, and fraction) on the NTP server that the request was received 
      T3 - the Instant (era, offset, and fraction) on the NTP server that the response was sent
```

Since the client clock is initialized to within 68 years of actual server time, era on the client clock is known. Instants on the client 
are given as a full 64-bit signed integer system time (seconds since Jan 1, 1900) and an unsigned 32-bit fraction. Now, define two additional 
Instants:
   
```
      T1 - the Instant (era, offset, and fraction) on the client computer that the request was sent
      T4 - the Instant (era, offset, and fraction) on the client computer that the response was received
```

The clock offset between client and NTP server can now be computed as:

```
      clock offset = ((T2-T1+(T3-T4)))/2
```

where each of the Instants T1,...,T4 are signed 64-bit seconds and unsigned 32-bit fraction. The client clock is then updated with the 
NTP clock offset as:

```
      sysTime = T4 + clock offset;
```

For more detail on NTP see: [NTP Protocol rfc5905](https://www.rfc-editor.org/rfc/rfc5905)


## Class Detail ##

As mentioned above there are a number of classes that make up this library, together they form the basis for a system clock
that synchronizes with NTP. 

### Instant ###

The <b><i>Instant</b></i> class represents a point on the NTP timeline, and has methods that provide era, era offset, date, and time. 
Instant is given only as seconds (from Jan 1, 1900 UTC) and fraction; timezone must be managed separately. 

Conceptually, Instant can either be implemented with a signed 64-bit integer seconds and unsigned 32-bit fraction offset, or more simply, a 
double precision seconds from Jan 1, 1900. In this library, Instant is implemented with signed 64-bit seconds and unsigned 32 bit 
offset fraction. The choice to use a combination of 64 and 32 bit integers rather than double precision is two-fold: First, to maintain 
the same precision as NTP, and second to take advantage of the speed of integer computation over double precision. The 64-bit seconds field 
is a combination of 32-bit signed era and 32-bit unsigned era offset pre-computed to the sysTime as:

```
      sysTime = era*POW2_32+eraOffset
```
 
The unsigned offset fraction makes arithmetic a little more tricky when sysTime is negative, but still only requires 32-bit addition and subtraction. 

Instant has a full compliment of operators to make the clock offset calculation simpler. For example, if making an NTP request and:

```
      T1 := Instant the request was sent on the client
      T2 := Instant the request was received on the NTP server
      T3 := Instant the server sends the response and
      T4 := Instant the client receives the response
```

then the clock offset can be computed as:

```
      Instant clockOffset = ((T2-T1)+(T3-T4))/2;
```

and the system time can be updated as:

```
      Instant sysTime = T4 + clockOffset;
```

Instant can be converted between NTP system time and date/time or visa versa. For example, the first second of era -1 occurs 
on Dec 31, 1899 at 23:59:59, and thus era offset is 4294967295, so:

```
      Date d = {12,31,1899};
      Time t = {23,59,59};
      Instant T(d,t).secs()           = -1 or
      Instant(-1,4294967295,0).secs() = -1 or
      Instant(-1).toDate()            = {12,31,1899} 
 ```
 
 and
 
 ```
      Instant(-1).toTime()            = {23,59,59}
 ```

The comlete set of methods for Instant are are as follows:
<b>

```
	void           setSysTime(int64_t secs)                       // Set system time to a signed 64-bit integer
	void           setFraction(uint32_t fraction)                 // Set the fractional part to an unsigned 32-bit integer
	void           initialize(int32_t e,uint32_t o,uint32_t f=0)  // Initialize Instant with signed era, unsigned era offset, and unsigned fraction
	void           initialize(double sysd)                        // Initialize Instant with signed double precision system time
	void           initialize(const Date& d, const Time& t)       // Initialize Instant with a Date and Time
	int32_t        era()       const                              // Return the NTP era
	uint32_t       eraOffset() const                              // Rethrn the NTP era offset
	int64_t        secs()      const                              // Return system time as signed 64-bit seconds since Jan 1, 1900 00:00:00
	uint32_t       fraction()  const                              // Return the fractional part of seconds
	double         sysTimed()  const                              // Return system time as signed double precision seconds since Jan 1, 1900 00:00:00, including fraction
    void           addMillis(uint32_t millis)                     // Add milliseconds to Instant
	Date           toDate()                                       // Return Date (month, day, and year) of this Instant
	Time           toTime()                                       // Return time of day (hour:minute:second) of this Instant
    uint64_t       elapsedTime(const Instant& t)   const          // Elapsed time in seconds between this Instant and the input Instant t                             
    Instant        toTimezone(double hours)                       // Return an Instant whose seconds have been shifted by the input hours   
    static Instant toInstant(const Date& d, const Time& t)        // Convert Date and Time to Instant
    static Date    toDate(int64_t secs)                           // Convert signed 64-bit system time to Date
    static Time    toTime(int64_t secs)                           // Convert signed 64-bit system time to Time
    int            cmp(const Instant& rhs)                        // 3-way compare, returns -1 for less than, 0 for equal, and +1 for greater than

    friend Instant abs(const Instant& ref)                        // Return an Instant with positive secs; if secs are negative the result is symetric about NTP time=0

```

</b>

Note that <b><i>Date</b></i> and <b><i>Time</b></i> are C-style structures defined in <i>Instant.h</i>

### Timestamp ###

The <b><i>Timestamp</b></i> class aligns an NTP Instant with the on board millisecond timer. It consists of an <i>Instant</i> and two millisecond timestamps, one for timekeeping and the other as a record of when the Timestamp was minted. <i>Updating</i> the Timestamp folds elapsed milliseconds into the Instant. So, for example:

```
  Date d(1,1,2024);                                                   // Date Jan 1, 2024
  Time t(12,15,0);                                                    // Time 12:15:00
  Instant start(d,t);                                                 // Instant initialized to Jan 1, 2024, 12:15:00
  Timestamp stamp(start);                                             // Timestamp initialized to start
  delay(500);                                                         // Wait for a half sec
  unsigned long currentMillis = millis();                             // Current milliseconds from system
  unsigned long sinceUpdate   = currentMillis - stamp.getMillis();    // Milliseconds since last update (500)
  unsigned long sinceCreation = currentMillis - stamp.getStamp();     // Milliseconds since creation (500)
  for( int i=1; i<100; i++) {
     stamp.update();                                                  // Fold elapsed milliseconds into start
     delay(500);                                                      // Wait for half a second
     currentMillis   = millis();                                      // Current milliseconds from system
     sinceUpdate     = currentMillis - stamp.getMillis();             // Milliseconds since last update (500)
     sinceCreation   = currentMillis - stamp.getStamp();              // Milliseconds since creation (i*500)
     Instant current = stamp.ntpTime();                               // Current time based on Jan 1, 2024, 12:15:00 start
  }

```

So <i>Timestamp::getMillis()</i> will change with each call to <i>Timestamp::update()</i>, however <i>Timestamp::getStamp()</i> will always remain the same. Timestamp methods are as follows:
<b>

```
  Instant           ntpTime()   const                                // Return the underlying Instant for this Timestamp
  unsigned long     getMillis() const                                // Return the millisecond timekeeping stamp  
  unsigned long     getStamp()  const                                // Return the millisecond minting stamp                                        
  void              initialize(const Instant& sysTime)               // Initializes Timestamp with an Instant and stamps millis
  void              update()                                         // Update Instant with elapsed milliseconds from timekeeping stamp
  static Timestamp  stampTime(const Timestamp& t)                    // Construct a new Timestamp with the underlying Instant and stamp it with current millis

  friend Timestamp  abs(const Timestamp& ref)                        // Return a Timestamp with abs(Instant)

```

</b>

Similar to <i>Instant</i>, <i>Timestamp</i> has a full complement of operators for addition, subtraction, and division by integer that operate on the underlying <i>Instant</i>.

### NTPTime ###

The <b><i>NTPTime</b></i> class is a utility class that queries NTP providing timestamps, clock offset, and system time synchronization.
The following time servers are queried in this order, depending on success of host address resolution:

```
      time.google.com
      time.apple.com
      time-a.nist.gov
```

System time is computed on the NTP timescale as seconds since 0h Jan 1, 1900. In computing NTP timestamps it is assumed that the clock runs 
forward from this date/time, hence when era rolls over it will go from n to n+1, and the era offset will go to 0. 

NTPTime methods include:

To query an NTP server for unsigned 32-bit timestamps (seconds and fraction) for request receive and response transmit:

<b>

```
    static void        getNTPTimestamp(unsigned long& rcvSecs, unsigned long& rcvFraction, unsigned long& tsmSecs, unsigned long& tsmFraction);
```

</b>

where 

```
    rcvSecs     - The seconds timestamp on the NTP server that the request was received
    rcvFraction - The fraction (of a second) timestamp on the NTP server that the request was received
    tsmSecs     - The seconds timestamp on the NTP server that the response was transmitted
    tsmFraction - The fraction (of a second) timestamp on the NTP server that the response was transmitted
```

To compute the NTP clock offset (<i>osft</i>) and return updated system time from the input current system time (<i>ref</i>):
 
<b>

```
    static Timestamp   updateSysTime(const Timestamp& ref, Instant& osft);
```

</b>

UpdateSysTime computes the following Instants:

```
     T1 - Instant provided from ref updated prior to NTP query
     T2 - Receive Instant provided by NTP
     T3 - Transmit Instant provided by NTP
     T4 - Instant stamped at time of NTP response
 ```
 
and computes a clock offset as:

```
     osft = ((T2-T1)+(T3-T4))/2;
```
     
and the updated system time as:

```
     T4 + osft;
```

Note it is not necessary to use a <i>SystemClock</i> to obtain the reference system time <i>ref</i> above. Rather, <i>ref</i> can be constructed from an instant that has been initialized to within 68 years of the actual time:

```
     Timestamp ref(Instant(0,JAN1_2024,0));                                            // Initialized to Jan 1, 2024, 00:00:00 (3913056000UL)
     Instant ofst;
     Timestamp current = NTPTime::updateSysTime(const Timestamp& ref, Instant& osft);  // osft will be the number of seconds between JAN1_2024 and NTP UTC
```

At this point, each call to <i>ref.update()</i> will move the Instant <i>ref.ntpTime()</i> forward in time, aligned with the system <i>millis()</i>, and each call to
<i>current.update()</i> will move the instant <i>current.ntpTime()</i> forward aligned with NTP UTC.

To query NTP and compute the clock offset from the input current system time (ref):

<b>

```
    static Instant     ntpClockOffset(const Timestamp& ref);
```

</b>

### SystemClock ###

The <b><i>SystemClock</b></i> class provides an NTP synchronized system time in terms of Instant UTC. NTP synchronization happens with an on board Timer (<i>syncTimer</i>) 
that updates every <i>ntpSync()</i> minutes. The syncTimer can be turned off with 
<b>

```
    setSyncTimerOFF()

```

</b>

in which case, NTP synchronization happens on demand with <i>sysTime()</i> if the <i>ntpSync()</i> interval has passed. 

SystemClock must be initialized to a time close to (within 68 years of) the actual time UTC. The default initialization time is Jan 1, 2024 00:00:00 UTC.

System time is internally managed as UTC. For example, the following methods provide:
<b>

```
   sysTime()            - Current system time UTC, updating with NTP as necessary
   updateSysTime()      - Force NTP update and return current system time UTC
   startTime()          - The actual UTC start time of SystemClock
   initializationDate() - Initialization date/time in UTC
```

</b>

Additionaly, for convenience SystemClock has a timezone offset so some methods provide system time as Instant in local time:
<b>

```
   now()                - System time in local time
   lastSync()           - The last NTP synchronization in local time
   nextSync()           - The next expected NTP synchronization in local time
```

</b> 

Other SystemClock methods include: 

<b>

```
    Instant           utcToLocal(const Instant& utc) const       // Convert utc Instant to local time from timezone offset
    void              initialize(const Instant& ref)             // Initialize SystemClock time, should be within 68 years of actual date/time; default is Jan 1, 2024
    double            tzOffset() const                           // Get timezone offset in hours
    void              tzOffset( double hours )                   // Set timezone offset in hours between -14.25 to +14.25
    const IPAddress&  serverAddress()                const       // Get timeserver IP address to use
    int               serverPort()                   const       // Get timeserver port to use
    void              useNTPService(IPAddress addr,int port)     // Set timeserver IP address and port to use
    void              ntpSync(unsigned int min);                 // Set NTP sync interval in minutes        
    void              setTimerOFF()                              // Turn syncTimer OFF
    void              setTimerON()                               // Turn syncTimer ON
    boolean           timerOFF()       const                     // True of syncTimer is OFF
    boolean           timerON()        const                     // True if syncTImer is ON
    void              doDevice()                                 // Do a unit of work updating syncTimer, should be called from loop() in Arduino sketch
```
</b>


<br><br>
<a name="references"></a>

## References ##

 1. [NTP Protocol rfc5905](https://www.rfc-editor.org/rfc/rfc5905) p. 12






