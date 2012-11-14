/*
  VHS Doorbot V2
	Modified from Arduino WebClient example
 
 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13
 * Door connected to pin 7 and ground (open=floating, closed=held low. Internal pullup is used)

 */

#include <SPI.h>
#include <Ethernet.h>
#include <aJSON.h>


const int kDoorPin = 7;
// const int kFooPin = 2;	// int0 = pin2, int1 = pin3 (on most Arduinos...)
const char* kHostName = "api.hackspace.ca";
IPAddress server(108,171,189,39);

// MAC address to use for the Ethernet shield.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield, otherwise this is arbitrary (should be unique though)
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEB, 0xDA, 0xED };


EthernetClient client;
int DoorState = -1;
// int FooVal = -2;
// int FooState = -1;
// ...

void setup()
{
	// Open serial communications and wait for port to open:
	Serial.begin(9600);
	while (!Serial)
		; // wait for serial port to connect. Needed for Leonardo only

	Serial.println();
	Serial.println();
	Serial.println();

	// start the Ethernet connection:
	Serial.println("Initializing Ethernet/DHCP...");
	if (Ethernet.begin(mac) == 0)
	{
		Serial.println("Failed to configure Ethernet using DHCP");
		// no point in carrying on, so do nothing forevermore:
		for(;;)
			;
	}

	// give the Ethernet shield a second to initialize:
	Serial.println("Waiting for ethernet/DHCP to init...");
	delay(1000);
	
	Serial.print("IP address is: ");
	Serial.println(Ethernet.localIP());
	Serial.println();
	
	pinMode(kDoorPin, INPUT_PULLUP);
	// attachInterrupt(0, OnFooChange, CHANGE);

	Serial.println("Running...");
	Serial.println();
}

void SendAPIGETRequest(const char* key, const char* value)
{
	char getCommand[128];
	sprintf(getCommand, "GET /s/vhs/data/%s/update?value=%s&apikey=monkey HTTP/1.1", key, value);
	char hostCommand[128];
	sprintf(hostCommand, "Host: %s", kHostName);

	client.println(getCommand);
	client.println(hostCommand);
	client.println();
}

char* ReadBytes(char* buffer, int numBytes)
{
	for (int i=0; i<numBytes; i++)
		*buffer++ = client.read();
	return buffer;
}
/*
bool IsMalformed(char* data, char** pHeader, char** pBody)
{
	// Check if there was no end of the header
	char* header = data;
	char* body = strstr(header, "\r\n\r\n");
	*pHeader = header;
	*pBody = body;
	if (body == NULL)
		return true;
	
	// NULL terminate the header and advance the body to where it actually begins
	*body++ = '\0';
	*body++ = '\0';
	*body++ = '\0';
	*body++ = '\0';
	
	*pBody = body;
	
	if (strstr(header, "HTTP/1.1 200 OK") == NULL)
		return true;
	
	return false;
}
*/
int ReadServerResponse(char* pBuffer, int bufferSize, bool* pMalformed)
{
	bool malformed = *pMalformed;
	
	int bytesRead = 0;
	while (client.connected())
	{
		// if there are incoming bytes available from the server, read them
		int bytesWaiting = client.available();
		if ((bytesRead + bytesWaiting) > bufferSize)
		{
			Serial.println("Too much data was sent by the server; assuming an error occurred.");
			malformed = true;
			break;
		}
		else if (bytesWaiting)
		{
			pBuffer = ReadBytes(pBuffer, bytesWaiting);
			bytesRead += bytesWaiting;
		}
	}
	*pBuffer = '\0';

	*pMalformed = malformed;
	return bytesRead;
}

bool ParseServerResponse(char* buffer, bool* pMalformed)
{
	bool result = false;
	bool malformed = *pMalformed;

	/*char* header = NULL;
	char* body = NULL;
	if (!malformed)
		malformed = IsMalformed(buffer, &header, &body);*/
		
	if (!malformed)
	{
		/*int headerSize = strlen(header);
		int bodySize = strlen(body);
		
		aJsonObject* root = aJson.parse(body);
		
		if (root != NULL)
		{
			aJsonObject* status = aJson.getObjectItem(root, "status");
			if ((status == NULL) || (status->type != aJson_String) || (strcmp(status->valuestring, "OK") != 0))
				malformed = true;
			else
				result = true;
		}
		else
		{
			malformed = true;
		}
		
		aJson.deleteItem(root);*/

		result = true;
	}
	
	*pMalformed = malformed;
	return result;
}

