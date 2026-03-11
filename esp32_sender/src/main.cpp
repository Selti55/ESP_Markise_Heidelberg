/**
 * ESP32-Sender für Markisensteuerung
 * LILYGO T-Energy-S3 mit 6 Tastern und RGB-LED
 *
 * Features:
 * - Deep Sleep mit Aufwachen durch Taster
 * - Inaktivitäts-Timeout (30 Sekunden)
 * - Motor EIN solange Taster gedrückt (periodische ESP-NOW-Pakete)
 * - Mehrfach-Taster-Erkennung (Mehrfachdruck wird verworfen)
 * - RGB-LED Statusanzeige
 * - Batterie-Überwachung mit Kalibrierung
 * - ESP-NOW Kommunikation zum Empfänger (ESP32 WROOM)
 */

#include <esp_now.h>
#include <WiFi.h>
#include "esp_sleep.h"

// =================== KONFIGURATION ===================

// Inaktivitäts-Timeout (Sekunden ohne Tastendruck bis Deep Sleep)
#define INACTIVITY_TIMEOUT 30

// Entprellzeit für Taster (ms)
#define DEBOUNCE_DELAY 50

// Maximale Taster-Haltezeit (ms) – Sicherheitslimit
#define BUTTON_HOLD_TIMEOUT 10000  // 10 s

// Batterie-Referenzspannungen (für LED-Status, nicht für ADC)
#define BATTERY_MIN_VOLTAGE 3.3   // Kritische Schwelle
#define BATTERY_FULL_VOLTAGE 4.1  // Voll geladen

// Wie oft bei gehaltenem Taster gesendet wird (Motor EIN solange Pakete kommen)
#define HOLD_SEND_INTERVAL 100  // ms

// =================== GPIO DEFINITIONEN ===================

// Taster GPIOs (alle als Wake-Quelle, Pull-Up, gegen GND)
const int buttonPins[6] = {1, 2, 4, 5, 6, 7};
const uint64_t buttonBitMask =
    (1ULL << 1) | (1ULL << 2) | (1ULL << 4) |
    (1ULL << 5) | (1ULL << 6) | (1ULL << 7);

// RGB-LED GPIOs (gemeinsame Anode: LOW = EIN, HIGH = AUS)
#define LED_RED_PIN   10
#define LED_GREEN_PIN 11
#define LED_BLUE_PIN  12

// USB Detect (für Ladeerkennung, Board-spezifisch)
#define USB_DETECT_PIN 15

// Batterie-Messung (ADC-Pin, an VBAT-Teiler des T-Energy S3)
#define BATTERY_ADC_PIN 3

// =================== ESP-NOW KONFIGURATION ===================

// MAC-Adresse des Empfängers (ESP32 WROOM) – HIER DEINE EMPFÄNGER-MAC EINTRAGEN!
uint8_t receiverMac[] = {0xFC, 0xF5, 0xC4, 0x67, 0xA8, 0xE4};

// Nachrichtenstruktur – muss exakt zur Empfängerseite passen
typedef struct struct_message {
  uint8_t buttonMask;      // Bit 0-5: Taster 1-6
  float   batteryVoltage;  // Batteriespannung in Volt (kalibriert)
  uint8_t sequence;        // Sequenznummer
  uint8_t rssi;            // Signalstärke (vom Sender gemessen)
} struct_message;

struct_message myData;

// =================== GLOBALE VARIABLEN ===================

unsigned long lastButtonPress = 0;  // Zeitpunkt des letzten erkkannten Tastendrucks
bool awakeFromSleep = false;        // Flag: wurde aus Deep Sleep geweckt?
bool isCharging = false;            // USB/Laden erkannt?
float batteryVoltage = 0.0;         // letzte gemessene Batteriespannung
uint8_t sequenceNumber = 0;         // laufende ESP-NOW-Sequenznummer

// =================== LED-FUNKTIONEN ===================

void initLEDs() {
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);

  // Alle LEDs aus (gemeinsame Anode -> HIGH = AUS)
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
  digitalWrite(LED_RED_PIN,   red   ? LOW : HIGH);
  digitalWrite(LED_GREEN_PIN, green ? LOW : HIGH);
  digitalWrite(LED_BLUE_PIN,  blue  ? LOW : HIGH);
}

// Aufwachen (kurzer Cyan-Puls)
void ledStatusAwake() {
  setLEDColor(false, true, true);  // Cyan
  delay(100);
  setLEDColor(false, false, false);
}

// Taster OK (kurz Grün)
void ledStatusButtonOK() {
  setLEDColor(false, true, false);  // Grün
  delay(100);
  setLEDColor(false, false, false);
}

// Mehrfach-Taster Fehler (Rot 3x)
void ledStatusMultiButtonError() {
  for (int i = 0; i < 3; i++) {
    setLEDColor(true, false, false);
    delay(200);
    setLEDColor(false, false, false);
    delay(100);
  }
}

