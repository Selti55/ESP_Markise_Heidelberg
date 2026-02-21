/**
 * ESP32-Sender für Markisensteuerung
 * LILYGO T-Energy-S3 mit 6 Tastern und RGB-LED
 * 
 * Features:
 * - Deep Sleep mit Aufwecken durch Taster
 * - Inaktivitäts-Timeout (30 Sekunden)
 * - Mehrfach-Taster-Erkennung
 * - RGB-LED Statusanzeige
 * - Batterie-Überwachung
 * - ESP-NOW Kommunikation
 */

#include <esp_now.h>
#include <WiFi.h>
#include "esp_sleep.h"

// ============== KONFIGURATION ==============
// Inaktivitäts-Timeout (Sekunden ohne Tastendruck bis Deep Sleep)
#define INACTIVITY_TIMEOUT 30

// Entprellzeit für Taster (ms)
#define DEBOUNCE_DELAY 50

// Maximale Taster-Haltezeit (ms) - danach wird loslassen erzwungen
#define BUTTON_HOLD_TIMEOUT 5000

// Batterie-Referenzspannungen
#define BATTERY_MIN_VOLTAGE 3.3  // Kritische Schwelle
#define BATTERY_FULL_VOLTAGE 4.1  // Voll geladen

// ============== GPIO DEFINITIONEN ==============
// Taster GPIOs (alle mit Weckfunktion)
const int buttonPins[6] = {1, 2, 4, 5, 6, 7};
const uint64_t buttonBitMask = (1ULL << 1) | (1ULL << 2) | (1ULL << 4) | 
                               (1ULL << 5) | (1ULL << 6) | (1ULL << 7);

// RGB-LED GPIOs (gemeinsame Anode)
#define LED_RED_PIN   10
#define LED_GREEN_PIN 11
#define LED_BLUE_PIN  12

// USB Detect (für Ladeerkennung)
#define USB_DETECT_PIN 15

// Batterie-Messung (ADC)
#define BATTERY_ADC_PIN 3

// ============== ESP-NOW KONFIGURATION ==============
// MAC-Adresse des Empfängers (HIER EINTRAGEN!)
// Format: {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}
uint8_t receiverMac[] = {0xFC, 0xF5, 0xC4, 0x67, 0xA8, 0xE4}; // Beispiel-MAC

// Nachrichtenstruktur für ESP-NOW
typedef struct struct_message {
  uint8_t buttonMask;      // Bit 0-5: Taster 1-6
  float batteryVoltage;    // Batteriespannung in Volt
  uint8_t sequence;        // Sequenznummer (gegen doppelte Ausführung)
  uint8_t rssi;            // Signalstärke (optional)
} struct_message;

struct_message myData;

// ============== GLOBALE VARIABLEN ==============
unsigned long lastButtonPress = 0;
bool awakeFromSleep = false;
bool isCharging = false;
float batteryVoltage = 0.0;
uint8_t sequenceNumber = 0;

// ============== LED FUNKTIONEN ==============
/**
 * Initialisiert die RGB-LED Pins
 * Bei gemeinsamer Anode: LOW = EIN, HIGH = AUS
 */
void initLEDs() {
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);
  
  // Alle LEDs ausschalten
  digitalWrite(LED_RED_PIN, HIGH);
  digitalWrite(LED_GREEN_PIN, HIGH);
  digitalWrite(LED_BLUE_PIN, HIGH);
}

/**
 * Setzt die RGB-LED Farbe
 * @param red   true = Rot ein
 * @param green true = Grün ein
 * @param blue  true = Blau ein
 */
void setLEDColor(bool red, bool green, bool blue) {
  digitalWrite(LED_RED_PIN, red ? LOW : HIGH);
  digitalWrite(LED_GREEN_PIN, green ? LOW : HIGH);
  digitalWrite(LED_BLUE_PIN, blue ? LOW : HIGH);
}

/**
 * LED-Status: Aufwachen (Cyan kurz)
 */
void ledStatusAwake() {
  setLEDColor(false, true, true);  // Cyan
  delay(100);
  setLEDColor(false, false, false);
}

/**
 * LED-Status: Taster OK (Grün kurz)
 */
void ledStatusButtonOK() {
  setLEDColor(false, true, false);  // Grün
  delay(100);
  setLEDColor(false, false, false);
}

/**
 * LED-Status: Mehrfach-Taster Fehler (Rot 3x)
 */
void ledStatusMultiButtonError() {
  for (int i = 0; i < 3; i++) {
    setLEDColor(true, false, false);  // Rot
    delay(200);
    setLEDColor(false, false, false);
    delay(100);
  }
}

/**
 * LED-Status: Senden (Blau 200ms)
 */
void ledStatusSending() {
  setLEDColor(false, false, true);  // Blau
  delay(200);
  setLEDColor(false, false, false);
}

/**
 * LED-Status: Sendebestätigung (Grün 2x kurz)
 */
void ledStatusSendSuccess() {
  for (int i = 0; i < 2; i++) {
    setLEDColor(false, true, false);  // Grün
    delay(100);
    setLEDColor(false, false, false);
    delay(80);
  }
}

