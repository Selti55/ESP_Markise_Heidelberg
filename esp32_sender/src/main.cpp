/**
 * ESP32-Sender für Markisensteuerung
 * LILYGO T-Energy-S3 mit 6 Tastern und RGB-LED
 *
 * LED-Logik:
 * - Batterie OK:
 *     - Taster erkannt: kurz GRÜN
 *     - Taster gehalten: GRÜN dauerhaft
 * - Batterie LOW:
 *     - Taster erkannt: kurz GRÜN
 *     - Taster gehalten: BLAU/ROT im Wechsel (blinken) als "bitte laden"
 *
 * Batterie-Überwachung:
 * - nur Info + batteryLow-Flag
 * - KEIN Einfluss auf ESP-NOW-Logik
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

// Batterie-Logik: Schwelle für "low" anhand ADC-Rohwert
#define BATTERY_LOW_RAW_THRESHOLD 1900  // vorläufiger Wert, anpassbar

// =================== GPIO DEFINITIONEN ===================

// Taster GPIOs (alle als Wake-Quelle, Pull-Up, gegen GND)
const int buttonPins[6] = {1, 2, 4, 5, 6, 7};
const uint64_t buttonBitMask =
    (1ULL << 1) | (1ULL << 2) | (1ULL << 4) |
    (1ULL << 5) | (1ULL << 6) | (1ULL << 7);

// RGB-LED GPIOs (gemeinsame Kathode: HIGH = EIN, LOW = AUS)
#define LED_RED_PIN   10/**
 * ESP32-Sender für Markisensteuerung - OPTIMIERTE VERSION
 * LILYGO T-Energy-S3 mit 6 Tastern und RGB-LED
 * 
 * WICHTIGE VERBESSERUNGEN:
 * - Schnellere Reaktionszeit durch nicht-blockierende Programmierung
 * - Keine delay() Aufrufe mehr in der Hauptschleife
 * - LED-Anzeige ohne Verzögerung
 * - Optimierte Taster-Entprellung
 * 
 * FÜR ANFÄNGER: 
 * - Jede Funktion ist ausführlich kommentiert
 * - Die Logik ist in kleine, überschaubare Teile zerlegt
 */

#include <esp_now.h>
#include <WiFi.h>
#include "esp_sleep.h"

// =================== KONFIGURATION ===================
// Diese Werte können nach Bedarf angepasst werden

// Inaktivitäts-Timeout: Nach 30 Sekunden ohne Tastendruck geht der ESP in den Tiefschlaf
#define INACTIVITY_TIMEOUT 30  // Sekunden

// Entprellzeit: Verhindert, dass ein Taster mehrfach auslöst
// VON 50ms AUF 10ms REDUZIERT für schnellere Reaktion
#define DEBOUNCE_DELAY 10  // Millisekunden

// Maximale Haltezeit: Sicherheit, falls Taster klemmt
#define BUTTON_HOLD_TIMEOUT 10000  // 10 Sekunden

// Sende-Intervall: Wie oft wird bei gehaltenem Taster gesendet?
// VON 50ms AUF 25ms REDUZIERT für flüssigeren Motorlauf
#define HOLD_SEND_INTERVAL 25  // Millisekunden

// Hauptschleifen-Delay: Kurze Pause zur Entlastung der CPU
// VON 50ms AUF 5ms REDUZIERT für schnellere Reaktion
#define LOOP_DELAY 5  // Millisekunden

// Batterie-Schwelle: ADC-Wert unter diesem Wert = Batterie schwach
#define BATTERY_LOW_RAW_THRESHOLD 1900

// =================== GPIO DEFINITIONEN ===================
// Hier werden die Pins für Taster und LED festgelegt

// Taster an GPIO 1,2,4,5,6,7 (alle mit internem Pull-Up-Widerstand)
const int buttonPins[6] = {1, 2, 4, 5, 6, 7};

// Bitmaske für die Wake-Quellen (welche Taster können den ESP wecken)
const uint64_t buttonBitMask =
    (1ULL << 1) | (1ULL << 2) | (1ULL << 4) |
    (1ULL << 5) | (1ULL << 6) | (1ULL << 7);

// RGB-LED Pins (gemeinsame Kathode: HIGH = LED an, LOW = aus)
#define LED_RED_PIN   10
#define LED_GREEN_PIN 11
#define LED_BLUE_PIN  12

// Batterie-Messung (ADC-Pin für Spannungsmessung)
#define BATTERY_ADC_PIN 3

