#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_NeoPixel.h>

#define NEO_PIN 45
#define LED_COUNT 1
#define LED_PIN GPIO_NUM_48
#define I2C_SDA 11
#define I2C_SCL 12
#define SEND_INTERVAL_MS 10000

const char* wifi_ssid = "HuyBui";
const char* wifi_pass = "huybui1234";

const char* tb_server = "http://app.coreiot.io";
const char* tb_token  = "95el4gf1nym0wx4iz5u9";

Adafruit_AHTX0 dht20;

void readAndSendSensor() {
    sensors_event_t humidity, temp;
    if (!dht20.getEvent(&humidity, &temp)) {
        Serial.println("[DHT20] Đọc cảm biến thất bại");
        return;
    }
    sendDataToThingsBoard(temp.temperature, humidity.relative_humidity);
}

bool sendDataToThingsBoard(float temperature, float humidity) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[TB] WiFi chưa kết nối");
        return false;
    }

    HTTPClient http;
    http.begin(String(tb_server) + "/api/v1/" + tb_token + "/telemetry");
    http.addHeader("Content-Type", "application/json");

    String payload = "{\"temperature\":" + String(temperature, 2) + ",\"humidity\":" + String(humidity, 2) + "}";
    int code = http.POST(payload);
    http.end();

    Serial.printf("[TB] %s -> HTTP %d\n", payload.c_str(), code);
    return code == HTTP_CODE_OK;
}



void neo_blinky(void *pvParameters) {
    Adafruit_NeoPixel strip(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);
    strip.begin();
    strip.clear();
    strip.show();

    const uint32_t colors[] = {
        strip.Color(255, 255, 0), strip.Color(0, 255, 0), strip.Color(0, 0, 255),
        strip.Color(255, 0, 0), strip.Color(0, 0, 0)};

    while (1) {
        for (uint32_t c : colors) {
            strip.setPixelColor(0, c);
            strip.show();
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}

void led_blinky(void *pvParameters) {
    pinMode(LED_PIN, OUTPUT);
    while (1) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void setup() {
    Serial.begin(115200);
    xTaskCreate(led_blinky, "LED Control", 2048, NULL, 2, NULL);
    xTaskCreate(neo_blinky, "NEO Control", 2048, NULL, 2, NULL);

    Wire.begin(I2C_SDA, I2C_SCL);
    if (!dht20.begin()) {
        Serial.println("[DHT20] Không tìm thấy cảm biến!");
    }

    WiFi.begin(wifi_ssid, wifi_pass);
    Serial.print("Đang kết nối WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.printf("\nWiFi OK, IP: %s\n", WiFi.localIP().toString().c_str());
}

void loop() {
    readAndSendSensor();
    delay(SEND_INTERVAL_MS);
}
