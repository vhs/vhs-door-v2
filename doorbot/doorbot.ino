/*
  Web client
 
 This sketch connects to a website (http://www.google.com)
 using an Arduino Wiznet Ethernet shield. 
 
 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13
 
 created 18 Dec 2009
 modified 9 Apr 2012
 by David A. Mellis
 
 */

#include <SPI.h>
#include <Ethernet.h>

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = {  0xDE, 0xAD, 0xBE, 0xEB, 0xDA, 0xED };
//IPAddress server(66,196,40,252); // vancouver.hackspace.ca
IPAddress server(108,171,189,39); // vancouver.hackspace.ca

// Initialize the Ethernet client library
// with the IP address and port of the server 
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;

int DoorState = -1;

void setup()
{
 // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  
  pinMode(7, INPUT_PULLUP);
  pinMode(13, OUTPUT); 

  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0)
  {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    for(;;)
      ;
  }
  
  // give the Ethernet shield a second to initialize:
  Serial.println("Waiting for ethernet/DHCP to init...");
  delay(5000);
  Serial.println("Running...");
}

void loop()
{
  //read the pushbutton value into a variable
  int sensorVal;
  do
  {
	sensorVal = digitalRead(7);
  } while (sensorVal == DoorState);

  Serial.println("connecting...");

  // if you get a connection, report back via serial:
  if (client.connect(server, 80))
  {
	DoorState = sensorVal;
	
    Serial.println("connected");
    // Make a HTTP request:
	if (sensorVal != 0)
	{
		Serial.println("Space is OPEN!");
		client.println("GET /s/vhs/data/door/update?value=open&apikey=monkey HTTP/1.1");
	}
	else
	{
		Serial.println("Space is CLOSED!");
		client.println("GET /s/vhs/data/door/update?value=closed&apikey=monkey HTTP/1.1");
	}
	//client.println("GET /doku.php?id=playground:playground HTTP/1.1");
    //client.println("Host: vancouver.hackspace.ca");
    client.println("Host: vhs.recollect.net");
    client.println();
  } 
  else
  {
    // kf you didn't get a connection to the server:
    Serial.println("connection failed");
  }

  while (client.connected())
  {
	  // if there are incoming bytes available 
	  // from the server, read them and print them:
	  if (client.available()) {
	//  Serial.println("Response received:");
		char c = client.read();
		Serial.print(c);
	  }
  }
/*
  // if the server's disconnected, stop the client:
  if (!client.connected()) {
    Serial.println();
*/
    Serial.println("disconnecting.");
    client.stop();
/*
    // do nothing forevermore:
    for(;;)
      ;
  }
*/

  // Wait a bit before updating the status again
  delay(1000);
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println();
}