// =================== ESP-NOW KONFIGURATION ===================
// MAC-Adresse des Empfängers (muss an Ihre Hardware angepasst werden!)
uint8_t receiverMac[] = {0xFC, 0xF5, 0xC4, 0x67, 0xA8, 0xE4};

// Diese Struktur wird per Funk übertragen
// Wichtig: MUSS exakt mit der Empfänger-Seite übereinstimmen!
typedef struct struct_message {
  uint8_t buttonMask;      // Bit 0-5: Welche Taster sind gedrückt?
  float   batteryVoltage;  // Batteriespannung (nur zur Information)
  uint8_t sequence;        // Sequenznummer (erkennt doppelte Pakete)
  int16_t adcRaw;          // Rohwert vom ADC (für Diagnose)
  int8_t  rssi;            // Signalstärke (NEU: für Diagnose)
  uint32_t timestamp;      // Zeitstempel (NEU: für Laufzeitanalyse)
} struct_message;

struct_message myData;  // Hier wird die zu sendende Nachricht gespeichert

// =================== GLOBALE VARIABLEN ===================
// Diese Variablen sind im ganzen Programm sichtbar

unsigned long lastButtonPressTime = 0;   // Wann wurde zuletzt ein Taster gedrückt?
float batteryVoltage = 0.0;              // Aktuelle Batteriespannung
uint8_t sequenceNumber = 0;              // Zähler für gesendete Pakete
bool batteryLow = false;                 // TRUE = Batterie ist schwach

// =================== LED-CONTROLLER KLASSE ===================
// Diese Klasse kümmert sich um die LED-Anzeige, OHNE die Programmausführung zu blockieren
// Das ist wichtig für schnelle Reaktionszeiten!

class LEDController {
private:
  // Private Variablen (nur innerhalb dieser Klasse sichtbar)
  unsigned long lastChangeTime = 0;  // Wann wurde die LED zuletzt geändert?
  int currentMode = 0;               // 0=aus, 1=kurzer Blink, 2=dauerhaft, 3=blinken
  unsigned long modeStartTime = 0;   // Wann wurde der aktuelle Modus gestartet?
  bool blinkState = false;           // Aktueller Zustand beim Blinken (an/aus)
  
  // Interne Funktion zum Setzen der LED-Farben
  void setLEDColor(bool red, bool green, bool blue) {
    digitalWrite(LED_RED_PIN,   red   ? HIGH : LOW);
    digitalWrite(LED_GREEN_PIN, green ? HIGH : LOW);
    digitalWrite(LED_BLUE_PIN,  blue  ? HIGH : LOW);
  }
  
public:
  // Öffentliche Funktionen (können von außen aufgerufen werden)
  
  // Setzt den LED-Modus
  // mode: 0=AUS, 1=kurzer Grün-Blink, 2=Grün dauerhaft, 3=Blau/Rot blinken
  void setMode(int mode) {
    currentMode = mode;
    modeStartTime = millis();  // Startzeit merken
    blinkState = false;        // Blinkzustand zurücksetzen
  }
  
  // Diese Funktion MUSS regelmäßig aufgerufen werden (z.B. in der loop)
  // Sie aktualisiert die LED, ohne die Programmausführung zu blockieren
  void update() {
    unsigned long now = millis();  // Aktuelle Zeit in Millisekunden
    
    switch(currentMode) {
      case 0: // Modus: AUS
        setLEDColor(false, false, false);
        break;
        
      case 1: // Modus: Kurzer Grün-Blink (80ms)
        if (now - modeStartTime < 80) {
          setLEDColor(false, true, false);  // Grün an
        } else {
          currentMode = 0;  // Nach 80ms automatisch ausschalten
        }
        break;
        
      case 2: // Modus: Grün dauerhaft
        setLEDColor(false, true, false);
        break;
        
      case 3: // Modus: Blinken (Blau/Rot) für schwache Batterie
        // Alle 250ms die Farbe wechseln
        if (now - lastChangeTime >= 250) {
          lastChangeTime = now;
          blinkState = !blinkState;  // Zustand umschalten
          if (blinkState) {
            setLEDColor(false, false, true);  // Blau
          } else {
            setLEDColor(true, false, false);  // Rot
          }
        }
        break;
    }
  }
};

// =================== TASTER-CONTROLLER KLASSE ===================
// Diese Klasse liest die Taster aus, OHNE die Programmausführung zu blockieren
// Verwendet ein "State Machine" Prinzip (Zustandsautomat)