// Senden (Blau kurz)
void ledStatusSending() {
  setLEDColor(false, false, true);  // Blau
  delay(200);
  setLEDColor(false, false, false);
}

// Sendebestätigung (Grün 2x)
void ledStatusSendSuccess() {
  for (int i = 0; i < 2; i++) {
    setLEDColor(false, true, false);
    delay(100);
    setLEDColor(false, false, false);
    delay(80);
  }
}

// Sendefehler (Rot 5x schnell)
void ledStatusSendError() {
  for (int i = 0; i < 5; i++) {
    setLEDColor(true, false, false);
    delay(50);
    setLEDColor(false, false, false);
    delay(50);
  }
}

// Batterie kritisch (langsam Rot)
void ledStatusBatteryCritical() {
  setLEDColor(true, false, false);
  delay(2000);
  setLEDColor(false, false, false);
  delay(2000);
}

// Batterie lädt (Orange pulsierend)
void ledStatusCharging() {
  setLEDColor(true, true, false);
  delay(1000);
  setLEDColor(false, false, false);
  delay(1000);
}

// Batterie voll (Grün dauerhaft)
void ledStatusBatteryFull() {
  setLEDColor(false, true, false);
}

// Inaktiv-Timeout (Cyan 1 s)
void ledStatusInactivityTimeout() {
  setLEDColor(false, true, true);
  delay(1000);
  setLEDColor(false, false, false);
}

// =================== BATTERIE-FUNKTIONEN ===================

/**
 * Misst die Batteriespannung über den Board-internen Teiler.
 *
 * Kalibrierung:
 *  - Gemessen am Akku: ~4,07 V
 *  - ADC raw: ~2442
 *  -> k = 4,07 / 2442 ≈ 0,001667 V pro ADC-Step
 */
float getBatteryVoltage() {
  pinMode(BATTERY_ADC_PIN, INPUT_PULLDOWN);
  delay(10);
  int raw = analogRead(BATTERY_ADC_PIN);

  // Optional zur Kontrolle:
  Serial.print("ADC raw: ");
  Serial.println(raw);

  // Kalibrierte Umrechnung von ADC-Rohwert in Volt:
  const float K = 0.001667f;   // ggf. fein nachjustieren
  float voltage = raw * K;
  return voltage;
}

/**
 * Prüft den Batteriestatus und aktualisiert globale Variablen + LED-Status.
 */
