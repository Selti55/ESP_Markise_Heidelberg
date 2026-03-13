/**
 * ESP32-Sender für Markisensteuerung
 * LILYGO T-Energy-S3 mit 6 Tastern und RGB-LED
 *
 * Stand:
 * - Taster + ESP-NOW Senden wie im funktionierenden Code
 * - RGB-LED:
 *      - Taster OK: kurz GRÜN
 *      - Akku niedrig + Taster gehalten: blau-rot-blau pulsierend
 *      - sonst: keine Sende-LED
 * - Batterie-Überwachung:
 *      - nur Info im Serial Monitor
 *      - setzt ein batteryLow-Flag für die LED-Anzeige, beeinflusst keine Logik
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

// Wie oft bei gehaltenem Taster gesendet wird (Motor EIN solange Pakete kommen)
#define HOLD_SEND_INTERVAL 50  // ms

// =================== GPIO DEFINITIONEN ===================

// Taster GPIOs (alle als Wake-Quelle, Pull-Up, gegen GND)
const int buttonPins[6] = {1, 2, 4, 5, 6, 7};
const uint64_t buttonBitMask =
    (1ULL << 1) | (1ULL << 2) | (1ULL << 4) |
    (1ULL << 5) | (1ULL << 6) | (1ULL << 7);

// RGB-LED GPIOs (gemeinsame Kathode: HIGH = EIN, LOW = AUS)
#define LED_RED_PIN   10
#define LED_GREEN_PIN 11
#define LED_BLUE_PIN  12

// Batterie-Messung (ADC-Pin, an VBAT-Teiler des T-Energy S3)
#define BATTERY_ADC_PIN 3

// =================== ESP-NOW KONFIGURATION ===================

// MAC-Adresse des Empfängers (ESP32 WROOM)
uint8_t receiverMac[] = {0xFC, 0xF5, 0xC4, 0x67, 0xA8, 0xE4};

// Nachrichtenstruktur – muss exakt zur Empfängerseite passen
typedef struct struct_message {
  uint8_t buttonMask;      // Bit 0-5: Taster 1-6
  float   batteryVoltage;  // Batteriespannung in Volt (nur Info)
  uint8_t sequence;        // Sequenznummer
  int16_t adcRaw;          // Rohwert
} struct_message;

struct_message myData;

// =================== GLOBALE VARIABLEN ===================

unsigned long lastButtonPress = 0;  // Zeitpunkt des letzten erkkannten Tastendrucks
bool   awakeFromSleep = false;
float  batteryVoltage = 0.0;
uint8_t sequenceNumber = 0;

// Akku-/Sende-Status
bool batteryLow    = false;   // wird aus ADC-Rohwert abgeleitet
bool sendingActive = false;   // true, solange ButtonMask != 0 gesendet wird

// =================== LED-FUNKTIONEN ===================

void initLEDs() {
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);

  // Alle LEDs aus (gemeinsame Kathode -> LOW = AUS)
  digitalWrite(LED_RED_PIN, LOW);
  digitalWrite(LED_GREEN_PIN, LOW);
  digitalWrite(LED_BLUE_PIN, LOW);
}

/**
 * Setzt die RGB-LED Farbe
 * Common Cathode: HIGH = LED an, LOW = aus.
 */
void setLEDColor(bool red, bool green, bool blue) {
  digitalWrite(LED_RED_PIN,   red   ? HIGH : LOW);
  digitalWrite(LED_GREEN_PIN, green ? HIGH : LOW);
  digitalWrite(LED_BLUE_PIN,  blue  ? HIGH : LOW);
}

// Aufwachen (einmaliger kurzer Cyan-Puls – mit delay, aber nur im Setup)
void ledStatusAwake() {
  setLEDColor(false, true, true);  // Cyan
  delay(100);
  setLEDColor(false, false, false);
}

// Taster OK – kurzer Grün-Blink
void ledStatusButtonOK() {
  setLEDColor(false, true, false);  // Grün
  // kein delay hier; wird von weiterer Logik bzw. Puls-Update überschrieben
}

// Mehrfach-Taster Fehler – kurze rote Anzeige
void ledStatusMultiButtonError() {
  setLEDColor(true, false, false);  // Rot
  // kein delay hier
}

// Sendebestätigung – wieder aus
void ledStatusSendSuccess() {
  setLEDColor(false, false, false);
}

// Sendefehler – kurze rote Anzeige
void ledStatusSendError() {
  setLEDColor(true, false, false);
  delay(150);
  setLEDColor(false, false, false);
}

// Inaktiv-Timeout (Cyan 1 s) – nur beim Übergang in Sleep
void ledStatusInactivityTimeout() {
  setLEDColor(false, true, true);
  delay(1000);
  setLEDColor(false, false, false);
}

// =================== BATTERIE-FUNKTIONEN ===================

/**
 * Misst die Batteriespannung und gibt sie nur im Serial Monitor aus.
 * KEINE direkte LED- oder Logik-Reaktion.
 */
float getBatteryVoltage() {
  pinMode(BATTERY_ADC_PIN, INPUT_PULLDOWN);
  delay(10);
  int raw = analogRead(BATTERY_ADC_PIN);

  // Rohwert ausgeben, damit wir später sauber kalibrieren können.
  Serial.print("ADC raw: ");
  Serial.println(raw);

  // Vorläufige Umrechnung (kann durch Lade-IC verfälscht sein)
  const float K = 0.00172f;  // Volt pro ADC-Step (wird später neu kalibriert)
  float voltage = raw * K;
  return voltage;
}

