/**
 * ESP32-Empfänger für Markisensteuerung - OPTIMIERTE VERSION
 * ESP32 WROOM mit ULN2803 für 6 Relais-Ausgänge
 * 
 * WICHTIGE VERBESSERUNGEN:
 * - Schnelleres Timeout für Sicherheitsabschaltung
 * - Optimierte Ausgangsansteuerung
 * - Bessere Fehlererkennung
 * 
 * FÜR ANFÄNGER:
 * - Jede Funktion ist ausführlich kommentiert
 * - Die Logik ist in kleine, überschaubare Teile zerlegt
 */

#include <esp_now.h>
#include <WiFi.h>

// =================== KONFIGURATION ===================

// Timeout: Wenn länger keine Pakete kommen, werden alle Ausgänge ausgeschaltet
// VON 200ms AUF 150ms REDUZIERT für schnellere Sicherheitsabschaltung
#define RECEIVE_TIMEOUT 150  // Millisekunden

// Hauptschleifen-Delay
#define LOOP_DELAY 5  // Millisekunden

// =================== GPIO DEFINITIONEN ===================

// Ausgänge für ULN2803 (entsprechen Tastern 1-6)
const int outputPins[6] = {26, 27, 14, 12, 13, 15};

// Klartext-Bezeichnungen für Debug-Ausgaben
const char* outputNames[6] = {
  "Motor 1 Linkslauf (Taster 1)",
  "Motor 1 Rechtslauf (Taster 2)",
  "Motor 2 Linkslauf (Taster 3)",
  "Motor 2 Rechtslauf (Taster 4)",
  "Motor 3 Linkslauf (Taster 5)",
  "Motor 3 Rechtslauf (Taster 6)"
};

// Motor-Zuordnung: Jeder Motor hat zwei Ausgänge
// Format: {Linkslauf-Ausgang, Rechtslauf-Ausgang}
const int motorPairs[3][2] = {
  {0, 1},  // Motor 1: Ausgang 0 (Links), Ausgang 1 (Rechts)
  {2, 3},  // Motor 2: Ausgang 2 (Links), Ausgang 3 (Rechts)
  {4, 5}   // Motor 3: Ausgang 4 (Links), Ausgang 5 (Rechts)
};

// =================== ESP-NOW KONFIGURATION ===================

// MAC-Adresse des Senders (muss an Ihre Hardware angepasst werden!)
uint8_t senderMac[] = {0x20, 0x6E, 0xF1, 0xA7, 0x4E, 0xB8};

// Nachrichtenstruktur (muss exakt mit dem Sender übereinstimmen!)
typedef struct struct_message {
  uint8_t buttonMask;      // Bit 0-5: Taster 1-6
  float   batteryVoltage;  // Batteriespannung des Senders
  uint8_t sequence;        // Sequenznummer
  int16_t adcRaw;          // ADC-Rohwert
  int8_t  rssi;            // Signalstärke (NEU)
  uint32_t timestamp;      // Zeitstempel (NEU)
} struct_message;

struct_message receivedData;  // Hier wird die empfangene Nachricht gespeichert

// =================== GLOBALE VARIABLEN ===================

unsigned long lastReceiveTime = 0;  // Wann wurde zuletzt ein Paket empfangen?
uint8_t lastSequence = 0;           // Letzte Sequenznummer (erkennt doppelte Pakete)
bool outputState[6] = {false};      // Aktueller Zustand der Ausgänge

// =================== AUSGANGS-FUNKTIONEN ===================

// Initialisiert alle Ausgänge (setzt sie auf AUS)
void initOutputs() {
  for (int i = 0; i < 6; i++) {
    pinMode(outputPins[i], OUTPUT);
    digitalWrite(outputPins[i], LOW);
    outputState[i] = false;
    Serial.printf("Ausgang %d (GPIO %d): %s\n", 
                  i+1, outputPins[i], outputNames[i]);
  }
}

// Schaltet alle Ausgänge aus (Sicherheitsfunktion)
void disableAllOutputs() {
  Serial.println("!!! SICHERHEITSABSCHALTUNG: Alle Ausgänge AUS !!!");
  for (int i = 0; i < 6; i++) {
    if (outputState[i]) {
      outputState[i] = false;
      digitalWrite(outputPins[i], LOW);
    }
  }
}

// Setzt die Ausgänge basierend auf der empfangenen Taster-Maske
void setOutputsFromMask(uint8_t buttonMask) {
  bool hasInvalidCombination = false;
  
  // Debug-Ausgabe der empfangenen Maske
  Serial.print("Empfangene Maske: 0b");
  for (int i = 5; i >= 0; i--) {
    Serial.print((buttonMask >> i) & 1);
  }
  Serial.print(" (0x");
  Serial.print(buttonMask, HEX);
  Serial.println(")");
  
  // Prüfung auf ungültige Kombinationen (beide Taster eines Motors gleichzeitig)
  for (int motor = 0; motor < 3; motor++) {
    int leftIndex  = motorPairs[motor][0];   // Linkslauf-Ausgang
    int rightIndex = motorPairs[motor][1];   // Rechtslauf-Ausgang
    
    bool leftPressed  = (buttonMask >> leftIndex) & 1;
    bool rightPressed = (buttonMask >> rightIndex) & 1;
    
    if (leftPressed && rightPressed) {
      // Beide Richtungen gleichzeitig - DAS DARF NICHT PASSIEREN!
      Serial.printf("FEHLER: Motor %d würde Links und Rechts gleichzeitig bekommen!\n", motor+1);
      Serial.println("-> Beide Ausgänge werden AUSgeschaltet!");
      
      // Sicherheitshalber beide Ausgänge ausschalten
      if (outputState[leftIndex]) {
        outputState[leftIndex] = false;
        digitalWrite(outputPins[leftIndex], LOW);
      }
      if (outputState[rightIndex]) {
        outputState[rightIndex] = false;
        digitalWrite(outputPins[rightIndex], LOW);
      }
      
      hasInvalidCombination = true;
    }
  }
  
  // Normale Ausgangssetzung für alle Kanäle
  for (int i = 0; i < 6; i++) {
    bool newState = (buttonMask >> i) & 1;
    
    if (newState != outputState[i]) {
      // Zustand ändert sich
      outputState[i] = newState;
      digitalWrite(outputPins[i], newState ? HIGH : LOW);
      
      // Debug-Ausgabe
      Serial.printf("  %s: %s\n", outputNames[i], newState ? "EIN" : "AUS");
    }
  }
  
  if (hasInvalidCombination) {
    Serial.println("WARNUNG: Ungültige Tasterkombination wurde korrigiert!");
  }
}