void checkBatteryStatus() {
  batteryVoltage = getBatteryVoltage();

  // USB-Detect (lädt?)
  pinMode(USB_DETECT_PIN, INPUT);
  isCharging = (digitalRead(USB_DETECT_PIN) == HIGH);

  Serial.print("Batteriespannung: ");
  Serial.print(batteryVoltage, 2);
  Serial.print(" V, USB: ");
  Serial.println(isCharging ? "angeschlossen" : "nicht angeschlossen");

  // LED anhand des Status
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

// =================== ESP-NOW-FUNKTIONEN ===================

/**
 * Callback nach Sendeversuch (ESP-NOW).
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
 * Initialisiert ESP-NOW im STA-Mode und trägt den Empfänger als Peer ein.
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

  // Empfänger-Peer hinzufügen
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, receiverMac, 6);
  peerInfo.channel = 0;   // gleichen Kanal wie STA benutzen
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
 * Sendet Tasterstatus per ESP-NOW.
 * @param buttonMask Bitmaske der gedrückten Taster
 */
void sendButtonStatus(uint8_t buttonMask) {
  // Nicht senden, wenn mehrere Taster gedrückt (Sicherheit)
  if (buttonMask != 0 && (buttonMask & (buttonMask - 1)) != 0) {
    Serial.println("Mehrere Taster gedrückt - sende NICHT!");
    ledStatusMultiButtonError();
    return;
  }

  // Nachricht füllen
  myData.buttonMask     = buttonMask;
  myData.batteryVoltage = batteryVoltage;
  myData.sequence       = sequenceNumber++;
  myData.rssi           = WiFi.RSSI();

  // Senden
  ledStatusSending();
  esp_err_t result = esp_now_send(receiverMac, (uint8_t*)&myData, sizeof(myData));

  if (result == ESP_OK) {
    Serial.println("Senden gestartet");
  } else {
    Serial.println("Fehler beim Senden (esp_now_send)");
    ledStatusSendError();
  }
}

// =================== TASTER-FUNKTIONEN ===================

/**
 * Liest alle Taster ein (entprellt) und liefert eine Bitmaske.
 * Bit 0-5: Taster 1-6 (1 = gedrückt).
 */
uint8_t readButtons() {
  uint8_t mask = 0;
  int pressedCount = 0;

  for (int i = 0; i < 6; i++) {
    if (digitalRead(buttonPins[i]) == LOW) {  // Taster gegen GND, Pull-Up aktiv
      delay(DEBOUNCE_DELAY);
      if (digitalRead(buttonPins[i]) == LOW) {
        mask |= (1 << i);
        pressedCount++;
      }
    }
  }

  if (pressedCount > 1) {
    Serial.print("WARNUNG: ");
    Serial.print(pressedCount);
    Serial.println(" Taster gleichzeitig gedrückt!");
    ledStatusMultiButtonError();
  }

  return mask;
}

/**
 * Prüft Taster und sendet solange periodisch, wie EIN Taster gehalten wird.
 * Motorseite: Motor EIN solange Pakete kommen.
 */
void checkButtonsAndSend() {
  uint8_t buttonMask = readButtons();

  if (buttonMask != 0) {
    // Mehrere Taster – keine Aktion (Fehler ist schon angezeigt)
    if ((buttonMask & (buttonMask - 1)) != 0) {
      Serial.println("Mehrere Taster - keine Aktion");
    } else {
      // Einzelner Taster
      int buttonIndex = 0;
      while (!(buttonMask & (1 << buttonIndex))) {
        buttonIndex++;
      }

      Serial.print("Taster ");
      Serial.print(buttonIndex + 1);
      Serial.println(" gedrückt (einzeln)");

      ledStatusButtonOK();

      // Solange dieser Taster gehalten wird, periodisch Status senden
      unsigned long pressStart = millis();
      unsigned long lastSend = 0;

      while ((readButtons() & (1 << buttonIndex)) &&
             ((millis() - pressStart) < BUTTON_HOLD_TIMEOUT)) {
        unsigned long now = millis();
        if (now - lastSend >= HOLD_SEND_INTERVAL) {
          sendButtonStatus(1 << buttonIndex);  // nur dieser Taster
          lastSend = now;
        }
        delay(10);
      }

      // Nach Loslassen (oder Timeout) optional ein "alles aus"-Paket senden
      sendButtonStatus(0);
    }

    lastButtonPress = millis();
  }
}

// =================== DEEP SLEEP-FUNKTIONEN ===================

/**
 * Geht in Deep Sleep, Taster als Wake-Quelle (ANY_LOW).
 */
void goToDeepSleep() {
  Serial.println("Gehe in Deep Sleep...");
  ledStatusInactivityTimeout();
  delay(1100);  // LED-Status sichtbar lassen

  Serial.flush();

  // Alle Taster als Wake-Quelle, LOW = gedrückt
  esp_sleep_enable_ext1_wakeup(buttonBitMask, ESP_EXT1_WAKEUP_ANY_LOW);

  // LEDs sicher aus
  setLEDColor(false, false, false);

  // Deep Sleep starten – ab hier läuft nichts mehr, bis Wake-Event
  esp_deep_sleep_start();
}

// =================== SETUP ===================

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n\n=== ESP32-Sender Markisensteuerung ===");

  // LEDs initialisieren
  initLEDs();

  // Aufwachgrund bestimmen
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT1: {
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
    }

    case ESP_SLEEP_WAKEUP_TIMER:
      Serial.println("Aufgewacht durch Timer");
      ledStatusAwake();
      break;

    default:
      Serial.println("Neustart (Power-On Reset)");
      ledStatusAwake();
      break;
  }

  // Taster (Eingänge mit internem Pull-Up)
  for (int i = 0; i < 6; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    Serial.printf("Taster %d an GPIO %d\n", i + 1, buttonPins[i]);
  }

  // Batterie initial prüfen
  checkBatteryStatus();

  // ESP-NOW initialisieren
  initESPNOW();

  // Inaktivitäts-Timer initialisieren
  lastButtonPress = millis();

  Serial.println("System bereit - warte auf Tastendruck...");
  Serial.printf("Inaktivitäts-Timeout: %d Sekunden\n", INACTIVITY_TIMEOUT);
  Serial.println("=====================================\n");

  delay(500);
}

// =================== LOOP ===================

void loop() {
  // Taster prüfen und ggf. ESP-NOW senden
  checkButtonsAndSend();

  // Batterie alle 60 s prüfen
  static unsigned long lastBatteryCheck = 0;
  if (millis() - lastBatteryCheck > 60000) {
    checkBatteryStatus();
    lastBatteryCheck = millis();
  }

  // Inaktivitäts-Timeout -> Deep Sleep
  if ((millis() - lastButtonPress) > (INACTIVITY_TIMEOUT * 1000UL)) {
    Serial.printf("Inaktivitäts-Timeout (%d Sekunden) erreicht\n", INACTIVITY_TIMEOUT);
    goToDeepSleep();
  }

  delay(50);  // kleine Entlastung für CPU
}
