# ESP32-Sender für Markisensteuerung (LILYGO T-Energy-S3)

Batteriebetriebener Sender mit 6 Tastern, Status-LED und intelligentem Energiemanagement. Die Taster wecken den ESP32 aus dem Deep Sleep, nach einstellbarer Inaktivität geht er automatisch wieder schlafen. Eine RGB-LED zeigt Statusmeldungen durch verschiedene Blinksequenzen.

---

## 📋 Inhaltsverzeichnis
1. [Hardware-Beschreibung](#1-hardware-beschreibung)
2. [Schaltplan (Textuell)](#2-schaltplan-textuell)
3. [Status-LED Blinksequenzen](#3-status-led-blinksequenzen)
4. [Installation & Inbetriebnahme](#4-installation--inbetriebnahme)
5. [Testanleitung](#5-testanleitung)
6. [Fehlersuche](#6-fehlersuche)
7. [Konfigurationsmöglichkeiten](#7-konfigurationsmöglichkeiten)
8. [Batterielaufzeit](#8-batterielaufzeit)

---

## 1. Hardware-Beschreibung

### Hauptkomponente: LILYGO T-Energy-S3
Das LILYGO T-Energy-S3 ist ein ESP32-S3 Entwicklungsboard mit integriertem 18650 Batteriehalter und Ladeschaltung. Es verfügt über einen Dual-Core Prozessor mit 240 MHz, einen integrierten 18650 Halter auf der Rückseite, eine USB-C Ladeschaltung, Verpolungsschutz, Überladung und Tiefentladung. Der Battery-ADC ist an GPIO3 angeschlossen, USB-Detect an GPIO15. Der Deep Sleep Stromverbrauch liegt bei extrem geringen 5-20 µA. Das Board bietet zusätzlich einen Qwiic/STEMMA Anschluss und M2.5 Montagelöcher.

### Status-LED (RGB mit gemeinsamer Anode)
Für die Statusanzeige wird eine RGB-LED mit gemeinsamer Anode verwendet, zum Beispiel eine 5mm RGB LED mit 4 Pins. Die Ansteuerung erfolgt über drei GPIOs mit Vorwiderständen: Rot an GPIO 10 mit 220Ω, Grün an GPIO 11 mit 220Ω, Blau an GPIO 12 mit 220Ω. Die gemeinsame Anode wird an 3.3V angeschlossen. Die Farben haben folgende Funktionen: Rot für Fehler und Batterie kritisch, Grün für Erfolg, Taster OK und Batterie voll, Blau für Senden und Aufwachen.

### Taster (6 Stück)
Es werden sechs Taster als Schließer (Momentkontakt) verwendet, zum Beispiel 6x6x5mm Tact Switches. Jeder Taster wird mit einem externen 10kΩ Pull-up Widerstand nach 3.3V beschaltet. Die Taster verbinden bei Druck den GPIO mit GND. Alle sechs Taster können den ESP32 aus dem Deep Sleep aufwecken. Die GPIO-Belegung: Taster 1 an GPIO 1, Taster 2 an GPIO 2, Taster 3 an GPIO 4, Taster 4 an GPIO 5, Taster 5 an GPIO 6, Taster 6 an GPIO 7. Alle Taster teilen sich einen gemeinsamen GND.

### Stromversorgung
Die Stromversorgung erfolgt über einen 18650 Li-Ion Akku (geschützt oder ungeschützt) im integrierten Halter. Der Spannungsbereich liegt zwischen 3.0V und 4.2V. Über USB-C kann mit bis zu 500mA geladen werden. Bei Unterschreitung von 3.2V sollte der Sender nicht mehr senden (nur noch Aufwecken) – der integrierte Tiefentladeschutz schaltet bei Bedarf ab.

### Benötigte Komponenten (Stückliste)
Für das Projekt werden folgende Komponenten benötigt: ein LILYGO T-Energy-S3 Board, sechs Taster als 6x6x5mm Tact Switch, sechs 10kΩ Widerstände (1/4W) für die Pull-ups, drei 220Ω Widerstände (1/4W) für die LED, eine RGB-LED mit gemeinsamer Anode (5mm), ein geschützter 18650 Akku, 0.14mm² flexible Litze in verschiedenen Farben und optional eine Lochrasterplatine (7x5 cm) für saubere Verdrahtung.

---

## 2. Schaltplan (Textuell)

### Taster-Verdrahtung (für alle 6 Taster identisch)
Die Verdrahtung ist für alle sechs Taster identisch. Für Taster 1: 3.3V wird mit einem 10kΩ Widerstand verbunden, der Widerstand wird mit GPIO 1 verbunden. Parallel dazu wird der 10kΩ Widerstand auch mit Taster 1 (Pin 1) verbunden. Taster 1 (Pin 2) wird mit GND 

**Taster 1:**
- 3.3V → 10kΩ Widerstand → GPIO 1
- 3.3V → 10kΩ Widerstand → Taster 1 (Pin 1)
- Taster 1 (Pin 2) → GND

**Taster 2:**
- 3.3V → 10kΩ Widerstand → GPIO 2
- 3.3V → 10kΩ Widerstand → Taster 2 (Pin 1)
- Taster 2 (Pin 2) → GND

**Taster 3:**
- 3.3V → 10kΩ Widerstand → GPIO 4
- 3.3V → 10kΩ Widerstand → Taster 3 (Pin 1)
- Taster 3 (Pin 2) → GND

**Taster 4:**
- 3.3V → 10kΩ Widerstand → GPIO 5
- 3.3V → 10kΩ Widerstand → Taster 4 (Pin 1)
- Taster 4 (Pin 2) → GND

**Taster 5:**
- 3.3V → 10kΩ Widerstand → GPIO 6
- 3.3V → 10kΩ Widerstand → Taster 5 (Pin 1)
- Taster 5 (Pin 2) → GND

**Taster 6:**
- 3.3V → 10kΩ Widerstand → GPIO 7
- 3.3V → 10kΩ Widerstand → Taster 6 (Pin 1)
- Taster 6 (Pin 2) → GND

### RGB-LED Verdrahtung (gemeinsame Anode)

- 3.3V → LED Anode (längster Pin)
- GPIO 10 → 220Ω Widerstand → Rot-Kathode
- GPIO 11 → 220Ω Widerstand → Grün-Kathode
- GPIO 12 → 220Ω Widerstand → Blau-Kathode

### Verdrahtungstabelle

| Von | Nach | Kabeltyp |
|-----|------|----------|
| 3.3V | 10kΩ Widerstände (x6) | 0.14mm² |
| 10kΩ | GPIO 1-7 | 0.14mm² |
| GPIO 1-7 | Taster (Pin1) | 0.14mm² |
| Taster (Pin2) | GND | 0.14mm² |
| GPIO 10 | 220Ω → Rot-Kathode | 0.14mm² |
| GPIO 11 | 220Ω → Grün-Kathode | 0.14mm² |
| GPIO 12 | 220Ω → Blau-Kathode | 0.14mm² |
| LED Anode | 3.3V | 0.14mm² |
| GND-Schiene | GND (ESP32) | 0.25mm² |
---

## 3. Status-LED Blinksequenzen

### RGB-LED Farb- und Blinkdefinitionen

| Zustand | Farbe | Blinkmuster | Dauer | Bedeutung |
|---------|-------|-------------|-------|-----------|
| Deep Sleep | Aus | - | - | ESP32 schläft |
| Aufwachen | Cyan (Grün+Blau) | Einmal kurz | 100ms | System gestartet |
| Taster OK | Grün | Kurz aufblitzen | 100ms | Einzelner Taster erkannt |
| Mehrfach-Taste | Rot | 3x schnell blinken | 200ms an, 100ms aus | FEHLER: Mehrere Taster |
| Senden | Blau | Einmal mittel | 200ms | ESP-NOW Datenübertragung |
| Sendebestätigung | Grün | 2x kurz | 100ms an, 80ms aus | Erfolgreich gesendet |
| Sendefehler | Rot | 5x schnell | 50ms an, 50ms aus | Übertragung fehlgeschlagen |
| Batterie kritisch | Rot | Langsames Atmen | 2s an, 2s aus | Akku < 3.3V |
| Batterie lädt | Orange (Rot+Grün) | Pulsierend | 1s an, 1s aus | USB-C angeschlossen |
| Batterie voll | Grün | Dauerhaft an | - | Ladevorgang abgeschlossen |
| Inaktiv-Timeout | Cyan | 1x lang | 1000ms | Gehe in Deep Sleep |

### Prioritäten der LED-Anzeigen

1. Batterie kritisch (Sicherheitswarnung)
2. Mehrfach-Taster (Fehlerfall)
3. Sendefehler (Kommunikationsproblem)
4. Senden (Aktivität)
5. Taster OK (Normalfall)
---

## 4. Installation & Inbetriebnahme

### Projektstruktur
Für das Projekt wird eine klare Ordnerstruktur empfohlen. Im Hauptverzeichnis `esp32_sender` befinden sich die `platformio.ini` (Projekt-Konfiguration) und die `README.md` (diese Dokumentation). Im Unterordner `src` liegt die `main.cpp` mit dem Quellcode.

### Schritt-für-Schritt Anleitung
Zuerst wird der Projektordner angelegt mit den Befehlen `mkdir esp32_sender`, `cd esp32_sender` und `mkdir src`. Anschließend muss die platformio.ini mit den richtigen Einstellungen für das LILYGO T-Energy-S3 Board erstellt werden. Der vollständige Quellcode für src/main.cpp wird aus der separaten Codedatei übernommen.

### Empfänger-MAC-Adresse ermitteln
Die MAC-Adresse des Empfängers wird mit einem einfachen Programm ausgelesen. Dazu wird der Empfänger-ESP32 mit folgendem Code programmiert: Zuerst wird die serielle Kommunikation mit 115200 Baud gestartet, dann wird der WiFi-Modus auf Station gesetzt und die MAC-Adresse über die serielle Schnittstelle ausgegeben. Die Ausgabe erscheint im seriellen Monitor im Format AA:BB:CC:DD:EE:FF. Diese Adresse wird im Quellcode bei receiverMac[] eingetragen.

### Code hochladen
Zum Hochladen wird VS Code mit der PlatformIO Extension verwendet. Nach dem Öffnen des Projektordners kann über das PlatformIO Icon "Upload and Monitor" ausgeführt werden. Bei erfolgreichem Upload erscheint die serielle Ausgabe im Monitor.

---

## 5. Testanleitung

### Vorbereitung
Vor dem Test müssen folgende Punkte erfüllt sein: Der 18650 Akku ist eingelegt oder USB-C angeschlossen. Taster und LED sind laut Schaltplan verdrahtet. Der Empfänger ist eingeschaltet und bereit. Der serielle Monitor ist für Debug-Zwecke geöffnet.

### Test 1: Grundfunktionen
Beim USB-C Laden sollte die LED orange pulsieren. Bei voller Batterie und angeschlossenem USB leuchtet die LED dauerhaft grün. Beim Drücken des Reset-Tasters blinkt die LED kurz cyan und es erscheint eine serielle Ausgabe. Nach 30 Sekunden Inaktivität blinkt die LED einmal lang cyan und erlischt dann – der ESP32 ist im Deep Sleep.

### Test 2: Taster-Einzeltests
Jeder Taster wird einzeln getestet. Bei kurzem Drücken sollte folgende Sequenz erscheinen: Grün (Taster OK) gefolgt von Blau (Senden) gefolgt von zweimal kurzem Grün (Erfolg). Dies wird für alle sechs Taster nacheinander durchgeführt.

### Test 3: Mehrfach-Taster
Bei gleichzeitigem Drücken von zwei Tastern (z.B. Taster 1+2) blinkt die LED dreimal rot und es wird nichts gesendet. Bei drei gleichzeitigen Tastern (Taster 3+4+5) ebenfalls dreimal rot ohne Senden. Auch bei allen sechs Tastern gleichzeitig erscheint die dreimal rote Fehler-LED. Im seriellen Monitor erscheint die Meldung "WARNUNG: X Taster gleichzeitig!".

### Test 4: Deep Sleep Verhalten
Nach 30 Sekunden ohne Tastendruck geht der ESP32 in den Deep Sleep (LED aus). Jeder Taster (1 bis 6) kann den ESP32 aus dem Schlaf aufwecken – die LED blinkt kurz cyan und der ESP32 reagiert wieder auf Tastendruck. Nach dem Aufwecken und erneuter Inaktivität geht er wieder in den Schlaf.

### Test 5: Batterie-Überwachung
Im seriellen Monitor erscheint regelmäßig "Batteriespannung: X.XXV". Bei normaler Batterie (>3.3V) gibt es keine Warnung. Bei simulierter Unterspannung (<3.3V) atmet die LED langsam rot. Bei angeschlossenem USB und leerem Akku pulsiert die LED orange. Bei angeschlossenem USB und vollem Akku leuchtet die LED dauerhaft grün.

### Test 6: ESP-NOW Kommunikation
Bei eingeschaltetem Empfänger und Tastendruck erscheint die grüne Erfolgs-LED (zweimal kurz) und der Empfänger reagiert. Bei ausgeschaltetem Empfänger blinkt die LED fünfmal schnell rot (Fehler). Dieser Test sollte mehrfach wiederholt werden.

### Testprotokoll
Alle Testergebnisse sollten in einem Protokoll festgehalten werden mit Datum, Name des Testers, Ergebnis und eventuellen Bemerkungen.

---

## 6. Fehlersuche

### Problem: ESP32 wacht nicht auf
Mögliche Ursachen sind eine falsche Verdrahtung der Taster, fehlende Pull-up Widerstände oder eine falsch konfigurierte Bitmaske. Zur Lösung sollte geprüft werden, ob die Taster GPIO mit GND verbinden bei Druck. Mit einem Multimeter sollte die Spannung an den GPIOs im Ruhezustand etwa 3.3V betragen. Die Bitmaske muss für jeden Taster als (1ULL << GPIO) konfiguriert sein. Zur Diagnose kann esp_sleep_get_wakeup_cause() ausgegeben werden.

### Problem: LED leuchtet gar nicht
Mögliche Ursachen sind fehlende oder falsche Vorwiderstände, bei RGB-LED eine falsch angeschlossene Anode oder falsch konfigurierte GPIO-Pins. Zur Lösung sollte der Vorwiderstand auf 220-470Ω geprüft werden. Bei RGB-LED muss die gemeinsame Anode an 3.3V liegen (das ist der längste Pin). Zum Test kann digitalWrite(LED_PIN, LOW) im Setup eingebaut werden.

### Problem: Mehrfach-Taster wird erkannt obwohl einzeln
Mögliche Ursachen sind prellende Taster (mechanisch), eine schlechte Masseverbindung oder eine zu kurze Entprellzeit. Zur Lösung sollte die Entprellzeit DEBOUNCE_DELAY auf 100ms erhöht werden. Die gemeinsame GND-Schiene muss geprüft werden. Die Taster sollten einzeln durchgemessen werden.

### Problem: Keine ESP-NOW Kommunikation
Mögliche Ursachen sind eine falsche MAC-Adresse des Empfängers, ein nicht bereiter Empfänger oder zu große Entfernung. Zur Lösung muss die MAC-Adresse neu ausgelesen und geprüft werden. Beide ESP32 sollten nah nebeneinander gehalten werden. Die serielle Ausgabe "ESP-NOW initialisiert" muss erscheinen. Es muss geprüft werden, ob WiFi.mode(WIFI_STA) aufgerufen wurde.

### Problem: Batterie wird nicht korrekt gemessen
Mögliche Ursachen sind ein falscher ADC-Pin (beim T-Energy-S3 ist GPIO3 korrekt), eine falsche ADC-Referenzspannung oder eine falsche Formel. Zur Lösung sollte das Datenblatt des Boards geprüft werden. Die Formel (raw / 4095.0) * 2.0 * 3.3 * 1.02 muss verwendet werden. Die Messung sollte mit einem Multimeter verglichen werden.

### Problem: ESP32 geht zu früh in Deep Sleep
Mögliche Ursachen sind, dass lastButtonPress nicht aktualisiert wird, dass Taster nicht erkannt werden oder dass INACTIVITY_TIMEOUT zu klein ist. Zur Lösung sollte INACTIVITY_TIMEOUT erhöht werden (z.B. auf 60). Im Code kann eine serielle Ausgabe "lastButtonPress = ..." eingebaut werden. Es muss geprüft werden, ob lastButtonPress = millis() im Code steht.

### Problem: Komische Farben bei RGB-LED
Mögliche Ursachen sind die Verwechslung von gemeinsamer Anode und gemeinsamer Kathode, fehlende oder falsche Vorwiderstände oder vertauschte GPIOs. Zur Lösung gilt: Bei gemeinsamer Anode ist LOW = EIN, HIGH = AUS. Bei gemeinsamer Kathode ist HIGH = EIN, LOW = AUS. Zum Test sollten einzeln Rot, Grün und Blau eingeschaltet werden.

---

## 7. Konfigurationsmöglichkeiten

### Parameter im Quellcode
Im Quellcode können verschiedene Parameter angepasst werden. INACTIVITY_TIMEOUT bestimmt die Sekunden ohne Tastendruck bis zum Deep Sleep (Standard 30). DEBOUNCE_DELAY ist die Entprellzeit in Millisekunden (Standard 50). BUTTON_HOLD_TIMEOUT begrenzt die maximale Taster-Haltezeit auf 5000 ms. BATTERY_MIN_VOLTAGE definiert die kritische Batterieschwelle bei 3.3 Volt. BATTERY_FULL_VOLTAGE setzt die Schwelle für "Batterie voll" auf 4.1 Volt. receiverMac[] muss auf die tatsächliche MAC-Adresse des Empfängers gesetzt werden.

### Anpassungsmöglichkeiten
Für eine längere Aktivitätszeit wird INACTIVITY_TIMEOUT auf 60 gesetzt (60 Sekunden statt 30). Bei prellenden Tastern wird DEBOUNCE_DELAY auf 100 erhöht (100ms Entprellung). Die receiverMac[] muss auf die tatsächliche MAC-Adresse des Empfängers gesetzt werden, zum Beispiel {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}. Für andere Batterie-Schwellen kann BATTERY_MIN_VOLTAGE auf 3.2 gesetzt werden (kritisch bei 3.2V) und BATTERY_FULL_VOLTAGE auf 4.0 (voll bei 4.0V).

---

## 8. Batterielaufzeit

### Berechnungsgrundlage
Die Berechnung der Batterielaufzeit basiert auf folgenden Werten: Der Deep Sleep Strom beträgt ca. 20 µA. Der Aktiv-Strom beträgt ca. 80 mA für etwa 500ms pro Tastendruck. Die Akku-Kapazität eines typischen 18650 beträgt 3500 mAh.

### Geschätzte Laufzeiten
Bei seltener Nutzung mit 10 Tastendrücken pro Tag und 30 Sekunden Timeout ergibt sich ein täglicher Verbrauch von 1.2 mAh, was einer Laufzeit von über 8 Jahren entspricht. Bei gleicher Nutzung mit 60 Sekunden Timeout sind es 1.8 mAh pro Tag und über 5 Jahre Laufzeit. Bei häufiger Nutzung mit 50 Tastendrücken pro Tag und 30 Sekunden Timeout beträgt der Verbrauch 2.5 mAh pro Tag mit über 3.5 Jahren Laufzeit. Bei 50 Tastendrücken und 60 Sekunden Timeout sind es 4.0 mAh pro Tag und etwa 2.5 Jahre Laufzeit.

### Einflussfaktoren auf die Batterielaufzeit
Mehrere Faktoren beeinflussen die tatsächliche Batterielaufzeit. Ein längerer Timeout bedeutet höheren Verbrauch. Mehr Tastendrücke pro Tag erhöhen den Verbrauch. Kälte reduziert die Akku-Kapazität. Marken-Akkus haben eine höhere Qualität und längere Lebensdauer. USB-Ladezyklen sollten vermieden werden – nur bei Bedarf laden.

### Batterie-Überwachung
Die Batteriespannung wird automatisch überwacht. Im Normalbetrieb über 3.3V ist alles in Ordnung. Unter 3.3V wird die Spannung kritisch – die LED warnt durch rotes Atmen, Senden ist aber noch möglich. Unter 3.1V schaltet der Hardware-Tiefentladeschutz (falls vorhanden) ab.

### Ladeempfehlungen
Der Akku sollte nachgeladen werden, wenn die rote LED zu atmen beginnt (ca. 3.3V). Die Ladezeit über USB-C beträgt etwa 4-6 Stunden. Es werden geschützte 18650 Zellen empfohlen. Bei längerer Nichtbenutzung sollte der Akku auf ca. 50% geladen gelagert werden.

---

## 📋 Abschließende Checkliste

### Hardware-Check
- LILYGO T-Energy-S3 Board vorhanden
- 6 Taster mit 10kΩ Pull-ups verdrahtet
- RGB-LED mit 220Ω Vorwiderständen verdrahtet
- 18650 Akku eingelegt
- Alle Verbindungen auf Kurzschluss geprüft

### Software-Check
- PlatformIO-Projekt angelegt
- platformio.ini korrekt erstellt
- Empfänger-MAC-Adresse im Quellcode eingetragen
- Code kompiliert ohne Fehler
- Code erfolgreich hochgeladen

### Funktions-Check
- Alle 6 Taster funktionieren einzeln
- Mehrfach-Taster werden ignoriert (rote LED)
- Deep Sleep nach 30 Sekunden Inaktivität
- Aufwecken durch jeden Taster möglich
- ESP-NOW Kommunikation mit Empfänger funktioniert
- Batterie-Überwachung zeigt korrekte Werte
- Alle LED-Statusmeldungen sichtbar und korrekt

### Dokumentations-Check
- README.md im Projektordner vorhanden
- Empfänger-MAC-Adresse notiert
- Testprotokoll ausgefüllt

---

## 🔧 Wartungshinweise
Der Akku sollte gewechselt werden, wenn die rote LED häufig leuchtet (Batterie kritisch). Bei Tastern, die nicht reagieren, müssen die Kontakte geprüft werden. Nach längerer Nichtbenutzung sollte der Akku neu geladen werden. Bei neuen Funktionen oder Fehlerbehebungen ist ein Firmware-Update durchzuführen. Gelegentlich sollten die Kontakte mit Kontaktspray gereinigt werden.

---

## 🆘 Support
Bei Problemen sollte zuerst der serielle Monitor geöffnet werden – er gibt oft Hinweise auf Fehler. Dann sollte das Fehlersuche-Kapitel durchgegangen werden. Die Verdrahtung muss nochmals gegen den Schaltplan geprüft werden. Einzelkomponenten können getestet werden, zum Beispiel die LED einzeln ansteuern. Die MAC-Adresse sollte nochmals ausgelesen werden – das ist die häufigste Fehlerquelle.

---

**Dokumentation abgeschlossen** ✅