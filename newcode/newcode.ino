#include <SPI.h>
#include <Ethernet.h>
#include <DHT.h>

int dhtPin = 6;
#define DHTTYPE DHT22
DHT dht(dhtPin, DHTTYPE);

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 54);
IPAddress subnet(255, 255, 255, 0);
IPAddress gateway(192, 168, 1, 1);
int port = 80;
EthernetServer server(port);

float vcc = 0.00;
float R1 = 166800.0;
float R2 = 10000.0;
int voltagePin1 = A1;
int voltagePin2 = A3;
int relayPin = 3;
int fanPin = 7;
int LED = 5;
int outputState = 0;

float temperature;
float humidity;
float voltage;
float voltageAc = 0.00;
String acStatus = "No";
String logs = ""; // Global variable to store all logs

void setup() {
  Serial.begin(9600);
  Ethernet.begin(mac, ip, gateway, subnet);
  server.begin();
  dht.begin();
  pinMode(relayPin, OUTPUT);
  pinMode(LED, OUTPUT);
  analogReference(INTERNAL);

  // Initialize logs with a starting message
  logs = "Arduino Started!<br>";
}

void loop() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  vcc = analogRead(voltagePin1) * (5.0 / 1024.0);
  voltage = vcc / (R2 / (R1 + R2));
  voltageAc = analogRead(voltagePin2) * (5.00 / 1024.0);

  if (voltageAc > 4.99) {
    acStatus = "Yes";
  } else {
    acStatus = "No";
  }

  if (temperature >= 33) {
    digitalWrite(fanPin, HIGH);
  } else {
    digitalWrite(fanPin, LOW);
  }

  digitalWrite(LED, HIGH);
  
  EthernetClient client = server.available();
  if (client) {
    String request = client.readStringUntil('\r');
    client.flush();

    // CORS Headers to Allow Cross-Origin Requests
    client.println("HTTP/1.1 200 OK");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Access-Control-Allow-Methods: GET, POST");
    client.println("Content-Type: text/html");
    client.println();
    
    // HTML Response
    client.println("<html>");
    client.println("<head><title>Arduino Monitor</title>");
    client.println("<meta http-equiv=\"refresh\" content=\"10\">");  // Refresh every 10 seconds
    client.println("<style>body { font-family: Arial; } h1 { color: blue; } </style>");
    client.println("</head>");
    client.println("<body>");
    client.println("<h1>Arduino Sensor Data</h1>");
    
    client.println("<h3>Sensor Readings:</h3>");
    client.print("Temperature: ");
    client.print(temperature);
    client.println(" &deg;C<br><br>");
    
    client.print("Humidity: ");
    client.print(humidity);
    client.println(" %<br><br>");
    
    client.print("Voltage: ");
    client.print(voltage);
    client.println(" V<br><br>");
    
    client.print("AC Status: ");
    client.print(acStatus);
    client.println("<br><br>");
    
    client.println("<h3>Control Generator</h3>");
    client.println("<form method=\"get\">");
    client.println("<input type=\"submit\" name=\"off\" value=\"Turn OFF\">");
    client.println("<input type=\"submit\" name=\"on\" value=\"Turn ON\">");
    client.println("</form>");
    
    // Generator Control
    if (request.indexOf("GET /?on") >= 0) {
      outputState = 1;
      digitalWrite(relayPin, HIGH);
      logs += "Turn ON Generator<br>"; // Append log entry
    }
    
    if (request.indexOf("GET /?off") >= 0) {
      outputState = 0;
      digitalWrite(relayPin, LOW);
      logs += "Turn OFF Generator<br>"; // Append log entry
    }
    
    client.print("<br>Generator Status: ");
    client.println(outputState ? "ON" : "OFF");

    // Send all logs
    client.println("<h3>Recent Activity:</h3>");
    client.print("<p>");
    client.print(logs); // Send all logs
    client.println("</p>");
    
    client.println("</body></html>");
    client.stop();
  }
}
