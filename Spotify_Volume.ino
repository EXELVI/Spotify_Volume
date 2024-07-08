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
String base64secret = SECRET_BASE64;
int keyIndex = 0; // your network key index number (needed only for WEP)

String code = "";
String token = "";
bool requestSent = false;
bool waitingCallback = true;

int status = WL_IDLE_STATUS;

WiFiClient client;

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

void setup()
{

  if (client.available())
  {
    String line = client.readStringUntil('\r');
    Serial.println(line);
    if (line.indexOf("access_token") != -1)
    {
      int tokenIndex = line.indexOf("access_token") + 15;
      int tokenEndIndex = line.indexOf("\"", tokenIndex);
      token = line.substring(tokenIndex, tokenEndIndex);
      Serial.println(token);
    }
  }

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

  getSpotifyRequestUrl();
}

void loop()
{
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
    String auth = base64secret;
    String headers = "content-type: application/x-www-form-urlencoded\nAuthorization: Basic " + auth;

    // request
    client.stop();

    Serial.println("\nStarting connection to server...");

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
