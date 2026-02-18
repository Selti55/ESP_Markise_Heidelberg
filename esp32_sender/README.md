# ESP32-Sender für Markisensteuerung

Batteriebetriebener Sender mit 6 Tastern zur Ansteuerung einer Markise. **Die Taster wecken den ESP32 aus dem Deep Sleep.** Nach einer einstellbaren Inaktivitätszeit (kein Tastendruck) kehrt der ESP32 automatisch in den Deep Sleep zurück. Betrieb mit **18650 Li-Ion Akku** auf einem **LILYGO T-Energy-S3** Board mit integrierter Ladeschaltung.

## 🎯 Gewählte Hardware: **LILYGO T-Energy-S3**

| Eigenschaft | Beschreibung |
|-------------|--------------|
| **Modell** | LILYGO T-Energy-S3 |
| **ESP32-Typ** | ESP32-S3 (Dual-Core, 240 MHz) |
| **Batteriehalter** | Integrierter 18650 Halter auf der Rückseite |
| **Ladeschaltung** | HX6610S Laderegler, USB-C Ladung |
| **Schutzfunktionen** | Verpolungsschutz, Überladung, Tiefentladung |
| **Besonderheiten** | Battery-ADC an GPIO3 (Spannungsmessung), Qwiic/STEMMA Anschluss |
| **Deep Sleep** | Extrem geringer Stromverbrauch (ca. 5-20 µA) |
| **Preis** | ca. 17-20€ |

## 🔌 GPIO-Belegung (LILYGO T-Energy-S3)

| Taster | GPIO | Interne Pull-ups | Weck-Pin |
|--------|------|------------------|----------|
| Taster 1 | GPIO 1 | Ja (INPUT_PULLUP) | ✅ GPIO 1 |
| Taster 2 | GPIO 2 | Ja | ✅ GPIO 2 |
| Taster 3 | GPIO 4 | Ja | ✅ GPIO 4 |
| Taster 4 | GPIO 5 | Ja | ✅ GPIO 5 |
| Taster 5 | GPIO 6 | Ja | ✅ GPIO 6 |
| Taster 6 | GPIO 7 | Ja | ✅ GPIO 7 |
| **Battery ADC** | GPIO 3 | - | ❌ |

**Wichtig:** Alle 6 Taster können als Weckquelle aus dem Deep Sleep verwendet werden! Der ESP32-S3 unterstützt das Aufwecken durch externe GPIOs (EXT1 Wakeup).

## ⏱️ Gewünschtes Verhalten

- **Aufwecken:** Jeder der 6 Taster weckt den ESP32 aus dem Deep Sleep
- **Aktivphase:** Nach dem Aufwachen bleibt der ESP32 wach und fragt die Taster ab
- **Senden:** Bei jedem Tastendruck wird der Status per ESP-NOW gesendet
- **Inaktivitäts-Timeout:** Nach **x Sekunden ohne Tastendruck** (konfigurierbar) geht der ESP32 zurück in den Deep Sleep
- **Manueller Tiefschlaf:** Auch möglich durch langen Tastendruck (optional)

## 🔧 Implementierung des Weckverhaltens

### Grundprinzip

Der ESP32-S3 kann aus dem Deep Sleep durch **externe GPIOs (EXT1)** aufgeweckt werden. Alle 6 Taster sind als Weckquelle konfiguriert.

### Code-Struktur

```cpp
#include <esp_now.h>
#include <WiFi.h>
#include "esp_sleep.h"

// ============== KONFIGURATION ==============
#define uS_TO_S_FACTOR 1000000ULL
#define INACTIVITY_TIMEOUT 30  // x Sekunden ohne Tastendruck bis Deep Sleep

// Taster GPIOs (alle als Weckquelle)
const int buttonPins[6] = {1, 2, 4, 5, 6, 7};
const uint64_t buttonBitMask = (1ULL << 1) | (1ULL << 2) | (1ULL << 4) | 
                               (1ULL << 5) | (1ULL << 6) | (1ULL << 7);

// ESP-NOW Konfiguration
uint8_t receiverMac[] = {0x34, 0x85, 0x18, 0x78, 0x92, 0xAC}; // Empfänger-MAC

// ============== VARIABLEN ==============
unsigned long lastButtonPress = 0;
bool awakeFromSleep = false;

// ============== SETUP ==============
void setup() {
  Serial.begin(115200);
  delay(100); // Für Serial Monitor
  
  // Aufwachgrund ermitteln
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  
  switch(wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT1:
      Serial.println("Aufgewacht durch Tastendruck");
      awakeFromSleep = true;
      break;
    default:
      Serial.println("Neustart (Power-On Reset)");
      awakeFromSleep = false;
  }
  
  // Taster konfigurieren
  for (int i = 0; i < 6; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
  
  // ESP-NOW initialisieren
  initESPNOW();
  
  // Letzten Tastendruck initialisieren
  lastButtonPress = millis();
  
  // Sofortige Tasterabfrage nach Aufwachen
  checkButtonsAndSend();
}

// ============== LOOP ==============
void loop() {
  // Taster abfragen
  checkButtonsAndSend();
  
  // Prüfen, ob Inaktivitäts-Timeout erreicht
  if ((millis() - lastButtonPress) > (INACTIVITY_TIMEOUT * 1000)) {
    Serial.println("Inaktivitäts-Timeout - gehe in Deep Sleep");
    goToDeepSleep();
  }
  
  delay(50); // Kleine Pause zur Entprellung
}

// ============== FUNKTIONEN ==============
void checkButtonsAndSend() {
  for (int i = 0; i < 6; i++) {
    if (digitalRead(buttonPins[i]) == LOW) { // Taster gedrückt (Pull-up, daher LOW)
      delay(50); // Einfache Entprellung
      if (digitalRead(buttonPins[i]) == LOW) {
        // Taster bestätigt
        Serial.print("Taster ");
        Serial.print(i+1);
        Serial.println(" gedrückt");
        
        // Nachricht senden
        sendButtonStatus(i);
        
        // Warten bis Taster losgelassen (mit Timeout)
        unsigned long pressStart = millis();
        while (digitalRead(buttonPins[i]) == LOW) {
          if ((millis() - pressStart) > 5000) { // Max 5 Sekunden gedrückt halten
            break;
          }
          delay(10);
        }
        
        // Letzten Tastendruck aktualisieren
        lastButtonPress = millis();
      }
    }
  }
}

void sendButtonStatus(int buttonIndex) {
  // Hier ESP-NOW Code einfügen
  // Siehe Beispiel im vorherigen README
}

void initESPNOW() {
  WiFi.mode(WIFI_STA);
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init fehlgeschlagen");
    return;
  }
  
  // Peer konfigurieren etc.
  // Siehe vorheriges README
}

void goToDeepSleep() {
  Serial.println("Aktiviere Deep Sleep...");
  Serial.flush();
  
  // Alle 6 Taster als Weckquelle konfigurieren
  // LOW = Taster gedrückt (wegen Pull-up)
  esp_sleep_enable_ext1_wakeup(buttonBitMask, ESP_EXT1_WAKEUP_ALL_LOW);
  
  // In Deep Sleep gehen
  esp_deep_sleep_start();
}

// Batteriespannung auslesen (optional)
float getBatteryVoltage() {
  pinMode(3, INPUT_PULLDOWN);
  int raw = analogRead(3);
  float voltage = (raw / 4095.0) * 2.0 * 1.1 * 4.2; 
  return voltage;
}