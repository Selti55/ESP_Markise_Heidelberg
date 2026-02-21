/**
 * ESP32-Empfänger für Markisensteuerung
 * ESP32 WROOM mit ULN2803 für 6 Relais-Ausgänge
 * 
 * Funktion:
 * - Empfängt Taster-Signale per ESP-NOW vom Sender
 * - Schaltet 6 Ausgänge (GPIOs) entsprechend der Taster
 * - Ausgänge steuern ULN2803 für Relais (Links/Rechtslauf für 3 Motoren)
 * - Statusausgabe über seriellen Monitor
 */

#include <esp_now.h>
#include <WiFi.h>

// ============== KONFIGURATION ==============
// Timeout: Wenn länger als diese Zeit kein Paket empfangen wird,
// werden alle Ausgänge ausgeschaltet (Sicherheitsfunktion)
#define RECEIVE_TIMEOUT 2000  // 2 Sekunden

// ============== GPIO DEFINITIONEN ==============
// Ausgänge für ULN2803 (entsprechen Tastern 1-6)
const int outputPins[6] = {26, 27, 14, 12, 13, 15};

// Benennung der Ausgänge für Klartext-Ausgabe
const char* outputNames[6] = {
  "Motor 1 Linkslauf (Taster 1)",
  "Motor 1 Rechtslauf (Taster 2)",
  "Motor 2 Linkslauf (Taster 3)",
  "Motor 2 Rechtslauf (Taster 4)",
  "Motor 3 Linkslauf (Taster 5)",
  "Motor 3 Rechtslauf (Taster 6)"
};

// Motor-Zuordnung für Plausibilitätsprüfung
// Jeder Motor hat zwei Ausgänge: [Links, Rechts]
const int motorPairs[3][2] = {
  {0, 1},  // Motor 1: Ausgang 0 (Links), Ausgang 1 (Rechts)
  {2, 3},  // Motor 2: Ausgang 2 (Links), Ausgang 3 (Rechts)
  {4, 5}   // Motor 3: Ausgang 4 (Links), Ausgang 5 (Rechts)
};

// ============== ESP-NOW KONFIGURATION ==============
// MAC-Adresse des Senders (muss ausgelesen und hier eingetragen werden!)
// Format: {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}
uint8_t senderMac[] = {0x34, 0x85, 0x18, 0x78, 0x92, 0xAC}; // Beispiel-MAC - BITTE ÄNDERN!

// Nachrichtenstruktur (muss mit Sender übereinstimmen)
typedef struct struct_message {
  uint8_t buttonMask;      // Bit 0-5: Taster 1-6
  float batteryVoltage;    // Batteriespannung des Senders
  uint8_t sequence;        // Sequenznummer
  uint8_t rssi;            // Signalstärke (vom Sender gemessen)
} struct_message;

struct_message receivedData;

// ============== GLOBALE VARIABLEN ==============
unsigned long lastReceiveTime = 0;
uint8_t lastSequence = 0;
bool senderPaired = false;

// ============== AUSGANGS FUNKTIONEN ==============
/**
 * Initialisiert alle Ausgangs-Pins
 * Setzt sie auf LOW (Relais aus)
 */
void initOutputs() {
  for (int i = 0; i < 6; i++) {
    pinMode(outputPins[i], OUTPUT);
    digitalWrite(outputPins[i], LOW);
    Serial.printf("Ausgang %d (GPIO %d) initialisiert: %s\n", 
                  i+1, outputPins[i], outputNames[i]);
  }
}

/**
 * Setzt die Ausgänge basierend auf der empfangenen Button-Maske
 * @param buttonMask Bit 0-5: Taster 1-6 (1 = gedrückt)
 */
void setOutputsFromMask(uint8_t buttonMask) {
  // Ausgabe der empfangenen Maske
  Serial.print("Empfangene Taster-Maske: 0b");
  for (int i = 5; i >= 0; i--) {
    Serial.print((buttonMask >> i) & 1);
  }
  Serial.print(" (Hex: 0x");
  Serial.print(buttonMask, HEX);
  Serial.println(")");
  
  // Prüfen auf ungültige Kombinationen (beide Taster eines Motors gleichzeitig)
  bool invalidCombination = false;
  
  for (int motor = 0; motor < 3; motor++) {
    int leftPin = motorPairs[motor][0];
    int rightPin = motorPairs[motor][1];
    
    bool leftPressed = (buttonMask >> leftPin) & 1;
    bool rightPressed = (buttonMask >> rightPin) & 1;
    
    if (leftPressed && rightPressed) {
      Serial.printf("FEHLER: Motor %d würde gleichzeitig Links und Rechtslauf erhalten!\n", motor+1);
      Serial.println("Dieser Befehl wird ignoriert - keine Änderung für diesen Motor.");
      invalidCombination = true;
      
      // Für diesen Motor beide Ausgänge ausschalten (Sicherheit)
      digitalWrite(outputPins[leftPin], LOW);
      digitalWrite(outputPins[rightPin], LOW);
    }
  }
  
  // Normale Ausgangssetzung für gültige Kombinationen
  for (int i = 0; i < 6; i++) {
    bool pinState = (buttonMask >> i) & 1;
    
    // Prüfen ob dieser Pin Teil einer ungültigen Kombination war
    bool skip = false;
    for (int motor = 0; motor < 3; motor++) {
      int leftPin = motorPairs[motor][0];
      int rightPin = motorPairs[motor][1];
      
      if ((i == leftPin || i == rightPin) && 
          ((buttonMask >> leftPin) & 1) && ((buttonMask >> rightPin) & 1)) {
        skip = true;
        break;
      }
    }
    
    if (!skip) {
      digitalWrite(outputPins[i], pinState ? HIGH : LOW);
    }
    
    // Status ausgeben
    if (pinState && !skip) {
      Serial.printf("  → %s: EIN\n", outputNames[i]);
    } else if (!pinState && !skip) {
      Serial.printf("  → %s: AUS\n", outputNames[i]);
    }
  }
  
  if (invalidCombination) {
    Serial.println("WARNUNG: Ungültige Tasterkombination empfangen!");
  }
}

