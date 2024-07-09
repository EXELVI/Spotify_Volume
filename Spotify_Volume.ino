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
#include "WiFiSSLClient.h"

const char *host = "api.spotify.com";
#include "arduino_secrets.h"

int wifiStatus = WL_IDLE_STATUS;
WiFiUDP Udp;

WiFiServer server(80);

unsigned long lastConnectionTime = 0;
const unsigned long postingInterval = 120000;
JSONVar myObject;

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
String client_id = SECRET_CLIENT_ID;
String client_secret = SECRET_CLIENT_SECRET;
String base64auth = SECRET_BASE64;
int keyIndex = 0; // your network key index number (needed only for WEP)

String code = "";
String token = "";
bool requestSent = false;
bool waitingCallback = true;

bool editing = false;

int status = WL_IDLE_STATUS;

WiFiSSLClient client;

unsigned long lastBlinkTime = 0;
const unsigned long blinkInterval = 500;

String getRandomString(int length)
{
  String randomString;
  char charset[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  for (int n = 0; n < length; n++)
  {
    randomString += charset[random(0, sizeof(charset) - 1)];
  }
  return randomString;
}

String getSpotifyRequestUrl()
{
  String redirect_uri = "http://" + WiFi.localIP().toString() + "/callback";
  String scope = "user-read-playback-state user-modify-playback-state";
  String state = getRandomString(16);

  String url = "https://accounts.spotify.com/authorize?" + String("client_id=") + client_id + String("&response_type=code") + String("&redirect_uri=") + redirect_uri + String("&scope=") + scope + String("&state=") + state;

  Serial.println(url);
  return url;
}

void printToMatrix(char text[])
{

  matrix.beginDraw();
  matrix.clear();
  matrix.textScrollSpeed(100);

  matrix.stroke(0xFFFFFFFF);
  matrix.textFont(Font_5x7);
  matrix.beginText(0, 1, 0xFFFFFF);
  matrix.println(text);
  matrix.endText(SCROLL_LEFT);

  matrix.endDraw();
}

void printIntToMatrix(int number)
{

  matrix.beginDraw();
  matrix.clear();

  matrix.stroke(0xFFFFFFFF);
  char cstr[16];
  itoa(number, cstr, 10);
  matrix.textFont(number < 100 ? Font_5x7 : Font_4x6);
  matrix.beginText(0, 1, 0xFFFFFF);
  matrix.println(cstr);
  matrix.endText(NO_SCROLL);

  matrix.endDraw();
}

void clearMatrix()
{
  matrix.beginDraw();
  matrix.clear();
  matrix.endDraw();
}

bool blinked = false;

void setup()
{
  Serial.begin(9600);
  while (!Serial)
  {
    ;
  }
  matrix.begin();

  pinMode(pinA, INPUT);
  pinMode(pinB, INPUT);
  pinMode(pinSW, INPUT_PULLUP);

  pinALast = digitalRead(pinA);

  if (WiFi.status() == WL_NO_MODULE)
  {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
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

  server.begin();

  getSpotifyRequestUrl();
}

void loop()
{
  if (client.available())
  {
    if (token == "")
    {
      String line = client.readStringUntil('\r');
      Serial.println(line);
      if (line.indexOf("access_token") != -1)
      {
        int tokenIndex = line.indexOf("access_token") + 15;
        int tokenEndIndex = line.indexOf("\"", tokenIndex);
        token = line.substring(tokenIndex, tokenEndIndex);
        Serial.println(token);

        delay(1000);
        client.stop();

        delay(1000);
        Serial.print("Starting connection to server... " + String(host) + ": ");

        if (client.connect(host, 443))
        {
          Serial.println("connected to server");
          client.println("GET /v1/me/player HTTP/1.1");
          client.println("Host: api.spotify.com");
          client.println("Authorization: Bearer " + token);
          client.println("Content-Type: application/json");
          client.println("Accept: application/json");
          client.println("Connection: close");
          client.println();
        }
        else
        {
          Serial.println("connection failed");
        }
      }
    }
    else
    {
      String line = client.readStringUntil('\r');
      Serial.println(line);
      if (line.indexOf("volume_percent") != -1)
      {
        int volumeIndex = line.indexOf("volume_percent") + 17;
        int volumeEndIndex = line.indexOf(",", volumeIndex);
        int volume = line.substring(volumeIndex, volumeEndIndex).toInt();
        Serial.println(volume);
        encoderPosCount = volume;
        printIntToMatrix(encoderPosCount);
      }
    }
  }

  if (waitingCallback)
  {
    WiFiClient clientServer = server.available();

    if (clientServer)
    { // callback
      Serial.println("new client");
      String currentLine = "";
      while (clientServer.connected())
      {
        if (clientServer.available())
        {
          char c = clientServer.read();
          Serial.write(c);
          if (c == '\n')
          {
            if (currentLine.length() == 0)
            {
              String curl = getSpotifyRequestUrl();
              clientServer.println("HTTP/1.1 200 OK");
              clientServer.println("Content-type:text/html");
              clientServer.println();
              clientServer.println("<html>");
              clientServer.println("<head>");
              clientServer.println("<title>Spotify Volume</title>");
              clientServer.println("</head>");
              clientServer.println("<body>");
              clientServer.println("<h1>Spotify Volume</h1>");
              clientServer.println("<a href=\"" + curl + "\">Callback Url</a>");
              clientServer.println("</body>");
              clientServer.println("</html>");
              break;
            }
            else
            {
              currentLine = "";
            }
          }
          else if (c != '\r')
          {
            currentLine += c;
          }
          if (currentLine.indexOf("GET /callback?code=") != -1 && currentLine.indexOf("HTTP/1.1") != -1)
          {
            int codeIndex = currentLine.indexOf("GET /callback?code=") + 20;
            int stateIndex = currentLine.indexOf("&state=");
            code = currentLine.substring(codeIndex - 1, stateIndex);
            waitingCallback = false;
            Serial.println(code);

            clientServer.println("HTTP/1.1 200 OK");
            clientServer.println("Content-type:text/html");
            clientServer.println();
            clientServer.println("<html>");
            clientServer.println("<head>");
            clientServer.println("<title>Spotify Volume</title>");
            clientServer.println("</head>");
            clientServer.println("<body>");
            clientServer.println("<h1>Spotify Volume</h1>");
            clientServer.println("<h2>Callback Received</h2>");
            clientServer.println("<a href=\"/\">Home</a>");
            clientServer.println("</body>");
            clientServer.println("</html>");

            break;
          }
        }
      }
    }
  }
  else if (token == "" && !requestSent)
  {

    // get spotify user token from code
    String redirect_uri = "http://" + WiFi.localIP().toString() + "/callback";

    String body = "code=" + code + "&redirect_uri=" + redirect_uri + "&grant_type=authorization_code";
    String auth = base64auth;
    String headers = "content-type: application/x-www-form-urlencoded\nAuthorization: Basic " + auth;

    // request
    client.stop();

    Serial.print("\nStarting connection to server accounts.spotify.com: ");

    if (client.connect("accounts.spotify.com", 443))
    {
      Serial.println("connected to server");
      client.println("POST /api/token HTTP/1.1");
      client.println("Host: accounts.spotify.com");
      client.println("Content-Type: application/x-www-form-urlencoded");
      client.println("Authorization: Basic " + auth);
      client.print("Content-Length: ");
      client.println(body.length());
      client.println();
      client.println(body);
      requestSent = true;
    }
    else
    {
      Serial.println("connection failed");
    }
  }
  else if (token != "")
  {
    aVal = digitalRead(pinA);

    if (aVal != pinALast)
    {
      editing = true;
      if (digitalRead(pinB) != aVal)
      {
        encoderPosCount++;
        encoderPosCount++;
        bCW = true;
        if (encoderPosCount > 100)
        {
          encoderPosCount = 100;
        }
      }
      else
      {
        encoderPosCount--;
        encoderPosCount--;
        bCW = false;

        if (encoderPosCount < 0)
        {
          encoderPosCount = 0;
        }
      }
      Serial.print("Rotated: ");
      if (bCW)
      {
        Serial.println("clockwise");
      }
      else
      {
        Serial.println("counterclockwise");
      }
      Serial.print("Encoder Position: ");
      Serial.println(encoderPosCount);
      printIntToMatrix(encoderPosCount);
    }

    if (editing)
    {
      if (millis() - lastBlinkTime > blinkInterval)
      {
        if (blinked)
        {
          printIntToMatrix(encoderPosCount);
          blinked = false;
        }
        else
        {
          clearMatrix();
          blinked = true;
        }
      }
    }

    if (digitalRead(pinSW) == LOW)
    {
      editing = false;
      // request
      client.stop();

      Serial.print("\nStarting connection to server api.spotify.com: ");

      if (client.connect("api.spotify.com", 443))
      {
        Serial.println("connected to server");
        client.println("PUT /v1/me/player/volume?volume_percent=" + String(encoderPosCount) + " HTTP/1.1");
        client.println("Host: api.spotify.com");
        client.println("Authorization: Bearer " + token);
        client.println("Content-Type: application/json");
        client.println("Accept: application/json");
        client.println("Connection: close");
        client.println();
        
      }
      else
      {
        Serial.println("connection failed");
      }
      delay(1000); 
    }
  }
}

void printWifiStatus()
{

  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  String ipString = WiFi.localIP().toString();
  char ipCh[ipString.length() + 1];
  strcpy(ipCh, ipString.c_str());

  printToMatrix(ipCh);

  long rssi = WiFi.RSSI();

  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
