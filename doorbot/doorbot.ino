/*
  VHS Doorbot V2
	Modified from Arduino WebClient example
 
 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13
 * Door connected to pin 7 and ground (open=floating, closed=held low. Internal pullup is used)

 */

#define ENABLE_VALIDATION

#include <SPI.h>
#include <Ethernet.h>
#if defined(ENABLE_VALIDATION)
#include <aJSON.h>
#endif
#include <MemoryFree.h>
#include <Debouncer.h>


const int kDoorPin = 7;
const unsigned int kDoorSampleRateInMS = 100;
// const int kFooPin = 2;	// int0 = pin2, int1 = pin3 (on most Arduinos...)
const char* kHostName = "api.hackspace.ca";
const char* kAPIKey = "monkey";
IPAddress server(108,171,189,39);

// MAC address to use for the Ethernet shield.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield, otherwise this is arbitrary (should be unique though)
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEB, 0xDA, 0xED };


EthernetClient client;
int DoorState = -1;
// int FooVal = -2;
// int FooState = -1;
// ...

Debouncer DoorDebouncer(kDoorPin, kDoorSampleRateInMS);


void setup()
{
	// Open serial communications and wait for port to open:
	Serial.begin(9600);
	while (!Serial)
		; // wait for serial port to connect. Needed for Leonardo only

	Serial.println();
	Serial.println();
	Serial.println();
	
	Serial.println(F("---- VHS API ----"));
	printMemoryStats();

	// start the Ethernet connection:
	Serial.println(F("Initializing Ethernet/DHCP..."));
	for (int retry=1; (Ethernet.begin(mac) == 0); retry++)
	{
		Serial.println(F("Failed to configure Ethernet using DHCP. Retrying in 10 seconds..."));
		delay(10000);

		Serial.println();
		Serial.print(F("Initializing Ethernet/DHCP (retry "));
		Serial.print(retry);
		Serial.println(F(")..."));
	}

	// give the Ethernet shield a second to initialize:
	Serial.println(F("Waiting for ethernet/DHCP to init..."));
	delay(1000);
	
	Serial.print(F("IP address is: "));
	Serial.println(Ethernet.localIP());
	Serial.println();
	
	pinMode(kDoorPin, INPUT_PULLUP);
	// attachInterrupt(0, OnFooChange, CHANGE);

	Serial.println(F("Running..."));
	Serial.println();
}

void printMemoryStats()
{
	Serial.print(F("Available memory: "));
	Serial.print(freeMemory());
	Serial.println(F(" bytes."));
	Serial.println();
	Serial.flush();
}

void SendAPIGETRequest(const char* key, const char* value)
{
	char getCommand[128];
	sprintf(getCommand, "GET /s/vhs/data/%s/update?value=%s&apikey=%s HTTP/1.1", key, value, kAPIKey);
	char hostCommand[128];
	sprintf(hostCommand, "Host: %s", kHostName);

	client.println(getCommand);
	client.println(hostCommand);
	client.println();
}

bool ReadLine(char* buffer, int bufferSize)
{
	bool result = false;
	int bytesRead = 0;

	while (client.connected() && (bytesRead < (bufferSize-1)))
	{
		if (client.available() > 0)
		{
			char c = client.read();
			if (c == '\n')
			{
				result = true;
				break;
			}
			else if (c == '\r')
			{
				continue;
			}
			
			buffer[bytesRead++] = c;
		}
	}
		
	buffer[bytesRead] = '\0';
	
	return result;
}

#if defined(ENABLE_VALIDATION)
char* ReadBytes(char* buffer, int numBytes)
{
	for (int i=0; i<numBytes; i++)
		*buffer++ = client.read();
	return buffer;
}

