#include "dom-gps.h"
#include <math.h>
#include <ctype.h>

// how long are max NMEA lines to parse?
#define MAXLINELENGTH 120

// we double buffer: read one line in and leave one for the main program
volatile char line1[MAXLINELENGTH];
volatile char line2[MAXLINELENGTH];
// our index into filling the current line
volatile uint8_t lineidx=0;
// pointers to the double buffers
volatile char *currentline;
volatile char *lastline;
volatile boolean recvdflag;
volatile boolean inStandbyMode;

Dom_GPS::Dom_GPS()
{
	recvdflag 	= false;
	paused 		= false;
	lineidx		= 0;
	currentline = line1;
	lastline	= line2;

	/* uint8_t */
	hour 		= 0;
	minute 		= 0;
	seconds 	= 0;
	year 		= 0;
	month 		= 0;
	day 		= 0;
	fixquality 	= 0;
	satellites 	= 0;

	/* char */
	lat = 0;
	lon = 0;
	mag = 0;

	/* boolean */
	fix = false;

	/* uint16_t */
	milliseconds = 0;

	/* float */
	latitude 		= 0.0;
	longitude 		= 0.0;
	geoidheight 	= 0.0;
	altitude 		= 0.0;
	speed 			= 0.0;
	angle 			= 0.0;
	magvariation 	= 0.0;
	HDOP 			= 0.0;
}

boolean Dom_GPS::parse(char *nmea) {
	// do checksum check

  	// first look if we even have one
	if (nmea[strlen(nmea)-4] == '*') {
	    uint16_t sum = parseHex(nmea[strlen(nmea)-3]) * 16;
	    sum += parseHex(nmea[strlen(nmea)-2]);
	    
	    // check checksum 
	    for (uint8_t i=1; i < (strlen(nmea)-4); i++) {
	      sum ^= nmea[i];
	    }
	    if (sum != 0) {
	      // bad checksum :(
	      //return false;
	    }
  	}

	// look for a few common sentences
	if (strstr(nmea, "$GPGGA")) {
	    // found GGA
	    char *p = nmea;
	    // get time
	    p = strchr(p, ',')+1;
	    float timef = atof(p);
	    uint32_t time = timef;
	    hour = time / 10000;
	    minute = (time % 10000) / 100;
	    seconds = (time % 100);

	    milliseconds = fmod(timef, 1.0) * 1000;

	    // parse out latitude
	    p = strchr(p, ',')+1;
	    latitude = atof(p);

	    p = strchr(p, ',')+1;
	    if (p[0] == 'N') lat = 'N';
	    else if (p[0] == 'S') lat = 'S';
	    else if (p[0] == ',') lat = 0;
	    else return false;

	    // parse out longitude
	    p = strchr(p, ',')+1;
	    longitude = atof(p);

	    p = strchr(p, ',')+1;
	    if (p[0] == 'W') lon = 'W';
	    else if (p[0] == 'E') lon = 'E';
	    else if (p[0] == ',') lon = 0;
	    else return false;

	    p = strchr(p, ',')+1;
	    fixquality = atoi(p);

	    p = strchr(p, ',')+1;
	    satellites = atoi(p);

	    p = strchr(p, ',')+1;
	    HDOP = atof(p);

	    p = strchr(p, ',')+1;
	    altitude = atof(p);
	    p = strchr(p, ',')+1;
	    p = strchr(p, ',')+1;
	    geoidheight = atof(p);
	    return true;
	}
	if (strstr(nmea, "$GPRMC")) {
	    // found RMC
	    char *p = nmea;

	    // get time
	    p = strchr(p, ',')+1;
	    float timef = atof(p);
	    uint32_t time = timef;
	    hour = time / 10000;
	    minute = (time % 10000) / 100;
	    seconds = (time % 100);

	    milliseconds = fmod(timef, 1.0) * 1000;

	    p = strchr(p, ',')+1;
	    // Serial.println(p);
	    if (p[0] == 'A') 
	      fix = true;
	    else if (p[0] == 'V')
	      fix = false;
	    else
	      return false;

	    // parse out latitude
	    p = strchr(p, ',')+1;
	    latitude = atof(p);

	    p = strchr(p, ',')+1;
	    if (p[0] == 'N') lat = 'N';
	    else if (p[0] == 'S') lat = 'S';
	    else if (p[0] == ',') lat = 0;
	    else return false;

	    // parse out longitude
	    p = strchr(p, ',')+1;
	    longitude = atof(p);

	    p = strchr(p, ',')+1;
	    if (p[0] == 'W') lon = 'W';
	    else if (p[0] == 'E') lon = 'E';
	    else if (p[0] == ',') lon = 0;
	    else return false;

	    // speed
	    p = strchr(p, ',')+1;
	    speed = atof(p);

	    // angle
	    p = strchr(p, ',')+1;
	    angle = atof(p);

	    p = strchr(p, ',')+1;
	    uint32_t fulldate = atof(p);
	    day = fulldate / 10000;
	    month = (fulldate % 10000) / 100;
	    year = (fulldate % 100);

	    // we dont parse the remaining, yet!
	    return true;
	}

	return false;
}