class ButtonReader {
private:
  // Zustandsvariablen für jeden Taster
  bool lastState[6] = {false};      // Letzter gemessener Zustand
  bool stableState[6] = {false};    // Entprellter, stabiler Zustand
  unsigned long debounceTime[6] = {0};  // Wann wurde der Zustand zuletzt geändert?
  
public:
  // Liest alle Taster aus und gibt eine Bitmaske zurück
  // Bit 0 = Taster 1, Bit 1 = Taster 2, usw.
  uint8_t readButtons() {
    uint8_t mask = 0;
    unsigned long now = millis();
    
    // Alle 6 Taster einzeln prüfen
    for (int i = 0; i < 6; i++) {
      bool currentState = (digitalRead(buttonPins[i]) == LOW);
      
      // Wurde der Zustand geändert?
      if (currentState != lastState[i]) {
        // Ja -> Entprell-Zeit zurücksetzen
        debounceTime[i] = now;
      }
      
      // Ist die Entprell-Zeit abgelaufen?
      if ((now - debounceTime[i]) > DEBOUNCE_DELAY) {
        // Ja -> Zustand ist stabil
        if (currentState != stableState[i]) {
          // Zustand hat sich wirklich geändert
          stableState[i] = currentState;
          if (currentState) {
            // Taster wurde gedrückt -> Bit setzen
            mask |= (1 << i);
          }
        }
      }
      
      // Aktuellen Zustand für nächsten Durchlauf merken
      lastState[i] = currentState;
    }
    
    return mask;
  }
  
  // Prüft, ob genau ein Taster gedrückt ist
  bool isSingleButton(uint8_t mask) {
    return (mask != 0 && (mask & (mask - 1)) == 0);
  }
  
  // Gibt den Index des gedrückten Tasters zurück (0-5)
  int getButtonIndex(uint8_t mask) {
    for (int i = 0; i < 6; i++) {
      if (mask & (1 << i)) return i;
    }
    return -1;  // Kein Taster gedrückt
  }
};

// =================== BATTERIE-FUNKTIONEN ===================

// Rechnet ADC-Rohwert in Spannung um
float getBatteryVoltageFromRaw(int raw) {
  const float K = 0.00172f;  // Kalibrierungsfaktor (muss an Ihre Hardware angepasst werden)
  return raw * K;
}

// Misst die Batteriespannung und setzt das batteryLow-Flag
void logBatteryStatus() {
  // ADC-Pin als Eingang mit Pulldown konfigurieren
  pinMode(BATTERY_ADC_PIN, INPUT_PULLDOWN);
  delay(10);  // Kurze Wartezeit für Stabilisierung
  
  int raw = analogRead(BATTERY_ADC_PIN);
  batteryVoltage = getBatteryVoltageFromRaw(raw);
  
  // Prüfen, ob Batterie schwach ist
  batteryLow = (raw < BATTERY_LOW_RAW_THRESHOLD);
  
  // Debug-Ausgabe (nur für Entwicklung)
  Serial.print("ADC: ");
  Serial.print(raw);
  Serial.print(" | Spannung: ");
  Serial.print(batteryVoltage, 2);
  Serial.print("V | Status: ");
  Serial.println(batteryLow ? "LOW" : "OK");
}

// =================== ESP-NOW FUNKTIONEN ===================

// Wird aufgerufen, wenn eine Nachricht gesendet wurde
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    // Erfolgreich gesendet - keine weitere Aktion nötig
    // Die LED wird nicht mehr hier gesteuert, das macht jetzt der LEDController
  } else {
    // Fehler beim Senden
    Serial.println("Sendefehler!");
  }
}

// Initialisiert ESP-NOW
void initESPNOW() {
  // WiFi im Station-Modus (nicht Access Point)
  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);  // Maximale Sendeleistung
  WiFi.setSleep(false);                  // Kein Power-Save (für schnellere Reaktion)
  WiFi.disconnect();
  
  // ESP-NOW initialisieren
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init fehlgeschlagen!");
    return;
  }
  
  // Callback für Sendestatus registrieren
  esp_now_register_send_cb(OnDataSent);
  
  // Empfänger als Peer hinzufügen
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, receiverMac, 6);
  peerInfo.channel = 0;      // Gleichen Kanal wie WiFi nutzen
  peerInfo.encrypt = false;   // Keine Verschlüsselung (für Geschwindigkeit)
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Peer hinzufügen fehlgeschlagen!");
    return;
  }
  
  Serial.println("ESP-NOW bereit");
}

