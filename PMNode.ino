#include <SPI.h>
#include <Ethernet.h>

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 1, 128);
EthernetServer server(80);

#include <SPFD5408_Adafruit_GFX.h>
#include <SPFD5408_Adafruit_TFTLCD.h>
#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0
#define LCD_RESET A4
Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

static uint8_t dataBuf[10];
static short pm25, pm10;

void setup() {
    Serial.begin(9600);
    Serial1.begin(9600);

    Serial.println("Initialize TFT");
    tft.reset();
    tft.begin(0x9341);
    tft.setRotation(2);
    tft.fillScreen(0xFFFF);
    tft.setCursor(0, 0);
    tft.setTextColor(0x0000, 0xFFFF); tft.setTextSize(3);

    Serial.println("Initialize ETH");
    Ethernet.begin(mac, ip);
    server.begin();
    Serial.print("Server is at ");
    Serial.println(Ethernet.localIP());

    Serial.println("Initialize finished");
    delay(1000);
}

bool checkPacket() {
    if (dataBuf[9] != 0xAB) return false;
    uint8_t sum = 0;
    for (uint8_t i = 2; i <=7; i++)
        sum += dataBuf[i];
    return sum == dataBuf[8];
}

void doUpdate() {
    if (Serial1.peek() == 0xAA) {
        for (uint8_t i = 0; i < 10; i++)
            dataBuf[i] = Serial1.read();
    } else {
        Serial1.read();
    }

    if (checkPacket()) {
        pm25 = (dataBuf[3] * 256 + dataBuf[2]) / 10;
        pm10 = (dataBuf[5] * 256 + dataBuf[4]) / 10;
    }

    tft.setCursor(16, 32);
    tft.print("PM2.5: ");
    tft.setCursor(128, 32);
    if (pm25 >= 1000) tft.print(" ");
    tft.print(pm25);
    tft.setCursor(16, 64);
    tft.print("PM10: ");
    tft.setCursor(128, 64);
    if (pm10 >= 1000) tft.print(" ");
    tft.print(pm10);
}

void loop() {
    /*
    Serial.print("Loop: ");
    for (uint8_t i = 0; i < 10; i++) {
        Serial.print(dataBuf[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
    */
    
    doUpdate();

    EthernetClient client = server.available();
    if (client) {
        Serial.println("New client");
        boolean currentLineIsBlank = true;
        while (client.connected()) { 
            if (client.available()) {
                char c = client.read();
                Serial.write(c);
            
                if (c == '\n' && currentLineIsBlank) {
                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-Type: text/html");
                    client.println("Connection: close");
                    client.println("Refresh: 5");
                    client.println();
                    client.println("<!DOCTYPE HTML>");
                    client.println("<html>");

                    client.print("PM2.5: ");
                    client.print(pm25);
                    client.println("<br />");

                    client.print("PM10: ");
                    client.print(pm10);
                    client.println("<br />");
                    
                    client.println("</html>");
                    break;
                }
                if (c == '\n') {
                    currentLineIsBlank = true;
                } else if (c != '\r') {
                    currentLineIsBlank = false;
                }
            }
            doUpdate();
        }
        delay(1);
        client.stop();
        Serial.println("Client disconnected");
    }
}