bool UpdateAPIServer(const char* apiKey, const char* apiValue)
{
	bool result = false;
	
	Serial.println();
	Serial.println("Connecting to server... ");

	if (client.connect(server, 80))
	{
		Serial.println("...connected.");
		
		Serial.println("Waiting for ethernet client to be ready...");
		while (!client)
			;
  
		// Make a HTTP GET request with the status of the API
		SendAPIGETRequest(apiKey, apiValue);

		// Read the servers response
		bool malformed = false;
		char buffer[512];
		int bytesRead = ReadServerResponse(buffer, sizeof(buffer), &malformed);
		
		if (bytesRead > 0)
		{
			Serial.print(buffer);
		
			if (ParseServerResponse(buffer, &malformed))
			{
				// API server update was successful!
				Serial.println("Server successully updated.");
				
				result = true;
			}
			else
			{
				if (malformed)
					Serial.println("Server update failed: server error or malformed data was read. Trying again...");
				else
					Serial.println("Server update failed: server response status was invalid or indicated failure. Trying again...");
			}
		}
		else
		{
			Serial.println("Server update failed: no data was read. Trying again...");
		}
	} 
	else
	{
		Serial.println("...connection failed.");
	}

	Serial.println("Disconnecting.");
	client.stop();
	
	return result;
}

bool MaintainDHCPLease()
{
	Serial.print("Maintaining DHCP lease... ");
	int maintainResult = Ethernet.maintain();
	switch (maintainResult)
	{
	case DHCP_CHECK_NONE:
		Serial.println("nothing to do.");
		break;
		
	case DHCP_CHECK_RENEW_OK:
		Serial.println("successfully renewed.");
		break;
	case DHCP_CHECK_REBIND_OK:
		Serial.println("successfully rebound.");
		break;
		
	case DHCP_CHECK_RENEW_FAIL:
		Serial.println("renew failed. Trying again...");
		break;
	case DHCP_CHECK_REBIND_FAIL:
		Serial.println("rebind failed. Trying again...");
		break;
	};
	
	if ((maintainResult == DHCP_CHECK_RENEW_OK) || (maintainResult == DHCP_CHECK_REBIND_OK))
	{
		Serial.print("IP address is: ");
		Serial.println(Ethernet.localIP());
		Serial.println();
	}
	
	bool leaseValid = (maintainResult != DHCP_CHECK_RENEW_FAIL) && (maintainResult != DHCP_CHECK_REBIND_FAIL);
	return leaseValid;
}

/*
void OnFooChange()
{
	FooVal = digitalRead(kFooPin);
}
*/

void loop()
{
	int doorVal;
	do
	{
		// Note: Any changes with a short period (button presses, etc) should use interrupts.
		// There's only 2 hw interrupt pins though, so avoid them if possible so they can be
		// used for other purposes in the future.
		
		doorVal = digitalRead(kDoorPin);
		// ...
	} while ((doorVal == DoorState) /* && (FooVal == FooState) ... */);
	
	// Refresh the Ethernet Shield with the DNS server (DHCP)
	bool leaseValid = MaintainDHCPLease();
	
	// Don't attempt to update the server is the LAN connection is down
	if (leaseValid)
	{
		if (doorVal != DoorState)
		{
			char* apiValue;
			if (doorVal != 0)
			{
				Serial.println("Space is OPEN!");
				apiValue = "open";
			}
			else
			{
				Serial.println("Space is CLOSED!");
				apiValue = "closed";
			}
			
			if (UpdateAPIServer("door", apiValue))
				DoorState = doorVal;
		}

/*	
		//
		if (FooVal != FooState)
		{
			...
			FooState = FooVal;
		}
		
		...
*/
	}
	
	//
	// Wait a bit before checking the status again
	delay(1000);
	Serial.println();
	Serial.println();
	Serial.println();
	Serial.println();
	Serial.println();
}