/**
 * Schaltet alle Ausgänge aus (Sicherheitsfunktion)
 */
void disableAllOutputs() {
  Serial.println("ALLE AUSGÄNGE AUSGESCHALTET (Timeout)");
  for (int i = 0; i < 6; i++) {
    digitalWrite(outputPins[i], LOW);
  }
}

// ============== ESP-NOW FUNKTIONEN ==============
/**
 * Callback wenn Daten empfangen wurden
 */
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  // Kopiere empfangene Daten in Struct
  memcpy(&receivedData, incomingData, sizeof(receivedData));
  
  // Prüfen ob der Absender bekannt ist
  bool knownSender = true;
  for (int i = 0; i < 6; i++) {
    if (mac[i] != senderMac[i]) {
      knownSender = false;
      break;
    }
  }
  
  if (!knownSender) {
    Serial.print("Unbekannter Absender: ");
    for (int i = 0; i < 6; i++) {
      Serial.printf("%02X", mac[i]);
      if (i < 5) Serial.print(":");
    }
    Serial.println(" - Paket ignoriert!");
    return;
  }
  
  // Zeitstempel aktualisieren
  lastReceiveTime = millis();
  
  // Paketinformationen ausgeben
  Serial.println("\n=== ESP-NOW Paket empfangen ===");
  Serial.printf("Bytes empfangen: %d\n", len);
  Serial.printf("Sequenznummer: %d\n", receivedData.sequence);
  Serial.printf("Batteriespannung Sender: %.2fV\n", receivedData.batteryVoltage);
  Serial.printf("Signalstärke (RSSI): %d dBm\n", receivedData.rssi);
  
  // Prüfen auf doppelte Pakete (gleiche Sequenznummer)
  if (receivedData.sequence == lastSequence) {
    Serial.println("WARNUNG: Doppeltes Paket empfangen (gleiche Sequenznummer)");
  }
  lastSequence = receivedData.sequence;
  
  // Ausgänge setzen
  setOutputsFromMask(receivedData.buttonMask);
  
  Serial.println("================================\n");
}

/**
 * Initialisiert ESP-NOW
 */
void initESPNOW() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  Serial.print("Eigene MAC-Adresse: ");
  Serial.println(WiFi.macAddress());
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init fehlgeschlagen!");
    return;
  }
  
  esp_now_register_recv_cb(OnDataRecv);
  
  Serial.println("ESP-NOW initialisiert - warte auf Sender...");
  Serial.print("Erwartete Sender-MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", senderMac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
}

// ============== SETUP ==============
void setup() {
  // Serielle Kommunikation starten
  Serial.begin(115200);
  delay(100);
  Serial.println("\n\n=====================================");
  Serial.println("ESP32-Empfänger für Markisensteuerung");
  Serial.println("=====================================");
  
  // Ausgänge initialisieren
  initOutputs();
  
  // ESP-NOW initialisieren
  initESPNOW();
  
  // Zeitstempel initialisieren
  lastReceiveTime = millis();
  
  Serial.println("System bereit - warte auf Sender...");
  Serial.println("=====================================\n");
}

// ============== LOOP ==============
void loop() {
  // Timeout-Überwachung: Wenn länger als RECEIVE_TIMEOUT kein Paket empfangen
  if (millis() - lastReceiveTime > RECEIVE_TIMEOUT) {
    // Nur einmal ausgeben beim Umschalten
    static bool timeoutActive = false;
    if (!timeoutActive) {
      Serial.printf("TIMEOUT: Kein Paket für %d ms empfangen!\n", RECEIVE_TIMEOUT);
      disableAllOutputs();
      timeoutActive = true;
    }
  } else {
    // Timeout zurücksetzen wenn Pakete empfangen werden
    // (keine Aktion nötig)
  }
  
  delay(10);  // Kleine Pause für CPU-Entlastung
}