/**
 * LED-Status: Sendefehler (Rot 5x schnell)
 */
void ledStatusSendError() {
  for (int i = 0; i < 5; i++) {
    setLEDColor(true, false, false);  // Rot
    delay(50);
    setLEDColor(false, false, false);
    delay(50);
  }
}

/**
 * LED-Status: Batterie kritisch (Rot langsam atmend)
 */
void ledStatusBatteryCritical() {
  setLEDColor(true, false, false);  // Rot
  delay(2000);
  setLEDColor(false, false, false);
  delay(2000);
}

/**
 * LED-Status: Batterie lädt (Orange pulsierend)
 */
void ledStatusCharging() {
  setLEDColor(true, true, false);  // Orange
  delay(1000);
  setLEDColor(false, false, false);
  delay(1000);
}

/**
 * LED-Status: Batterie voll (Grün dauerhaft)
 */
void ledStatusBatteryFull() {
  setLEDColor(false, true, false);  // Grün dauerhaft
}

/**
 * LED-Status: Inaktiv-Timeout (Cyan 1s)
 */
void ledStatusInactivityTimeout() {
  setLEDColor(false, true, true);  // Cyan
  delay(1000);
  setLEDColor(false, false, false);
}

// ============== BATTERIE FUNKTIONEN ==============
/**
 * Misst die Batteriespannung
 * @return Spannung in Volt
 */
float getBatteryVoltage() {
  pinMode(BATTERY_ADC_PIN, INPUT_PULLDOWN);
  delay(10);
  int raw = analogRead(BATTERY_ADC_PIN);
  
  // Formel für LILYGO T-Energy-S3
  // Referenz: 3.3V, Spannungsteiler 1:2, ADC 12-Bit (0-4095)
  float voltage = (raw / 4095.0) * 2.0 * 3.3;
  
  // Korrekturfaktor für genauere Messung
  voltage = voltage * 1.02;  // 2% Korrektur
  
  return voltage;
}

/**
 * Prüft den Batteriestatus und zeigt LED an
 */
void checkBatteryStatus() {
  batteryVoltage = getBatteryVoltage();
  
  // Prüfen ob USB angeschlossen (lädt)
  pinMode(USB_DETECT_PIN, INPUT);
  isCharging = (digitalRead(USB_DETECT_PIN) == HIGH);
  
  Serial.print("Batteriespannung: ");
  Serial.print(batteryVoltage);
  Serial.print("V, USB: ");
  Serial.println(isCharging ? "angeschlossen" : "nicht angeschlossen");
  
  // LED-Status basierend auf Batterie
  if (isCharging) {
    if (batteryVoltage > BATTERY_FULL_VOLTAGE) {
      ledStatusBatteryFull();
    } else {
      ledStatusCharging();
    }
  } else if (batteryVoltage < BATTERY_MIN_VOLTAGE) {
    ledStatusBatteryCritical();
  }
}

// ============== ESP-NOW FUNKTIONEN ==============
/**
 * Callback wenn Daten gesendet wurden
 */
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Sendestatus: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Erfolg" : "Fehler");
  
  if (status == ESP_NOW_SEND_SUCCESS) {
    ledStatusSendSuccess();
  } else {
    ledStatusSendError();
  }
}

/**
 * Initialisiert ESP-NOW
 */
void initESPNOW() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init fehlgeschlagen!");
    ledStatusSendError();
    return;
  }
  
  esp_now_register_send_cb(OnDataSent);
  
  // Peer registrieren
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, receiverMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Peer hinzufügen fehlgeschlagen!");
    ledStatusSendError();
    return;
  }
  
  Serial.println("ESP-NOW initialisiert");
  Serial.print("Empfänger MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", receiverMac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
}

/**
 * Sendet Tasterstatus per ESP-NOW
 * @param buttonMask Bitmaske der gedrückten Taster
 */
void sendButtonStatus(uint8_t buttonMask) {
  // Nicht senden wenn mehrere Taster gedrückt
  if (buttonMask == 0 || (buttonMask & (buttonMask - 1)) != 0) {
    Serial.println("Mehrere Taster gedrückt - sende NICHT!");
    ledStatusMultiButtonError();
    return;
  }
  
  // Nachricht vorbereiten
  myData.buttonMask = buttonMask;
  myData.batteryVoltage = batteryVoltage;
  myData.sequence = sequenceNumber++;
  myData.rssi = WiFi.RSSI();
  
  // Senden
  ledStatusSending();
  
  esp_err_t result = esp_now_send(receiverMac, (uint8_t *) &myData, sizeof(myData));
  
  if (result == ESP_OK) {
    Serial.println("Senden gestartet");
  } else {
    Serial.println("Fehler beim Senden");
    ledStatusSendError();
  }
}

// ============== TASTER FUNKTIONEN ==============
/**
 * Liest alle Taster aus und gibt Bitmaske zurück
 * @return Bit 0-5: Taster 1-6 (1 = gedrückt)
 */