// Sendet den Tasterstatus per ESP-NOW
void sendButtonStatus(uint8_t buttonMask) {
  // Nachricht zusammenstellen
  myData.buttonMask = buttonMask;
  myData.batteryVoltage = batteryVoltage;
  myData.adcRaw = analogRead(BATTERY_ADC_PIN);
  myData.sequence = sequenceNumber++;
  myData.rssi = WiFi.RSSI();           // Signalstärke für Diagnose
  myData.timestamp = millis();          // Zeitstempel für Laufzeitanalyse
  
  // Senden
  esp_err_t result = esp_now_send(receiverMac, (uint8_t*)&myData, sizeof(myData));
  
  if (result != ESP_OK) {
    Serial.println("Senden fehlgeschlagen!");
  }
}

// =================== DEEP SLEEP FUNKTIONEN ===================

// Versetzt den ESP in den Tiefschlaf
void goToDeepSleep() {
  Serial.println("Gehe in Tiefschlaf...");
  delay(100);  // Kurze Wartezeit für letzte Serial-Ausgaben
  Serial.flush();
  
  // Taster als Aufweck-Quelle konfigurieren
  esp_sleep_enable_ext1_wakeup(buttonBitMask, ESP_EXT1_WAKEUP_ANY_LOW);
  
  // LED ausschalten
  digitalWrite(LED_RED_PIN, LOW);
  digitalWrite(LED_GREEN_PIN, LOW);
  digitalWrite(LED_BLUE_PIN, LOW);
  
  // In Tiefschlaf gehen
  esp_deep_sleep_start();
}

// =================== SETUP ===================
// Wird einmal beim Start ausgeführt

void setup() {
  // Serielle Kommunikation für Debug-Ausgaben starten
  Serial.begin(115200);
  delay(100);
  Serial.println("\n\n=== Markisensteuerung Sender (Optimiert) ===");
  
  // LED-Pins als Ausgänge konfigurieren
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);
  
  // Taster-Pins als Eingänge mit Pull-Up konfigurieren
  for (int i = 0; i < 6; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    Serial.printf("Taster %d an GPIO %d\n", i + 1, buttonPins[i]);
  }
  
  // Batterie messen
  logBatteryStatus();
  
  // ESP-NOW initialisieren
  initESPNOW();
  
  // Startzeit für Inaktivitäts-Timeout setzen
  lastButtonPressTime = millis();
  
  Serial.println("Bereit - warte auf Tastendruck...");
  Serial.println("=====================================\n");
}

// =================== LOOP ===================
// Wird immer wieder ausgeführt (Hauptprogramm)

void loop() {
  // Statische Variablen (behalten ihren Wert zwischen den Aufrufen)
  static ButtonReader buttons;           // Taster-Logik
  static LEDController led;              // LED-Logik
  static unsigned long lastSendTime = 0; // Letzte Sendung
  static uint8_t currentMask = 0;        // Aktuell gedrückte Taster
  static unsigned long holdStartTime = 0;// Wann wurde der Taster gedrückt?
  static unsigned long lastBatteryCheck = 0;
  
  unsigned long now = millis();  // Aktuelle Zeit
  
  // 1. LED aktualisieren (nicht-blockierend!)
  led.update();
  
  // 2. Taster einlesen (nicht-blockierend!)
  uint8_t newMask = buttons.readButtons();
  
  // 3. Batterie regelmäßig prüfen (alle 60 Sekunden)
  if (now - lastBatteryCheck > 60000) {
    logBatteryStatus();
    lastBatteryCheck = now;
  }
  
  // 4. Taster-Logik
  if (newMask != 0) {
    // Ein oder mehrere Taster wurden gedrückt
    
    if (buttons.isSingleButton(newMask)) {
      // Genau EIN Taster wurde gedrückt
      
      if (newMask != currentMask) {
        // Neuer Taster oder Zustandsänderung
        currentMask = newMask;
        holdStartTime = now;
        lastButtonPressTime = now;
        
        // Kurze Rückmeldung: LED kurz grün blinken lassen
        led.setMode(1);
        
        // Sofort senden (für sofortige Reaktion)
        sendButtonStatus(currentMask);
        lastSendTime = now;
        
        // LED-Modus basierend auf Batteriestatus setzen
        if (batteryLow) {
          led.setMode(3);  // Blinken bei schwacher Batterie
        } else {
          led.setMode(2);  // Dauer-Grün bei OK
        }
      } else {
        // Gleicher Taster wird weiterhin gehalten
        // Periodisch senden (für flüssigen Motorlauf)
        if (now - lastSendTime >= HOLD_SEND_INTERVAL) {
          sendButtonStatus(currentMask);
          lastSendTime = now;
        }
        
        // Timeout-Prüfung (Sicherheit, falls Taster klemmt)
        if (now - holdStartTime > BUTTON_HOLD_TIMEOUT) {
          Serial.println("Sicherheits-Timeout: Taster zu lange gedrückt!");
          currentMask = 0;
          sendButtonStatus(0);
          led.setMode(0);
        }
      }
    } else {
      // Mehrere Taster gleichzeitig gedrückt -> ignorieren
      if (currentMask != 0) {
        Serial.println("Mehrere Taster gedrückt - Befehl ignoriert!");
        currentMask = 0;
        sendButtonStatus(0);
        led.setMode(0);
      }
    }
  } else {
    // KEIN Taster gedrückt
    if (currentMask != 0) {
      // Taster wurde losgelassen -> Stop-Signal senden
      Serial.println("Taster losgelassen - Stop");
      sendButtonStatus(0);
      currentMask = 0;
      led.setMode(0);
      lastButtonPressTime = now;  // Zeit für Inaktivitäts-Timeout zurücksetzen
    }
  }
  
  // 5. Inaktivitäts-Timeout prüfen
  if (currentMask == 0 && (now - lastButtonPressTime) > (INACTIVITY_TIMEOUT * 1000UL)) {
    Serial.println("Inaktivitäts-Timeout - Gehe in Tiefschlaf");
    goToDeepSleep();
  }
  
  // 6. Kleine Pause zur CPU-Entlastung
  // Wichtig: Nur 5ms statt 50ms für bessere Reaktionszeit!
  delay(LOOP_DELAY);
}
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