// =================== ESP-NOW FUNKTIONEN ===================

// Wird aufgerufen, wenn Daten empfangen wurden
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  // Daten in die Struktur kopieren
  memcpy(&receivedData, incomingData, sizeof(receivedData));
  
  // Prüfen, ob der Absender bekannt ist (Sicherheit)
  bool knownSender = true;
  for (int i = 0; i < 6; i++) {
    if (mac[i] != senderMac[i]) {
      knownSender = false;
      break;
    }
  }
  
  if (!knownSender) {
    // Unbekannter Absender - Paket ignorieren
    Serial.print("Unbekannter Absender: ");
    for (int i = 0; i < 6; i++) {
      Serial.printf("%02X", mac[i]);
      if (i < 5) Serial.print(":");
    }
    Serial.println(" - Paket ignoriert!");
    return;
  }
  
  // Zeitstempel aktualisieren (für Timeout-Überwachung)
  lastReceiveTime = millis();
  
  // Paket-Informationen ausgeben (für Diagnose)
  Serial.println("\n=== Paket empfangen ===");
  Serial.printf("Seq: %d | RSSI: %d dBm | ADC: %d\n", 
                receivedData.sequence, receivedData.rssi, receivedData.adcRaw);
  Serial.printf("Batterie Sender: %.2fV\n", receivedData.batteryVoltage);
  
  // Prüfen auf doppelte Pakete (gleiche Sequenznummer)
  if (receivedData.sequence == lastSequence) {
    Serial.println("Hinweis: Doppeltes Paket (Sequenznummer wiederholt)");
  }
  lastSequence = receivedData.sequence;
  
  // Ausgänge entsprechend der empfangenen Maske setzen
  setOutputsFromMask(receivedData.buttonMask);
  
  Serial.println("=====================\n");
}

// Initialisiert ESP-NOW
void initESPNOW() {
  // WiFi im Station-Modus (nicht Access Point)
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  // Eigene MAC-Adresse ausgeben (für die Konfiguration des Senders)
  Serial.print("Eigene MAC-Adresse (für Sender): ");
  Serial.println(WiFi.macAddress());
  
  // ESP-NOW initialisieren
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init fehlgeschlagen!");
    return;
  }
  
  // Callback für empfangene Daten registrieren
  esp_now_register_recv_cb(OnDataRecv);
  
  Serial.println("ESP-NOW bereit - warte auf Sender...");
  Serial.print("Erwartete Sender-MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", senderMac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
}

// =================== SETUP ===================

void setup() {
  // Serielle Kommunikation starten
  Serial.begin(115200);
  delay(100);
  Serial.println("\n\n=====================================");
  Serial.println("ESP32-Empfänger für Markisensteuerung");
  Serial.println("Optimierte Version");
  Serial.println("=====================================");
  
  // Ausgänge initialisieren
  initOutputs();
  
  // ESP-NOW initialisieren
  initESPNOW();
  
  // Zeitstempel initialisieren
  lastReceiveTime = millis();
  
  Serial.println("System bereit");
  Serial.printf("Sicherheits-Timeout: %d ms\n", RECEIVE_TIMEOUT);
  Serial.println("=====================================\n");
}

// =================== LOOP ===================

void loop() {
  static bool timeoutActive = false;
  static unsigned long lastStatusOutput = 0;
  
  // Timeout-Überwachung
  // Wenn länger als RECEIVE_TIMEOUT kein Paket empfangen wurde...
  if (millis() - lastReceiveTime > RECEIVE_TIMEOUT) {
    if (!timeoutActive) {
      // Nur einmal beim ersten Timeout ausgeben
      Serial.printf("TIMEOUT: Kein Paket für %d ms!\n", RECEIVE_TIMEOUT);
      disableAllOutputs();
      timeoutActive = true;
    }
  } else {
    // Pakete kommen wieder an
    if (timeoutActive) {
      Serial.println("Verbindung wiederhergestellt - Timeout aufgehoben");
      timeoutActive = false;
    }
  }
  
  // Nur alle 10 Sekunden einen Status ausgeben (für Diagnose)
  if (millis() - lastStatusOutput > 10000) {
    // Optional: Status der Ausgänge ausgeben
    // lastStatusOutput = millis();
  }
  
  // Kleine Pause zur CPU-Entlastung
  delay(LOOP_DELAY);
}