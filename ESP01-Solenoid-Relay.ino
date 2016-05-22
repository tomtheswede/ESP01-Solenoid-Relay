/*  
 *   For a 12V Solenoid with an ESP-01 chip.
 *   Most code by Thomas Friberg
 *   Updated 21/05/2016
 */

// Import ESP8266 libraries
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

//Sensor details
const char* sensorID1 = "RET001"; //Name of sensor
const char* deviceDescription = "Reticulation Relay";
const int ledPin = 2; //LED pin number

// WiFi parameters
const char* ssid = "TheSubway"; //Enter your WiFi network name here in the quotation marks
const char* password = "vanillamoon576"; //Enter your WiFi pasword here in the quotation marks

//Server details
unsigned int localPort = 5007;  //UDP send port
const char* ipAdd = "192.168.0.100"; //Server address
byte packetBuffer[512]; //buffer for incoming packets

//Sensor variables
int ledPinState = 0; //Default boot state of LEDs and last setPoint of the pin between 0 and 100
int ledSetPoint = 0;
String data = "";
int timerCount = 0;

WiFiUDP Udp; //Instance to send packets


//--------------------------------------------------------------

void setup()
{
  SetupLines();
}

//--------------------------------------------------------------


void loop()
{
  //Check timer state
  CheckTimer();
  
  data=ParseUdpPacket(); //Code for receiving UDP messages
  if (data!="") {
    ProcessLedMessage(data);//Conditionals for switching based on LED signal
  }
  
}

//--------------------------------------------------------------





void SetupLines() {
  //Set pins and turn off LED
  pinMode(ledPin, OUTPUT); //Set as output
  digitalWrite(ledPin, 0); //Turn off LED while connecting
  
  // Start Serial port monitoring
  Serial.begin(115200);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected with IP: ");
  // Print the IP address
  Serial.println(WiFi.localIP());

  //Open the UDP monitoring port
  Udp.begin(localPort);
  Serial.print("Udp server started at port: ");
  Serial.println(localPort);

  //Register on the network with the server after verifying connect
  delay(2000); //Time clearance to ensure registration
  SendUdpValue("REG",sensorID1,String(deviceDescription)); //Register LED on server
  digitalWrite(ledPin, HIGH); //Turn off LED while connecting
  delay(50); //A flash of light to confirm that the lamp is ready to take commands
  digitalWrite(ledPin, LOW); //Turn off LED while connecting
  ledSetPoint=0; // input a setpoint for fading as we enter the loop
}


String ParseUdpPacket() {
  int noBytes = Udp.parsePacket();
  String udpData = "";
  if ( noBytes ) {
    Serial.print("---Packet of ");
    Serial.print(noBytes);
    Serial.print(" characters received from ");
    Serial.print(Udp.remoteIP());
    Serial.print(":");
    Serial.println(Udp.remotePort());
    // We've received a packet, read the data from it
    Udp.read(packetBuffer,noBytes); // read the packet into the buffer

    // display the packet contents in HEX
    for (int i=1;i<=noBytes;i++) {
      udpData = udpData + char(packetBuffer[i - 1]);
    } // end for
    Serial.println("Data reads: " + udpData);
  } // end if
  return udpData;
}

void ProcessLedMessage(String dataIn) {
  String devID = "";
  String message = "";

  devID=dataIn.substring(0,6); //Break down the message in to it's parts
  Serial.println("DevID reads after processing: " + devID);
  message=dataIn.substring(7);
  Serial.println("Message reads after processing: " + message);
  
  if (devID==sensorID1) { //Only do this set of commands if there is a message for an LED device
    
    //Enables timed commands
    if (message.startsWith("timer")) { 
      int messagePos=message.indexOf(" ");
      //Serial.println("Position of space is " + messagePos);
      String timerVal=message.substring(5,messagePos);
      //Serial.println("Fade value is " + fadeVal);
      timerCount=atoi(timerVal.c_str());
      //Serial.println("Fade speed set to " + fadeSpeed);
      message=message.substring(messagePos+1); //Cutting 'timer' from the message
      Serial.println("Custom timer of " + timerVal + " seconds set");
      Serial.println("Message trimmed to: " + message);
    }
    else {
      timerCount=0;
    }

    //Enables instant toggling
    if (((message=="toggle")||(message=="on")||(message=="100")) && (ledPinState==0)) { //Only turn on if already off
      SendUdpValue("LOG",sensorID1,String(100));
      ledPinState=100;
      digitalWrite(ledPin, HIGH);
      Serial.println("---Instant on triggered");
    }
    else if (((message=="toggle")  || (message=="off") || (message=="0")) && (ledPinState>0)) { //Only turn off if already on
      SendUdpValue("LOG",sensorID1,String(0));
      ledPinState=0;
      digitalWrite(ledPin, LOW);
      Serial.println("---Instant off triggered");
    }
  }
}


void CheckTimer() {
  if(millis() % 1000 == 0) {
    if(timerCount==0) {
      //Do nothing
    }
    else if (timerCount>1) {
      timerCount=timerCount-1;
      Serial.println("Timer value reduced to " + String(timerCount) + "");
    }
    else {
      timerCount=timerCount-1;
      SendUdpValue("FWD",sensorID1,"off");
    }
    delay(1);
  }
}

void SendUdpValue(String type, String sensorID, String value) {
  //Print GPIO state in serial
  Serial.print("-Value sent via UDP: ");
  Serial.println(type + "," + sensorID + "," + value);

  // send a message, to the IP address and port
  Udp.beginPacket(ipAdd,localPort);
  Udp.print(type);
  Udp.write(",");
  Udp.print(sensorID);
  Udp.write(",");
  Udp.print(value); //This is the value to be sent
  Udp.endPacket();
}
