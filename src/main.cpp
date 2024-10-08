#include <Arduino.h>
#include <DHTesp.h>
#include <TFT_eSPI.h>

#define DHT_PIN 17      
#define ROTARY_PIN_A 12   
#define ROTARY_PIN_B 13  
#define RELAY_PIN 2       

DHTesp dht;
TFT_eSPI tft = TFT_eSPI();

volatile int encoderValue = 27; 
int lastEncoded = 0;            // 로터리 인코더 상태 저장 변수
unsigned long lastDHTReadMillis = 0;
int interval = 2000;
float humidity = 0;
float temperature = 0;

void IRAM_ATTR handleRotary() {
    int MSB = digitalRead(ROTARY_PIN_A);
    int LSB = digitalRead(ROTARY_PIN_B);
    int encoded = (MSB << 1) | LSB;
    int sum = (lastEncoded << 2) | encoded;

    if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) encoderValue++;
    if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) encoderValue--;

    lastEncoded = encoded;

    // 설정 온도를 제한(0 ~ 60도)
    encoderValue = encoderValue > 60 ? 60 : (encoderValue < 0 ? 0 : encoderValue);
}

void setup() {
    Serial.begin(115200); 
    dht.setup(DHT_PIN, DHTesp::DHT22); 
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    
    pinMode(ROTARY_PIN_A, INPUT_PULLUP);
    pinMode(ROTARY_PIN_B, INPUT_PULLUP);
    pinMode(RELAY_PIN, OUTPUT);
    
    attachInterrupt(ROTARY_PIN_A, handleRotary, CHANGE);
    attachInterrupt(ROTARY_PIN_B, handleRotary, CHANGE);

    Serial.println();
    Serial.println("Humidity (%)\tTemperature (C)");
}

void readDHT22() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastDHTReadMillis >= interval) {
        lastDHTReadMillis = currentMillis;

        humidity = dht.getHumidity();  // 습도 읽기
        temperature = dht.getTemperature();  // 온도 읽기

        if (isnan(humidity) || isnan(temperature)) {
            Serial.println("DHT22 읽기 실패");
        }
    }
}

void loop() {
    readDHT22();

    // TFT 디스플레이에 온도와 설정 값을 표시
    String tempStr = "Temp: " + String(temperature, 1) + " C";
    String setStr = "Set: " + String(encoderValue) + " C";

    tft.fillScreen(TFT_BLACK);
    tft.drawString(tempStr, 10, 40, 4);
    tft.drawString(setStr, 10, 70, 4);

    // 시리얼 모니터에 온도와 습도 출력
    Serial.printf("%.1f\t %.1f\n", temperature, humidity);

    // 릴레이 제어: 설정 온도보다 현재 온도가 낮으면 릴레이 활성화
    if (temperature < encoderValue) {
        digitalWrite(RELAY_PIN, HIGH);
    } else {
        digitalWrite(RELAY_PIN, LOW);
    }

    delay(1000); // 1초 간격으로 루프 실행
}