int ReadServerResponse(char* pBuffer, int bufferSize, bool* pMalformed)
{
	bool malformed = *pMalformed;
	
	int bytesRead = 0;
	while (client.connected())
	{
		// if there are incoming bytes available from the server, read them
		int bytesWaiting = client.available();
		if ((bytesRead + bytesWaiting) > (bufferSize-1))
		{
			Serial.println(F("Too much data was sent by the server; assuming an error occurred."));
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
#endif

bool ReadResponseHeader(int* pDataSize)
{
	*pDataSize = 0;
	
	char lineBuffer[64];
	
	ReadLine(lineBuffer, sizeof(lineBuffer));
	if (strcmp(lineBuffer, "HTTP/1.1 200 OK") != 0)
		return false;
	
	const char* kContentLength = "Content-Length: ";
	
	int dataSize = 0;
	while ((ReadLine(lineBuffer, sizeof(lineBuffer)) || 1) && (*lineBuffer != '\0'))
	{
		if (strstr(lineBuffer, kContentLength) == lineBuffer)
		{
			const char* pNum = lineBuffer + strlen(kContentLength);
			dataSize = atoi(pNum);
		}
	}

	*pDataSize = dataSize;
	return (dataSize > 0);
}

bool UpdateAPIServer(const char* apiKey, const char* apiValue)
{
	bool result = false;
	
	Serial.println(F("Connecting to server... "));

	if (client.connect(server, 80))
	{
		Serial.println(F("...connected."));
		
		Serial.println(F("Waiting for ethernet client to be ready..."));
		while (!client)
			;
  
		// Make a HTTP GET request with the status of the API
		Serial.println(F("Sending HTTP GET request to server..."));
		SendAPIGETRequest(apiKey, apiValue);

		// Read the servers response
		int dataSize = 0;
		Serial.println(F("Reading response header..."));
		if (ReadResponseHeader(&dataSize))
		{
#if defined(ENABLE_VALIDATION)
			// Guestimate of memory required to parse the servers response:
			//     malloc(~128) + aJsonObject tree(~160)
			const int upperLimitMemoryUse = (dataSize * 2) + (dataSize / 2);
			bool outOfMemory = (freeMemory() <= upperLimitMemoryUse);
			bool malformed = true;
			
			if (!outOfMemory)
			{
				Serial.print(F("Reading response body ("));
				Serial.print(dataSize);
				Serial.println(F(" bytes)..."));
				bool bufferTooSmall = false;
				char* buffer = (char*)malloc(dataSize+1);
				int bytesRead = ReadServerResponse(buffer, dataSize+1, &bufferTooSmall);
				
				Serial.println(F("Parsing server response..."));
				if (!bufferTooSmall)
				{
					if (bytesRead == dataSize)
					{
						aJsonObject* root = aJson.parse(buffer);
						if (root != NULL)
						{
							aJsonObject* status = aJson.getObjectItem(root, "status");
							if ((status != NULL) && (status->type == aJson_String))
							{
								malformed = false;
								
								if (strcmp(status->valuestring, "OK") == 0)
									result = true;
							}
							
							aJson.deleteItem(root);
						}
					}
				}
				
				free(buffer);
			}
#else
			const bool malformed = false;
			const bool outOfMemory = false;
			result = true;
#endif
			
			if (result)
			{
				// API server update was successful!
				Serial.println(F("Server successully updated!"));
			}
			else
			{
				if (outOfMemory)
				{
					Serial.print(F("Server update failed: out of memory. Data size: "));
					Serial.print(dataSize);
					Serial.print(F(" bytes, available memory: "));
					Serial.print(freeMemory());
					Serial.print(F(" bytes, expected upper bounds of memory use: "));
					Serial.print(upperLimitMemoryUse);
					Serial.println(F(" bytes). NOT trying again."));
					Serial.flush();
					
					// We do NOT want to try again, as if we're OOM, that's unlikely to
					// change and we'll just end up spamming the server.
					result = true;
				}
				else
				{
					if (malformed)
						Serial.println(F("Server update failed: server error or malformed data was read. Trying again..."));
					else
						Serial.println(F("Server update failed: server response status indicated failure. Trying again..."));
				}
			}
		}
		else
		{
			Serial.println(F("Server update failed: invalid header. Trying again..."));
		}
	} 
	else
	{
		Serial.println(F("...connection failed."));
	}

	Serial.println(F("Disconnecting."));
	client.stop();
	
	return result;
}

bool MaintainDHCPLease()
{
	Serial.print(F("Maintaining DHCP lease... "));
	int maintainResult = Ethernet.maintain();
	switch (maintainResult)
	{
	case DHCP_CHECK_NONE:
		Serial.println(F("nothing to do."));
		break;
		
	case DHCP_CHECK_RENEW_OK:
		Serial.println(F("successfully renewed."));
		break;
	case DHCP_CHECK_REBIND_OK:
		Serial.println(F("successfully rebound."));
		break;
		
	case DHCP_CHECK_RENEW_FAIL:
		Serial.println(F("renew failed. Trying again..."));
		break;
	case DHCP_CHECK_REBIND_FAIL:
		Serial.println(F("rebind failed. Trying again..."));
		break;
	};
	
	if ((maintainResult == DHCP_CHECK_RENEW_OK) || (maintainResult == DHCP_CHECK_REBIND_OK))
	{
		Serial.print(F("IP address is: "));
		Serial.println(Ethernet.localIP());
	}
	
	Serial.println();

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
	int doorVal = DoorState;
	do
	{
		// Note: Any changes with a short period (button presses, etc) should use interrupts.
		// There's only 2 hw interrupt pins though, so avoid them if possible so they can be
		// used for other purposes in the future.
		
        DoorDebouncer.Update();
        if (DoorDebouncer.IsStable())
			doorVal = DoorDebouncer.GetValue();
		// ...
	} while ((doorVal == DoorState) /* && (FooVal == FooState) ... */);
	
	// Refresh the Ethernet Shield with the DNS server (DHCP)
	bool leaseValid = MaintainDHCPLease();
	
	// Don't attempt to update the server is the LAN connection is down
	if (leaseValid)
	{
		printMemoryStats();
	
		if (doorVal != DoorState)
		{
			char* apiValue;
			if (doorVal != 0)
			{
				Serial.println(F("Space is OPEN!"));
				apiValue = "open";
			}
			else
			{
				Serial.println(F("Space is CLOSED!"));
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
	Serial.println();
	Serial.println();
	Serial.println();
	Serial.println();
	Serial.println();
}