uint8_t readButtons() {
  uint8_t mask = 0;
  int pressedCount = 0;
  
  for (int i = 0; i < 6; i++) {
    if (digitalRead(buttonPins[i]) == LOW) {
      delay(DEBOUNCE_DELAY);
      if (digitalRead(buttonPins[i]) == LOW) {
        mask |= (1 << i);
        pressedCount++;
      }
    }
  }
  
  // Warnung bei Mehrfach-Taster
  if (pressedCount > 1) {
    Serial.print("WARNUNG: ");
    Serial.print(pressedCount);
    Serial.println(" Taster gleichzeitig gedrückt!");
    ledStatusMultiButtonError();
  }
  
  return mask;
}

/**
 * Prüft Taster und sendet bei Bedarf
 */
void checkButtonsAndSend() {
  uint8_t buttonMask = readButtons();
  
  if (buttonMask != 0) {
    // Prüfen ob mehrere Taster
    if ((buttonMask & (buttonMask - 1)) != 0) {
      Serial.println("Mehrere Taster - keine Aktion");
      // LED zeigt bereits Fehler durch readButtons()
    } else {
      // Einzelner Taster
      int buttonIndex = 0;
      while (!(buttonMask & (1 << buttonIndex))) buttonIndex++;
      
      Serial.print("Taster ");
      Serial.print(buttonIndex + 1);
      Serial.println(" gedrückt (einzeln)");
      
      ledStatusButtonOK();
      sendButtonStatus(buttonMask);
      
      // Warten bis Taster losgelassen (mit Timeout)
      unsigned long pressStart = millis();
      while ((readButtons() & (1 << buttonIndex)) && 
             ((millis() - pressStart) < BUTTON_HOLD_TIMEOUT)) {
        delay(10);
      }
    }
    
    lastButtonPress = millis();
  }
}

// ============== DEEP SLEEP FUNKTIONEN ==============
/**
 * Geht in Deep Sleep (alle Taster als Weckquelle)
 */
void goToDeepSleep() {
  Serial.println("Gehe in Deep Sleep...");
  ledStatusInactivityTimeout();
  delay(1100);  // LED zeigen
  
  Serial.flush();
  
  // Alle 6 Taster als Weckquelle konfigurieren
  // LOW = Taster gedrückt (wegen Pull-up)
  esp_sleep_enable_ext1_wakeup(buttonBitMask, ESP_EXT1_WAKEUP_ALL_LOW);
  
  // LEDs ausschalten vor Sleep
  setLEDColor(false, false, false);
  
  // In Deep Sleep gehen
  esp_deep_sleep_start();
}

// ============== SETUP ==============
void setup() {
  // Serielle Kommunikation starten
  Serial.begin(115200);
  delay(100);
  Serial.println("\n\n=== ESP32-Sender Markisensteuerung ===");
  
  // LEDs initialisieren
  initLEDs();
  
  // Aufwachgrund ermitteln
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  
  switch(wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT1:
      Serial.println("Aufgewacht durch Tastendruck");
      awakeFromSleep = true;
      ledStatusAwake();
      
      // Welcher Taster hat geweckt?
      uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
      Serial.print("Geweckt von GPIO: ");
      for (int i = 0; i < 6; i++) {
        if (wakeup_pin_mask & (1ULL << buttonPins[i])) {
          Serial.print(buttonPins[i]);
          Serial.print(" ");
        }
      }
      Serial.println();
      break;
      
    case ESP_SLEEP_WAKEUP_TIMER:
      Serial.println("Aufgewacht durch Timer");
      ledStatusAwake();
      break;
      
    default:
      Serial.println("Neustart (Power-On Reset)");
      ledStatusAwake();
      break;
  }
  
  // Taster konfigurieren (INPUT_PULLUP für interne Pull-ups)
  for (int i = 0; i < 6; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    Serial.printf("Taster %d an GPIO %d\n", i+1, buttonPins[i]);
  }
  
  // Batterie prüfen
  checkBatteryStatus();
  
  // ESP-NOW initialisieren
  initESPNOW();
  
  // Letzten Tastendruck initialisieren
  lastButtonPress = millis();
  
  Serial.println("System bereit - warte auf Tastendruck...");
  Serial.printf("Inaktivitäts-Timeout: %d Sekunden\n", INACTIVITY_TIMEOUT);
  Serial.println("=====================================\n");
  
  delay(500);
}

// ============== LOOP ==============
void loop() {
  // Taster abfragen
  checkButtonsAndSend();
  
  // Batterie alle 60 Sekunden prüfen (wenn wach)
  static unsigned long lastBatteryCheck = 0;
  if (millis() - lastBatteryCheck > 60000) {
    checkBatteryStatus();
    lastBatteryCheck = millis();
  }
  
  // Prüfen ob Inaktivitäts-Timeout erreicht
  if ((millis() - lastButtonPress) > (INACTIVITY_TIMEOUT * 1000)) {
    Serial.printf("Inaktivitäts-Timeout (%d Sekunden) erreicht\n", INACTIVITY_TIMEOUT);
    goToDeepSleep();
  }
  
  delay(50);  // Kleine Pause für CPU-Entlastung
}