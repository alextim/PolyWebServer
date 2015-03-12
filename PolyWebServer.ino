#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>

#include "printf.h"
#include <my_sensor.h>

#include <Ethernet.h>
#include <WebServer.h>

// no-cost stream operator as described at 
// http://sundial.org/arduino/?page_id=119
template<class T>
inline Print &operator <<(Print &obj, T arg)
{ obj.print(arg); return obj; }

my_msg msg;
my_sensor radio(6,7);
//my_sensor radio;

float outsideTemperature = UNDEFINDED_VALUE;
float outsideHumidity = UNDEFINDED_VALUE;

float indoorTemperature = UNDEFINDED_VALUE;
float indoorPressure = UNDEFINDED_VALUE;
float indoorHumidity = UNDEFINDED_VALUE;

float boilerOutTemperature = UNDEFINDED_VALUE;
float boilerReturnTemperature = UNDEFINDED_VALUE;

float radiatorInpTemperature = UNDEFINDED_VALUE;
float radiatorReturnTemperature = UNDEFINDED_VALUE;

float floorInpTemperature = UNDEFINDED_VALUE;
float floorReturnTemperature = UNDEFINDED_VALUE;

static uint8_t mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
static uint8_t ip[] = { 192, 168, 1, 210 };

#define PREFIX ""
WebServer webserver(PREFIX, 80);

void printXmlNode(WebServer &server, String tag, float val) {
  server << "<" << tag << ">";
  if (val != UNDEFINDED_VALUE)
    server << val;
  server << "</" << tag << ">";
}

void printHtmlNode(WebServer &server, String tag, String metric, float val) {
	if( val != UNDEFINDED_VALUE)
	  server << "<div>" << tag << " = " << val << " " << metric << "</div>";
}
	  
void xmlCmd(WebServer &server, WebServer::ConnectionType type, char *, bool) {
  server.httpSuccess();

  if (type != WebServer::HEAD) {
    printXmlNode(server, my_msg::getSensorName(S_T_OUTSIDE), outsideTemperature);
    printXmlNode(server, my_msg::getSensorName(S_H_OUTSIDE), outsideHumidity);
    printXmlNode(server, my_msg::getSensorName(S_T_INDOOR), indoorTemperature);
    printXmlNode(server, my_msg::getSensorName(S_H_INDOOR), indoorHumidity);
    printXmlNode(server, my_msg::getSensorName(S_P_INDOOR), indoorPressure);
    printXmlNode(server, my_msg::getSensorName(S_T_BOILER_OUT), boilerOutTemperature);
    printXmlNode(server, my_msg::getSensorName(S_T_BOILER_RET), boilerReturnTemperature);
    printXmlNode(server, my_msg::getSensorName(S_T_RADIATOR_INP), radiatorInpTemperature);
    printXmlNode(server, my_msg::getSensorName(S_T_RADIATOR_RET), radiatorReturnTemperature);
    printXmlNode(server, my_msg::getSensorName(S_T_FLOOR_INP), floorInpTemperature);
    printXmlNode(server, my_msg::getSensorName(S_T_FLOOR_RET), floorReturnTemperature);
  }
}

void defaultCmd(WebServer &server, WebServer::ConnectionType type, char *, bool) {
  server.httpSuccess();

  if (type != WebServer::HEAD) {
    printHtmlNode(server, "Outside t", "&deg;C", outsideTemperature);
    printHtmlNode(server, "Outside H", "&#37;", outsideHumidity);
    printHtmlNode(server, "Indoor t", "&deg;C", indoorTemperature);
    printHtmlNode(server, "Indoor H", "&#37;", indoorHumidity);
    printHtmlNode(server, "Pressure", "kPa", indoorPressure);
    printHtmlNode(server, "Boiler Out t", "&deg;C", boilerOutTemperature);
    printHtmlNode(server, "Boiler Return t", "&deg;C", boilerReturnTemperature);
    printHtmlNode(server, "Radiator Inp t", "&deg;C", radiatorInpTemperature);
    printHtmlNode(server, "Radiator Return t", "&deg;C", radiatorReturnTemperature);
    printHtmlNode(server, "Floor Inp t", "&deg;C", floorInpTemperature);
    printHtmlNode(server, "Floor Return t", "&deg;C", floorReturnTemperature);
  }
}

void setup(void) {
  Serial.begin(57600);
  printf_begin();  
  
  radio.begin(true);

  Ethernet.begin(mac, ip);
  webserver.setDefaultCommand(&defaultCmd);
  webserver.addCommand("index.html", &defaultCmd);
  webserver.addCommand("xml", &xmlCmd);
  webserver.begin(); 
}

void proceedMsg() {
    msg.print();
	switch(msg.type) {
		case V_TEMP:
			switch (msg.sensor) {
				case S_T_INDOOR: 
					indoorTemperature = msg.getFloat(); 
					break;
				case S_T_OUTSIDE: 
					outsideTemperature = msg.getFloat(); 
					break;
				case S_T_BOILER_OUT: 
					boilerOutTemperature = msg.getFloat(); 
					break;
				case S_T_BOILER_RET: 
					boilerReturnTemperature = msg.getFloat(); 
					break;
				case S_T_RADIATOR_INP: 
					radiatorInpTemperature = msg.getFloat(); 
					break;
				case S_T_RADIATOR_RET: 
					radiatorReturnTemperature = msg.getFloat(); 
					break;
				case S_T_FLOOR_INP: 
					floorInpTemperature = msg.getFloat(); 
					break;
				case S_T_FLOOR_RET: 
					floorReturnTemperature = msg.getFloat(); 
					break;
				default:
					Serial.print("Wrong TEMP sensor No:");
					Serial.println(msg.sensor);
					break;
			}
			break;
		case V_PRESSURE:
			if (msg.sensor == S_P_INDOOR)
				indoorPressure = msg.getFloat();
			else {
				Serial.print("Wrong PRESSURE sensor No:");
				Serial.println(msg.sensor);
			}
			break;
		case V_HUM:	
			if (msg.sensor == S_H_INDOOR)
				indoorHumidity = msg.getFloat();
			else if (msg.sensor == S_H_OUTSIDE)
				outsideHumidity = msg.getFloat();
			else {
				Serial.print("Wrong HUM sensor No:");
				Serial.println(msg.sensor);
			}
			break;
		default:
			Serial.print("UKNOWN sensor type=");
			Serial.print(msg.type);
			Serial.print(", sensor No:");
			Serial.print(msg.sensor);
			Serial.print(", ");
			Serial.println(my_msg::getSensorName(msg.sensor));
			break;
	}
}

void loop(void) {
    if (radio.available()) {
      // Dump the payloads until we've gotten everything
      bool done = false;
      while (!done) {
        // Fetch the payload, and see if this was the last one.
        done = radio.read(&msg, sizeof(my_msg));
		proceedMsg();

	// Delay just a little bit to let the other unit
	// make the transition to receiver
	    delay(20);
      }

      // First, stop listening so we can talk
      radio.stopListening();
      unsigned long got_time = millis();
      // Send the final one back.
      radio.write( &got_time, sizeof(unsigned long) );
      printf("Sent response.\n\r");

      // Now, resume listening so we catch the next packets.
      radio.startListening();
    }
    //radio.read(msg);


  char buff[64];
  int len = 64;

  /* process incoming connections one at a time forever */
  webserver.processConnection(buff, &len);
}
