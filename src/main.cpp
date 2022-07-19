#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
WiFiServer server(80);
String header;

// Auxiliar variables to store the current output state
int lightstate = 0;
int output4State = 0;
// Assign output variables to GPIO pins
const int output5 = 5;
const int output4 = 4;
String json;
// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;
void setup()
{
  Serial.begin(9600);
  pinMode(output5, OUTPUT);
  pinMode(output4, OUTPUT);
  // set outputs to low
  digitalWrite(output5, LOW);
  digitalWrite(output4, LOW);
  WiFiManager wifiManager;
  // wifiManager.resetSettings();
  wifiManager.setSTAStaticIPConfig(IPAddress(10, 0, 0, 101), IPAddress(10, 0, 0, 1), IPAddress(255, 255, 255, 0)); // optional DNS 4th argument
  wifiManager.autoConnect("Fan Bridge");
  Serial.println("Connected!");
  server.begin();
}

void loop()
{
  WiFiClient client = server.available(); // Listen for incoming clients
  HTTPClient http;
  if (client)
  {
    http.begin(client, "http://10.0.0.105:8080/fanlight");
    http.addHeader("Content-Type", "text/json"); // If a new client connects,
    Serial.println("New Client.");               // print a message out in the serial port
    String currentLine = "";                     // make a String to hold incoming data from the client
    DynamicJsonDocument doc(1024);
    while (client.connected())
    { // loop while the client's connected
      if (client.available())
      {                         // if there's bytes to read from the client,
        char c = client.read(); // read a byte, then
        Serial.write(c);        // print it out the serial monitor
        header += c;
        if (c == '\n')
        { // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0)
          {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:

            // make sure to process state changes up here to include in the return before client disconnection
            Serial.println(header);
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            if (header.startsWith("GET /light/state"))
            {
              Serial.println("sending state");
              doc["characteristic"] = "On";
              if (lightstate == 1)
              {
                doc["value"] = "true";
              }
              else
              {
                doc["value"] = "false";
              }
              serializeJson(doc, json);
              http.POST(json);
              // client.println("<!DOCTYPE html><html>");
              http.end();
            }
            Serial.println("Did not ask for state, moving on.");
            if (!header.startsWith("GET /light/state"))
            {

              // turns the GPIOs on and off
              if (header.indexOf("GET /light/on") >= 0)
              {
                Serial.println("GPIO 5 on");
                lightstate = 1;
                digitalWrite(output5, HIGH);
                delay(500);
                digitalWrite(output5, LOW);
              }
              else if (header.indexOf("GET /light/off") >= 0)
              {
                Serial.println("GPIO 5 off");
                lightstate = 0;
                digitalWrite(output5, HIGH);
                delay(500);
                digitalWrite(output5, LOW);
              }
              else if (header.indexOf("GET /4?state=on") >= 0)
              {
                Serial.println("GPIO 4 on");
                output4State = 1;
                digitalWrite(output4, HIGH);
                delay(500);
                digitalWrite(output4, LOW);
              }
              else if (header.indexOf("GET /4?state=off") >= 0)
              {
                Serial.println("GPIO 4 off");
                output4State = 0;
                digitalWrite(output4, HIGH);
                delay(500);
                digitalWrite(output4, LOW);
              }
            }

            break;
          }
          else
          { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        }
        else if (c != '\r')
        {                   // if you got anything else but a carriage return character,
          currentLine += c; // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}