/**
 * Loggt Spannung und setzt ein einfaches batteryLow-Flag anhand des ADC-Rohwerts.
 * Schwelle ist bewusst grob, da Kalibrierung später sauber gemacht wird.
 */
void logBatteryStatus() {
  batteryVoltage = getBatteryVoltage();

  Serial.print("Batteriespannung (nur Info): ");
  Serial.print(batteryVoltage, 2);
  Serial.println(" V");

  int raw = analogRead(BATTERY_ADC_PIN);

  // Grobe Schwelle: bei deinen Messungen lagen "komische" Werte um 800-1100.
  // Hier nehmen wir >1500 als "ok", darunter "low".
  batteryLow = (raw < 1500);
  Serial.print("batteryLow = ");
  Serial.println(batteryLow ? "true" : "false");
}

// =================== ESP-NOW-FUNKTIONEN ===================

/**
 * Callback nach Sendeversuch (ESP-NOW).
 * - Erfolg  -> LED aus (Puls-Logik übernimmt ggf.)
 * - Fehler  -> Rot (kurz)
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

  myData.buttonMask     = buttonMask;
  myData.batteryVoltage = batteryVoltage;          // nur Info
  myData.adcRaw         = analogRead(BATTERY_ADC_PIN);
  myData.sequence       = sequenceNumber++;

  // merken, ob gerade gehalten gesendet wird
  sendingActive = (buttonMask != 0);

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
    if (digitalRead(buttonPins[i]) == LOW) {
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
 * (Originale, bewährte Logik)
 */
void checkButtonsAndSend() {
  uint8_t buttonMask = readButtons();

  if (buttonMask != 0) {
    // Mehrere Taster – keine Aktion (Fehler ist schon angezeigt)
    if ((buttonMask & (buttonMask - 1)) != 0) {
      Serial.println("Mehrere Taster - keine Aktion");
    } else {
      int buttonIndex = 0;
      while (!(buttonMask & (1 << buttonIndex))) {
        buttonIndex++;
      }

      Serial.print("Taster ");
      Serial.print(buttonIndex + 1);
      Serial.println(" gedrückt (einzeln)");

      ledStatusButtonOK();

      unsigned long pressStart = millis();
      unsigned long lastSend   = 0;

      while ((readButtons() & (1 << buttonIndex)) &&
             ((millis() - pressStart) < BUTTON_HOLD_TIMEOUT)) {
        unsigned long now = millis();
        if (now - lastSend >= HOLD_SEND_INTERVAL) {
          sendButtonStatus(1 << buttonIndex);
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

// =================== LED-PULS BEI AKKU NIEDRIG ===================

/**
 * Wird aus loop() aufgerufen.
 * Wenn Akku niedrig UND Taster gehalten (sendingActive),
 * lässt die Status-LED blau-rot-blau pulsieren.
 */
void updateLowBatterySendPulse() {
  if (!sendingActive || !batteryLow) {
    // Wenn kein Senden aktiv oder Akku ok -> nichts tun
    return;
  }

  static unsigned long lastChange = 0;
  static uint8_t phase = 0;

  unsigned long now = millis();
  const unsigned long step = 200;  // ms pro Phase

  if (now - lastChange >= step) {
    lastChange = now;
    phase = (phase + 1) % 3;

    switch (phase) {
      case 0:
        // Blau
        setLEDColor(false, false, true);
        break;
      case 1:
        // Rot
        setLEDColor(true, false, false);
        break;
      case 2:
        // Blau
        setLEDColor(false, false, true);
        break;
    }
  }
}

// =================== DEEP SLEEP-FUNKTIONEN ===================

/**
 * Geht in Deep Sleep, Taster als Wake-Quelle (ANY_LOW).
 */
void goToDeepSleep() {
  Serial.println("Gehe in Deep Sleep...");
  ledStatusInactivityTimeout();
  delay(1100);

  Serial.flush();

  esp_sleep_enable_ext1_wakeup(buttonBitMask, ESP_EXT1_WAKEUP_ANY_LOW);

  setLEDColor(false, false, false);
  esp_deep_sleep_start();
}

// =================== SETUP ===================

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n\n=== ESP32-Sender Markisensteuerung ===");

  initLEDs();

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT1: {
      Serial.println("Aufgewacht durch Tastendruck");
      awakeFromSleep = true;
      ledStatusAwake();

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

  for (int i = 0; i < 6; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    Serial.printf("Taster %d an GPIO %d\n", i + 1, buttonPins[i]);
  }

  // Batterie nur zum Start loggen (setzt auch batteryLow-Flag)
  logBatteryStatus();

  initESPNOW();

  lastButtonPress = millis();

  Serial.println("System bereit - warte auf Tastendruck...");
  Serial.printf("Inaktivitäts-Timeout: %d Sekunden\n", INACTIVITY_TIMEOUT);
  Serial.println("=====================================\n");

  delay(500);
}

// =================== LOOP ===================

void loop() {
  // Wenn Akku niedrig und Taster gehalten, LED blau-rot-blau pulsieren lassen
  updateLowBatterySendPulse();

  checkButtonsAndSend();

  static unsigned long lastBatteryCheck = 0;
  if (millis() - lastBatteryCheck > 60000) {
    // Batterie periodisch loggen (nur Info, setzt batteryLow)
    logBatteryStatus();
    lastBatteryCheck = millis();
  }

  if ((millis() - lastButtonPress) > (INACTIVITY_TIMEOUT * 1000UL)) {
    Serial.printf("Inaktivitäts-Timeout (%d Sekunden) erreicht\n", INACTIVITY_TIMEOUT);
    goToDeepSleep();
  }

  delay(50);
}
