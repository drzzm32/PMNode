#include <SPI.h>
#include <Ethernet.h>

byte mac[] = {
  0x32, 0x32, 0x32, 0x32, 0x32, 0x32
};
IPAddress ip(192, 168, 1, 128);
IPAddress dnsIp(192, 168, 1, 1);
IPAddress gateIp(192, 168, 1, 1);
IPAddress maskIp(255, 255, 255, 0);

EthernetClient client;
char server[] = "nya.ac.cn";
//IPAddress server(139, 199, 82, 84);
unsigned long lastConnectionTime = 0;
const unsigned long postingInterval = 10L * 1000L;

#include <SPFD5408_Adafruit_GFX.h>
#include <SPFD5408_Adafruit_TFTLCD.h>
Adafruit_TFTLCD tft(A3, A2, A1, A0, A4);
#define BLACK   0x0000
#define WHITE   0xFFFF
#include <stdarg.h>
#define IOBUF 128
int print(const char* format, ...) {
    char* iobuf = malloc(sizeof(char) * IOBUF);
    va_list args;
    va_start(args, format);
    int result = vsprintf(iobuf, format, args);
    va_end(args);
    if (tft.getCursorY() >= tft.height()) {
        tft.setCursor(0, 0);
        tft.fillScreen(BLACK);
    }
    tft.print(iobuf);
    free(iobuf);
    return result;
}

static uint8_t dataBuf[10];
static short pm25, pm10;

void setup() {
    tft.reset();
    tft.begin(0x9341);
    tft.setRotation(2);
    tft.fillScreen(BLACK);
    tft.setTextColor(WHITE);
    tft.setTextSize(1);
    tft.setCursor(0, 0);
    tft.println("[INFO]");
    tft.println("[INFO] SCREEN OK!");

    tft.println("[INIT] Serial initializing...");
    Serial1.begin(9600);

    tft.println("[INIT] Ethernet initializing...");
    delay(3000);
    if (Ethernet.begin(mac, 10000) == 0) {
        tft.println("[ERROR] DHCP failed");
        tft.println("[INFO] Using default IP");
        Ethernet.begin(mac, ip, dnsIp, gateIp, maskIp);
    }
    tft.print("[INFO] The IP is: ");
    tft.println(Ethernet.localIP());

    tft.println("[INFO] BEGIN SENDING");
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
    
    print("[INFO] PM2.5: ");
    print("%d\n", pm25);
    tft.print("[INFO] PM10: ");
    print("%d\n", pm10);
}

void httpRequest() {
    client.stop();

    if (client.connect(server, 5000)) {
        print("[INFO] Connecting...\n");
        
        client.print("GET /api/set");
        client.print("~id=arduino");
        client.print("&time=2017-6-18T12:22");
        client.print("&pm25=");
        client.print(pm25);
        client.print("&pm10=");
        client.print(pm10);
        client.println(" HTTP/1.1");
        client.println("Host: nya.ac.cn:5000");
        client.println("User-Agent: arduino-ethernet");
        client.println("Connection: close");
        client.println();

        lastConnectionTime = millis();

        print("[INFO] Connection finished\n");
    } else {
        print("[INFO] Connection failed\n");
    }
}

void loop() {
    if (millis() - lastConnectionTime > postingInterval) {
        print("[INFO] BEGIN HTTP REQUEST\n");
        doUpdate();
        httpRequest();
        print("\n");
    }
}
