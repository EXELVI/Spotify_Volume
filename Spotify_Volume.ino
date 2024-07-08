#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"

ArduinoLEDMatrix matrix;

int pinA = 3;  // CLK
int pinB = 4;  // DT
int pinSW = 2; // SW
int encoderPosCount = 0;

int pinALast;
int aVal;
boolean bCW;

#include "WiFiS3.h"
#include <WiFiUdp.h>
#include <Arduino_JSON.h>

const char *host = "api.spotify.com";
#include "arduino_secrets.h"

WiFiUDP Udp;

unsigned long lastConnectionTime = 0;
const unsigned long postingInterval = 120000;
JSONVar myObject;

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
String client_id = SECRET_CLIENT_ID;
String client_secret = SECRET_CLIENT_SECRET;
int keyIndex = 0; // your network key index number (needed only for WEP)

int status = WL_IDLE_STATUS;

WiFiClient client;

void setup()
{

  Serial.begin(9600);
  while (!Serial)
  {
    ;
  }

  pinMode(pinA, INPUT);
  pinMode(pinB, INPUT);
  pinMode(pinSW, INPUT);

  pinALast = digitalRead(pinA);

  if (WiFi.status() == WL_NO_MODULE)
  {
    Serial.println("Communication with WiFi module failed!");
    while (true)
      ;
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION)
  {
    Serial.println("Please upgrade the firmware");
  }

  while (status != WL_CONNECTED)
  {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
  }

  printWifiStatus();
  delay(5000);
}

void loop()
{
}

void printWifiStatus()
{

  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  long rssi = WiFi.RSSI();

  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