// Akku-Status
bool batteryLow = false;   // wird aus ADC-Rohwert abgeleitet

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
}

// Mehrfach-Taster Fehler – kurze rote Anzeige
void ledStatusMultiButtonError() {
  setLEDColor(true, false, false);  // Rot
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
 * Rechnet ADC-Rohwert in Spannung um.
 */
float getBatteryVoltageFromRaw(int raw) {
  const float K = 0.00172f;  // Volt pro ADC-Step (wird später neu kalibriert)
  return raw * K;
}

/**
 * Loggt Spannung und setzt batteryLow-Flag anhand des ADC-Rohwerts.
 * Nur EINE Messung, die für alles verwendet wird.
 */
void logBatteryStatus() {
  pinMode(BATTERY_ADC_PIN, INPUT_PULLDOWN);
  delay(10);

  int raw = analogRead(BATTERY_ADC_PIN);

  Serial.print("ADC raw: ");
  Serial.println(raw);

  batteryVoltage = getBatteryVoltageFromRaw(raw);

  Serial.print("Batteriespannung (nur Info): ");
  Serial.print(batteryVoltage, 2);
  Serial.println(" V");

  // Schwelle: ADC-Rohwert < BATTERY_LOW_RAW_THRESHOLD => "low"
  batteryLow = (raw < BATTERY_LOW_RAW_THRESHOLD);

  Serial.print("batteryLow = ");
  Serial.println(batteryLow ? "true" : "false");
}

// =================== ESP-NOW-FUNKTIONEN ===================

/**
 * Callback nach Sendeversuch (ESP-NOW).
 * - Erfolg  -> LED aus
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
 * LED-Feedback während des Haltens:
 * - Akku OK: grün dauerhaft
 * - Akku LOW: blau/rot blinkend
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

      // kurzer grüner Blink als „Taster erkannt“
      ledStatusButtonOK();
      delay(80);
      setLEDColor(false, false, false);

      // Einmal senden (auch bei kurzem Tastendruck)
      sendButtonStatus(1 << buttonIndex);

      unsigned long pressStart = millis();
      unsigned long lastSend   = millis();
      unsigned long lastBlink  = 0;
      bool blinkState = false;

      // Solange der Taster gehalten wird, weiter senden (Motor EIN)
      while ((readButtons() & (1 << buttonIndex)) &&
             ((millis() - pressStart) < BUTTON_HOLD_TIMEOUT)) {

        unsigned long now = millis();

        // 1) Senden in gewohnter Rate
        if (now - lastSend >= HOLD_SEND_INTERVAL) {
          sendButtonStatus(1 << buttonIndex);
          lastSend = now;
        }

        // 2) LED-Anzeige während des Haltens
        if (batteryLow) {
          // Akku LOW: blau/rot im Wechsel blinken
          if (now - lastBlink >= 250) {
            lastBlink = now;
            blinkState = !blinkState;
            if (blinkState) {
              setLEDColor(false, false, true);  // Blau
            } else {
              setLEDColor(true, false, false);  // Rot
            }
          }
        } else {
          // Akku OK: LED einfach dauerhaft grün
          setLEDColor(false, true, false);
        }

        delay(10);
      }

      // Nach Loslassen (oder Timeout) -> LED aus, "alles aus"-Paket senden
      setLEDColor(false, false, false);
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
