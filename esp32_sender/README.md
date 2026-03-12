# ESP32-Sender für Markisensteuerung (LILYGO T-Energy-S3)

Batteriebetriebener Sender mit 6 Tastern, Status-LED und Deep-Sleep.  
Die RGB-LED zeigt **nur drei Dinge** an:

- Senden OK → LED kurz **grün**
- Senden NOK → LED kurz **rot**
- Batterie kritisch → LED **rot 2 s**, dann aus („bitte laden“)

---

## 1. Hardware-Beschreibung

### LILYGO T-Energy-S3
- ESP32-S3 mit 18650-Halter und Lade-IC
- Batterie-ADC: GPIO 3
- Taster: GPIO 1, 2, 4, 5, 6, 7
- RGB-LED: GPIO 10 (Rot), 11 (Grün), 12 (Blau)

### RGB-LED (gemeinsame Kathode)
- Kathode an GND
- Rot: GPIO 10 → 220 Ω → LED
- Grün: GPIO 11 → 220 Ω → LED
- Blau: GPIO 12 → 220 Ω → LED
- Logik: HIGH = LED an, LOW = aus

### Taster (6 Stück)
- einfache Taster (Schließer)
- interne Pull-Ups des ESP32 werden verwendet
- Verdrahtung:
  - Taster-Pin 1 → GPIO (1,2,4,5,6,7)
  - Taster-Pin 2 → GND
- alle Taster können ESP32 aus Deep Sleep aufwecken

### Stromversorgung
- 18650 Li-Ion im Board-Halter
- Firmware überwacht Akkuspannung über GPIO 3 (kalibriert)
- unter 3,3 V: Batterie „kritisch“, LED 2 s rot, dann keine weitere Nutzung

---

## 2. Schaltplan (Textuell)

### Taster
Für jeden Taster:
- Pin 1 → entsprechender GPIO
- Pin 2 → GND

### RGB-LED
- LED-Kathode → GND
- GPIO 10 → 220 Ω → Rot
- GPIO 11 → 220 Ω → Grün
- GPIO 12 → 220 Ω → Blau

### Übersichtstabelle

| Von      | Nach                  |
|---------|-----------------------|
| GPIO 1  | Taster 1, Pin 1       |
| GPIO 2  | Taster 2, Pin 1       |
| GPIO 4  | Taster 3, Pin 1       |
| GPIO 5  | Taster 4, Pin 1       |
| GPIO 6  | Taster 5, Pin 1       |
| GPIO 7  | Taster 6, Pin 1       |
| Taster 1–6, Pin 2 | GND         |
| GPIO 10 | 220 Ω → LED Rot       |
| GPIO 11 | 220 Ω → LED Grün      |
| GPIO 12 | 220 Ω → LED Blau      |
| LED Kathode | GND               |

---

## 3. Status-LED Logik

Es gibt nur drei bewusst sichtbare Zustände:

1. **Senden OK**
   - Wann: wenn ESP-NOW-Callback „Erfolg“ meldet
   - Anzeige: LED ca. 150 ms **grün**, dann aus
   - Bedeutung: Befehl beim Empfänger angekommen

2. **Senden NOK**
   - Wann: wenn ESP-NOW-Callback „Fehler“ meldet  
     oder `esp_now_send()` direkt einen Fehler liefert
   - Anzeige: LED ca. 150 ms **rot**, dann aus
   - Bedeutung: Befehl wahrscheinlich nicht angekommen  
     → ggf. neu senden / Empfänger prüfen

3. **Batterie kritisch**
   - Wann: Akkuspannung < `BATTERY_MIN_VOLTAGE` (Standard 3,3 V)
   - Anzeige: LED **rot 2 s**, dann aus
   - Bedeutung: Akku ist zu leer, bitte laden/wechseln.  
     In diesem Zustand wird kein normaler Betrieb mehr durchgeführt.

Dezente Zusatz-Anzeigen (nur kurz):
- beim Aufwachen / vor Deep Sleep: LED ca. 100 ms **cyan**, dann aus

Keine weiteren Farben/Pattern (kein „Taster OK“, kein „Senden blau“, etc.).

---

## 4. Installation & Inbetriebnahme

1. Projekt mit PlatformIO anlegen (`esp32_sender`).
2. `platformio.ini` für ESP32-S3 / T-Energy-S3 konfigurieren.
3. `src/main.cpp` mit obigem Code füllen.
4. MAC-Adresse des Empfänger-ESP32 ermitteln und in `receiverMac[]` eintragen.
5. Kompilieren und auf das LILYGO T-Energy-S3 flashen.

Nach dem Start:
- im Serial Monitor erscheinen ADC-Rohwerte und Batteriespannung
- nach 30 s Inaktivität geht der Sender in Deep Sleep
- ein Tasterdruck weckt den Sender wieder

---

## 5. Testanleitung

### 5.1. Senden OK / NOK

- Empfänger eingeschaltet, in Reichweite:
  - Taster drücken → Motor/Aktor reagiert
  - kurz nach dem Senden: LED **grün** (OK)
- Empfänger ausgeschaltet oder weit weg:
  - Taster drücken → keine Reaktion am Aktor
  - kurz nach dem Senden: LED **rot** (NOK)

### 5.2. Batterie kritisch

- Akku bewusst entladen (oder Spannung simulieren)
- Wenn `batteryVoltage < 3,3 V`:
  - beim nächsten Batterietest: LED **2 s rot**, danach aus
  - normaler Betrieb wird abgebrochen (kein dauerhaftes Senden mehr)
  - → Akku nachladen

---

## 6. Konfigurationsparameter

Im Code anpassbar:

- `INACTIVITY_TIMEOUT` – Zeit bis Deep Sleep (s)
- `DEBOUNCE_DELAY` – Entprellzeit (ms)
- `BUTTON_HOLD_TIMEOUT` – max. Haltezeit eines Tasters (ms)
- `HOLD_SEND_INTERVAL` – Sendeintervall während Halten (ms)
- `BATTERY_MIN_VOLTAGE` – Schwelle für „Batterie kritisch“
- `BATTERY_FULL_VOLTAGE` – nur für Logging/Skalierung
- `receiverMac[]` – MAC-Adresse des Empfängers

Empfehlung:
- `BATTERY_MIN_VOLTAGE` nicht unter 3,2 V setzen  
- `INACTIVITY_TIMEOUT` je nach Bedarf (30–60 s)

---

## 7. Checkliste für den Benutzer

- Drücke Taster:
  - Motor läuft → LED kurz grün → alles gut
  - Motor reagiert nicht → LED kurz rot → erneut versuchen / Empfänger prüfen
- Siehst du LED 2 s rot → Akku ist leer → bitte laden
- Sonst: LED bleibt die meiste Zeit **aus** (nicht irritieren lassen)
