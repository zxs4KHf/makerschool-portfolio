#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

WebServer server(80);
Servo myServo;
const int servoPin = 13;

void setup() {
  Serial.begin(115200);
  myServo.attach(servoPin);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", []() {
    String html = "<html><body><h1>ESP32 Servo Control</h1>";
    html += "<a href=\"/set?angle=0\"><button>0 Deg</button></a> ";
    html += "<a href=\"/set?angle=90\"><button>90 Deg</button></a> ";
    html += "<a href=\"/set?angle=180\"><button>180 Deg</button></a>";
    html += "</body></html>";
    server.send(200, "text/html", html);
  });

  server.on("/set", []() {
    if (server.hasArg("angle")) {
      int angle = server.arg("angle").toInt();
      myServo.write(angle);
    }
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.begin();
}

void loop() {
  server.handleClient();
}