char Dom_GPS::read(void) {
  	char c = 0;
  
 	if (paused) return c;

 	c = Serial1.read();

  	Serial.print(c);

  	if (c == '$') {
    	currentline[lineidx] = 0;
    	lineidx = 0;
  	}

  	if (c == '\n') {
    	currentline[lineidx] = 0;

    	if (currentline == line1) {
      		currentline = line2;
      		lastline = line1;
    	} 
    	else {
      		currentline = line1;
      		lastline = line2;
    	}	

    	Serial.println("----");
    	Serial.println((char *)lastline);
    	Serial.println("----");

    	lineidx = 0;
    	recvdflag = true;
  	}

  	currentline[lineidx++] = c;

  	if (lineidx >= MAXLINELENGTH)
    	lineidx = MAXLINELENGTH-1;

 	return c;
}

void Dom_GPS::begin(uint16_t baud)
{
	Serial1.begin();
  	delay(10);
}

void Dom_GPS::sendCommand(char *str) {
	Serial1.println(str);
}

boolean Dom_GPS::newNMEAreceived(void) {
  	return recvdflag;
}

void Dom_GPS::pause(boolean p) {
  	paused = p;
}

char *Dom_GPS::lastNMEA(void) {
	recvdflag = false;
	return (char *)lastline;
}

// read a Hex value and return the decimal equivalent
uint8_t Dom_GPS::parseHex(char c) {
    if (c < '0')
      return 0;
    if (c <= '9')
      return c - '0';
    if (c < 'A')
       return 0;
    if (c <= 'F')
       return (c - 'A')+10;
}

boolean Dom_GPS::waitForSentence(char *wait4me, uint8_t max) {
  	char str[20];

  	uint8_t i=0;
  	while (i < max) {
    	if (newNMEAreceived()) { 
      		char *nmea = lastNMEA();
      		strncpy(str, nmea, 20);
      		str[19] = 0;
      		i++;

      		if (strstr(str, wait4me))
				return true;
    	}
  	}

  	return false;
}

boolean Dom_GPS::LOCUS_StartLogger(void) {
  	sendCommand(PMTK_LOCUS_STARTLOG);
  	recvdflag = false;
  	return waitForSentence(PMTK_LOCUS_LOGSTARTED);
}

boolean Dom_GPS::LOCUS_ReadStatus(void) {
  	sendCommand(PMTK_LOCUS_QUERY_STATUS);
  
  	if (! waitForSentence("$PMTKLOG"))
    	return false;

  	char *response = lastNMEA();
  	uint16_t parsed[10];
  	uint8_t i;
  
  	for (i=0; i<10; i++) parsed[i] = -1;
  
  	response = strchr(response, ',');

  	for (i=0; i<10; i++) {
    	if (!response || (response[0] == 0) || (response[0] == '*')) 
      		break;

    	response++;
    	parsed[i]=0;

    	while ((response[0] != ',') && (response[0] != '*') && (response[0] != 0)) {
      		parsed[i] *= 10;
      		char c = response[0];

      		if (isdigit(c))
        		parsed[i] += c - '0';
      		else
        		parsed[i] = c;

      		response++;
    	}
  	}

  	LOCUS_serial = parsed[0];
  	LOCUS_type = parsed[1];

  	if (isalpha(parsed[2])) {
    	parsed[2] = parsed[2] - 'a' + 10; 
  	}

  	LOCUS_mode = parsed[2];
  	LOCUS_config = parsed[3];
  	LOCUS_interval = parsed[4];
  	LOCUS_distance = parsed[5];
  	LOCUS_speed = parsed[6];
  	LOCUS_status = !parsed[7];
  	LOCUS_records = parsed[8];
  	LOCUS_percent = parsed[9];

  	return true;
}

// Standby Mode Switches
boolean Dom_GPS::standby(void) {
  	if (inStandbyMode) {
    	return false;  // Returns false if already in standby mode, so that you do not wake it up by sending commands to GPS
  	}
  	else {
    	inStandbyMode = true;
    	sendCommand(PMTK_STANDBY);
    //return waitForSentence(PMTK_STANDBY_SUCCESS);  // don't seem to be fast enough to catch the message, or something else just is not working
    	return true;
  	}
}

boolean Dom_GPS::wakeup(void) {
  	if (inStandbyMode) {
   		inStandbyMode = false;
    	sendCommand("");  // send byte to wake it up
    	return waitForSentence(PMTK_AWAKE);
  	}
  	else {
      	return false;  // Returns false if not in standby mode, nothing to wakeup
  	}
}
