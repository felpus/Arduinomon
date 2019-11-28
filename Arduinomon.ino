#include <SPI.h>
#include <WiFiNINA.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>
#include <Wire.h>
#include "secret.h" // This is where WiFi and MySQL credentials are saved

char ssid[] = SECRET_SSID; // Network name
char pass[] = SECRET_PASS; // Network password
int status = WL_IDLE_STATUS; // Wifi radio's status

IPAddress server_addr(SECRET_IP); // IP of the MySQL *server* here
char user[] = SECRET_USERNAME; // MySQL user login username
char password[] = SECRET_PASSWORD; // MySQL user login password

WiFiClient client; //Er dette en header?
MySQL_Connection conn((Client *)&client); //Er dette en header?

const int MPU_addr=0x68; // I2C address of the MPU-6050
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ; 

long randNumber; // Random Pokemon ID

char INSERT_DATA[] = "INSERT INTO arduinomon.pokemon (pokemon) VALUES (%d)"; // Insert generated Pokemon
char query[128]; // Specifies accepted character values

void setup() {
	Wire.begin();
	Wire.beginTransmission(MPU_addr);
	Wire.write(0x6B);  // PWR_MGMT_1 register
	Wire.write(0);     // set to zero (wakes up the MPU-6050)
	Wire.endTransmission(true);
	Serial.begin(9600);
	while (!Serial) {
		; // wait for serial port to connect. Needed for native USB port only
	}

	// check for the WiFi module:
	if (WiFi.status() == WL_NO_MODULE) {
		Serial.println("Communication with WiFi module failed!");
		// If no WiFI module detected, don't continue
		while (true);
	}

	String fv = WiFi.firmwareVersion(); // Checks if WiFi module firmware is updated
	if (fv < "1.0.0") {
		Serial.println("Please upgrade the firmware!");
	}

	// Attempts to connect to Wifi network with credentials SECRET_SSID and SECRET_PASS
	while (status != WL_CONNECTED) { 
		Serial.print("Attempting to connect to WPA SSID: ");
		Serial.println(ssid);
		// connect to WPA/WPA2 network:
		status = WiFi.begin(ssid, pass);

		// Waits 5 seconds for connection:
		delay(5000);
	}

	// Confirms a connection and prints out the network data
	Serial.println("You're connected to the network");
	printCurrentNet();
	printWifiData();

	// Connects to database with credentials SECRET_USERNAME and SECRET_PASSWORD
	Serial.println("Attempting to connect to database...");
	if (conn.connect(server_addr, 3306, user, password)) {
		delay(1000);
	} else {
		Serial.println("Connection to database failed!");
	}
}

// Main loop
void loop() { 
	randomSeed(AcX + AcY + AcZ); // Initialize a random seed based on a "throw"

	Wire.beginTransmission(MPU_addr);
	Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
	Wire.endTransmission(false);
	Wire.requestFrom(MPU_addr,14,true);  // request a total of 14 registers
	AcX=Wire.read()<<8|Wire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)    
	AcY=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
	AcZ=Wire.read()<<8|Wire.read();  // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
	Tmp=Wire.read()<<8|Wire.read();  // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
	GyX=Wire.read()<<8|Wire.read();  // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
	GyY=Wire.read()<<8|Wire.read();  // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
	GyZ=Wire.read()<<8|Wire.read();  // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)

	// If (accelerometer detects movement that is over 5000) run code.
	// Variable can be changed to make accelerometer more or less sensitive to movement.
	if (AcX > 5000) {
		Serial.println("Collision detected!");
		Serial.print("AcX = "); Serial.print(AcX);
		Serial.print(" | AcY = "); Serial.print(AcY);
		Serial.print(" | AcZ = "); Serial.println(AcZ);
		//Serial.print(" | Tmp = "); Serial.print(Tmp/340.00+36.53);  //equation for temperature in degrees C from datasheet
		//Serial.print(" | GyX = "); Serial.print(GyX);
		//Serial.print(" | GyY = "); Serial.print(GyY);
		//Serial.print(" | GyZ = "); Serial.println(GyZ);

		// Initiate the query class instance
		MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);

		// Generate random Pokemon from Gen 1
		Serial.println("Generating Pokemon...");
		randNumber = random(1, 151); // A Pokemon is given life

		sprintf(query, INSERT_DATA, randNumber); // Accepted character values, SQL command and the randomly generated Pokemon ID

		// Execute the query
		cur_mem->execute(query);
		// Note: since there are no results, we do not need to read any data
		// Deleting the cursor also frees up memory used
		delete cur_mem;
		Serial.println("Pokemon generated!");
		
		// Cooldown between each generated Pokemon
		int seconds = 0;

		while (seconds < 10) { // Set to 10 seconds
			Serial.println(seconds);
			seconds++;
			delay(1000); // Delay in miliseconds
		}

		//delay(10000);
	}
	delay(333);
}

// Gives information about the Arduino network status
void printWifiData() { 
	// print your board's IP address:
	IPAddress ip = WiFi.localIP();
	Serial.print("IP Address: ");
	Serial.println(ip);

	// print your MAC address:
	byte mac[6];
	WiFi.macAddress(mac);
	Serial.print("MAC address: ");
	printMacAddress(mac);
	Serial.println();
}

// Gives information about the currently connected network
void printCurrentNet() {
	// print the SSID of the network you're attached to:
	Serial.print("SSID: ");
	Serial.println(WiFi.SSID());

	// print the MAC address of the router you're attached to:
	byte bssid[6];
	WiFi.BSSID(bssid);
	Serial.print("BSSID: ");
	printMacAddress(bssid);

	// print the received signal strength:
	long rssi = WiFi.RSSI();
	Serial.print("Signal strength (RSSI):");
	Serial.println(rssi);

	// print the encryption type:
	byte encryption = WiFi.encryptionType();
	Serial.print("Encryption Type:");
	Serial.println(encryption, HEX);
	Serial.println();
}

// Used to print mac address for router and Arduino
void printMacAddress(byte mac[]) {
	for (int i = 5; i >= 0; i--) {
		if (mac[i] < 16) {
			Serial.print("0");
		}
		Serial.print(mac[i], HEX);
		if (i > 0) {
			Serial.print(":");
		}
	}
	Serial.println();
}
