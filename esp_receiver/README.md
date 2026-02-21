# ESP32-Empfänger für Markisensteuerung (ESP32 WROOM)

Netzbetriebener Empfänger mit 6 Relais-Ausgängen zur Ansteuerung von 3 Motoren (Links/Rechtslauf pro Motor). Empfängt Taster-Signale per ESP-NOW vom Sender und schaltet entsprechende Ausgänge über einen ULN2803 Darlington-Treiber.

---

## 📋 Inhaltsverzeichnis
1. [Hardware-Beschreibung](#1-hardware-beschreibung)
2. [Schaltplan (Textuell)](#2-schaltplan-textuell)
3. [Funktionsbeschreibung](#3-funktionsbeschreibung)
4. [Installation & Inbetriebnahme](#4-installation--inbetriebnahme)
5. [Testanleitung](#5-testanleitung)
6. [Fehlersuche](#6-fehlersuche)
7. [Konfigurationsmöglichkeiten](#7-konfigurationsmöglichkeiten)
8. [Sicherheitshinweise](#8-sicherheitshinweise)

---

## 1. Hardware-Beschreibung

### Hauptkomponente: ESP32 WROOM
Das ESP32 WROOM Dev Board ist ein klassisches ESP32-Entwicklungsboard mit einem ESP32-D0WDQ6 Dual-Core Prozessor mit 240 MHz. Es verfügt über 24 nutzbare GPIOs mit 3.3V Pegel und wird über 5V per USB oder externem Netzteil versorgt. Das Board ist weit verbreitet und bietet eine gute Community-Unterstützung.

### ULN2803 Darlington-Treiber
Der ULN2803 ist ein 8-Kanal Darlington-Transistor-Array. Die Eingänge sind mit 3.3V bis 5V kompatibel und können direkt von ESP32-GPIOs angesteuert werden. Die Ausgänge liefern bis zu 500mA pro Kanal bei bis zu 50V. Besonders wichtig: Der ULN2803 hat integrierte Freilaufdioden für induktive Lasten wie Relais.

### Relais (für Motoransteuerung)
Es werden sechs 5V Relais verwendet, zum Beispiel vom Typ Omron G5LE oder JQC-3FF. Die Relais haben eine Spulenspannung von 5V DC und die Kontakte sind für 10A bei 250V AC ausgelegt, was für Motoransteuerung geeignet ist. Die Relais sind bereits in einer bestehenden Schaltung für die Motoransteuerung installiert.

### Motoren (3 Stück)
Drei Motoren werden angesteuert, jeder mit Links- und Rechtslauf. Motor 1 (z.B. Markise 1) wird über Taster 1 (Linkslauf) und Taster 2 (Rechtslauf) gesteuert. Motor 2 (z.B. Markise 2) wird über Taster 3 (Linkslauf) und Taster 4 (Rechtslauf) gesteuert. Motor 3 (z.B. Markise 3) wird über Taster 5 (Linkslauf) und Taster 6 (Rechtslauf) gesteuert.

### Benötigte Komponenten (Stückliste)
Für das Projekt werden folgende Komponenten benötigt: ein ESP32 WROOM Dev Board, ein ULN2803 Darlington-Treiber im DIP-18 Gehäuse, sechs 10kΩ Widerstände (1/4W) für Pull-down, ein 100µF / 16V Kondensator, zwei 100nF Keramikkondensatoren, 0.14mm² und 0.5mm² Kabel, ein 5V / 2A Netzteil (z.B. altes Handy-Netzteil) und ein DC-Hohlstecker 5.5mm x 2.1mm. Die sechs Relais sind in der bestehenden Schaltung bereits vorhanden.

---

## 2. Schaltplan (Textuell)

### Übersicht
Der ESP32 empfängt per ESP-NOW die Taster-Signale vom Sender. Die Ausgänge des ESP32 (3.3V) steuern den ULN2803 an, der die 5V Relais schaltet. Die Relais sind in einer bestehenden Schaltung für die Motoransteuerung installiert.

### ULN2803 Pinbelegung
Der ULN2803 hat 18 Pins. Pin 1 (IN1) bis Pin 8 (IN8) sind die Eingänge. Pin 9 ist GND. Pin 10 ist COM (gemeinsamer Anschluss für Freilaufdioden). Pin 11 bis Pin 18 sind die Ausgänge OUT8 bis OUT1.

### Verdrahtung ESP32 → ULN2803
Die ESP32-GPIOs werden mit den ULN2803-Eingängen verbunden. GPIO 26 geht an IN1 (Pin 1) für Motor 1 Linkslauf (Taster 1). GPIO 27 geht an IN2 (Pin 2) für Motor 1 Rechtslauf (Taster 2). GPIO 14 geht an IN3 (Pin 3) für Motor 2 Linkslauf (Taster 3). GPIO 12 geht an IN4 (Pin 4) für Motor 2 Rechtslauf (Taster 4). GPIO 13 geht an IN5 (Pin 5) für Motor 3 Linkslauf (Taster 5). GPIO 15 geht an IN6 (Pin 6) für Motor 3 Rechtslauf (Taster 6). Die Pins IN7 und IN8 bleiben ungenutzt.

**Wichtiger Hinweis zu GPIO 12:** GPIO 12 ist ein Bootstrapping-Pin und muss beim Booten LOW sein. Durch den ULN2803-Eingang (hochohmig) und einen externen 10kΩ Pull-down Widerstand nach GND wird dies sichergestellt.

### Verdrahtung ULN2803 → Relais
Die ULN2803-Ausgänge werden mit den Relais-Spulen verbunden. OUT1 (Pin 18) geht an Relais K1 für Motor 1 Linkslauf. OUT2 (Pin 17) geht an Relais K2 für Motor 1 Rechtslauf. OUT3 (Pin 16) geht an Relais K3 für Motor 2 Linkslauf. OUT4 (Pin 15) geht an Relais K4 für Motor 2 Rechtslauf. OUT5 (Pin 14) geht an Relais K5 für Motor 3 Linkslauf. OUT6 (Pin 13) geht an Relais K6 für Motor 3 Rechtslauf.

**Wichtig:** Die Relais sind in einer bestehenden Schaltung bereits mit den Motoren verbunden. Der ULN2803 schaltet nur die Masse (GND) für die Relaisspulen. Die Plus-Seite der Relaisspulen (5V) ist bereits in der bestehenden Schaltung verdrahtet.

### Stromversorgung
Ein 5V Netzteil mit 2A liefert die Spannung. Vom Netzteil wird ein 100µF Kondensator zur Glättung und ein 100nF Kondensator zur Entstörung nach GND geschaltet. Die 5V gehen dann an den ESP32 (über USB oder 5V-Pin), an die Relaisspulen (in der bestehenden Schaltung) und an den ULN2803 COM-Pin (Pin 10) für die Freilaufdioden.

### Pull-down Widerstände für GPIOs
Um sicherzustellen, dass die Ausgänge beim Booten definiert LOW sind, werden 10kΩ Pull-down Widerstände empfohlen. GPIO 26, 27, 14, 13 und 15 erhalten jeweils einen 10kΩ nach GND. Für GPIO 12 ist ein 10kΩ Pull-down nach GND zwingend erforderlich.

### Komplette Verdrahtungstabelle
Die folgende Tabelle zeigt alle Verbindungen im Überblick:
- ESP32 GPIO 26 → ULN2803 IN1 (Pin 1) mit 0.14mm²
- ESP32 GPIO 27 → ULN2803 IN2 (Pin 2) mit 0.14mm²
- ESP32 GPIO 14 → ULN2803 IN3 (Pin 3) mit 0.14mm²
- ESP32 GPIO 12 → ULN2803 IN4 (Pin 4) mit 0.14mm²
- ESP32 GPIO 13 → ULN2803 IN5 (Pin 5) mit 0.14mm²
- ESP32 GPIO 15 → ULN2803 IN6 (Pin 6) mit 0.14mm²
- ULN2803 OUT1 (Pin 18) → Relais K1 Spule (-) mit 0.5mm²
- ULN2803 OUT2 (Pin 17) → Relais K2 Spule (-) mit 0.5mm²
- ULN2803 OUT3 (Pin 16) → Relais K3 Spule (-) mit 0.5mm²
- ULN2803 OUT4 (Pin 15) → Relais K4 Spule (-) mit 0.5mm²
- ULN2803 OUT5 (Pin 14) → Relais K5 Spule (-) mit 0.5mm²
- ULN2803 OUT6 (Pin 13) → Relais K6 Spule (-) mit 0.5mm²
- ULN2803 COM (Pin 10) → 5V (von Netzteil) mit 0.5mm²
- ULN2803 GND (Pin 9) → GND (Netzteil) mit 0.5mm²
- ESP32 GND → GND (Netzteil) mit 0.5mm²
- Alle GPIOs 26,27,14,12,13,15 → 10kΩ nach GND mit 0.14mm²

---

## 3. Funktionsbeschreibung

### Kommunikation mit dem Sender
Der Empfänger verwendet ESP-NOW für die Kommunikation mit dem Sender. ESP-NOW ist ein Peer-to-Peer Protokoll von Espressif, das ohne WLAN-Router auskommt. Der Empfänger registriert den Sender als Peer und empfängt dessen Datenpakete.

### Empfangenes Datenpaket
Vom Sender wird folgende Datenstruktur empfangen: Eine buttonMask als Byte, bei dem Bit 0-5 den Status der Taster 1-6 anzeigt (1 = gedrückt). Die batteryVoltage gibt die Batteriespannung des Senders für Diagnosezwecke an. Die sequence ist eine Sequenznummer zur Erkennung von doppelten Paketen. Der rssi-Wert zeigt die Signalstärke der Verbindung an.

### Verarbeitungslogik
Der ESP32 empfängt ein Datenpaket per ESP-NOW und wertet die buttonMask aus. Bei gesetztem Bit wird der entsprechende Ausgang auf HIGH gesetzt (Relais an). Bei nicht gesetztem Bit wird der Ausgang auf LOW gesetzt (Relais aus). Der Status wird auf dem seriellen Monitor ausgegeben.

### Wichtige Sicherheitsfunktionen
Eine gleichzeitige Ansteuerung eines Motors in beide Richtungen ist nicht möglich. Bei fehlerhaften Paketen, bei denen beide Bits für einen Motor gesetzt sind, wird nichts geschaltet und eine Fehlermeldung ausgegeben. Eine Timeout-Funktion schaltet alle Ausgänge aus, falls länger als 2 Sekunden kein Paket empfangen wird – das ist eine Sicherheitsfunktion bei Verbindungsabbruch. Eine Entprellung sorgt dafür, dass kurze Tastendrücke zuverlässig erkannt werden. Ein MAC-Adress-Filter stellt sicher, dass nur der konfigurierte Sender akzeptiert wird.

### Statusausgabe über seriellen Monitor
Der serielle Monitor zeigt folgende Informationen an: Die empfangene Taster-Maske in binärer und hexadezimaler Darstellung, die daraus abgeleiteten Motor-Befehle (z.B. "Motor 1: Linkslauf"), die Batteriespannung des Senders, die Signalstärke (RSSI), Fehlermeldungen bei ungültigen Paketen und Timeout-Warnungen.

---

## 4. Installation & Inbetriebnahme

### Projektstruktur
Für das Projekt wird folgende Ordnerstruktur empfohlen:


### platformio.ini Konfiguration
Die platformio.ini muss mit den richtigen Einstellungen für das ESP32 WROOM Board erstellt werden. Sie definiert die Plattform, das Board, das Framework, die serielle Geschwindigkeit, die benötigten Bibliotheken (insbesondere ESP-NOW) und die Compiler-Flags für Optimierung.

### main.cpp Quellcode
Der vollständige Quellcode für src/main.cpp enthält alle notwendigen Funktionen: Die Initialisierung der Ausgänge, die ESP-NOW Kommunikation mit Callback-Funktion für den Empfang, die Auswertung der Taster-Maske mit Plausibilitätsprüfung für die Motoren, die Ansteuerung der Ausgänge, die Timeout-Überwachung und die umfangreiche Statusausgabe über seriellen Monitor.

### Schritt-für-Schritt Installationsanleitung

**1. Projektordner erstellen:**
Zuerst wird ein neuer Ordner für das Projekt angelegt. Mit dem Befehl `mkdir esp32_receiver` wird der Hauptordner erstellt. Mit `cd esp32_receiver` wechselt man in diesen Ordner. Mit `mkdir src` wird der Unterordner für den Quellcode angelegt.

**2. Dateien erstellen:**
Im Hauptverzeichnis wird die Datei `platformio.ini` mit dem entsprechenden Inhalt erstellt. Im Unterordner `src` wird die Datei `main.cpp` mit dem vollständigen Quellcode erstellt.

**3. Eigene MAC-Adresse ermitteln:**
Der Code wird auf den ESP32 hochgeladen. Nach dem Öffnen des seriellen Monitors (115200 Baud) zeigt die Ausgabe "Eigene MAC-Adresse: XX:XX:XX:XX:XX:XX". Diese Adresse muss notiert werden, da sie für den Sender benötigt wird.

**4. Sender-MAC-Adresse ermitteln und eintragen:**
Der Sender muss seine MAC-Adresse ausgeben (siehe Sender-Dokumentation). Diese Adresse wird in `src/main.cpp` bei `senderMac[]` im Format {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF} eingetragen.

**5. Hardware aufbauen:**
ESP32, ULN2803 und Relais werden laut Schaltplan verdrahtet. Besonders auf den Pull-down Widerstand an GPIO 12 muss geachtet werden. Alle Verbindungen sollten vor dem Einschalten geprüft werden.

**6. Code kompilieren & hochladen:**
VS Code wird gestartet und die PlatformIO Extension installiert (falls nicht vorhanden). Der Projektordner wird geöffnet. Über das PlatformIO Icon wird "Upload and Monitor" ausgeführt. Bei erfolgreichem Upload erscheint die serielle Ausgabe im Monitor.

---

## 5. Testanleitung

### Vorbereitung
Vor dem Test müssen folgende Punkte erfüllt sein: Der ESP32 ist mit ULN2803 und Relais verdrahtet, ein 5V Netzteil ist angeschlossen, der Sender ist eingeschaltet und betriebsbereit, der serielle Monitor ist geöffnet (115200 Baud).

### Test 1: Grundfunktionen
Beim Einschalten des Netzteils bootet der ESP32 und es erscheint die serielle Ausgabe. Die eigene MAC-Adresse wird ausgegeben. Die Meldung "ESP-NOW initialisiert" bestätigt die erfolgreiche Initialisierung.

### Test 2: Kommunikation mit Sender
Beim Einschalten des Senders zeigt der Empfänger "Paket empfangen". Bei kurzem Drücken von Taster 1 erscheint "Motor 1 Linkslauf: EIN" und sofort danach "AUS". Analog dazu bei Taster 2 "Motor 1 Rechtslauf: EIN/AUS", bei Taster 3 "Motor 2 Linkslauf: EIN/AUS", bei Taster 4 "Motor 2 Rechtslauf: EIN/AUS", bei Taster 5 "Motor 3 Linkslauf: EIN/AUS" und bei Taster 6 "Motor 3 Rechtslauf: EIN/AUS".

### Test 3: Dauerbetrieb (Taster halten)
Wird Taster 1 für 2-3 Sekunden gedrückt gehalten, bleibt "Motor 1 Linkslauf: EIN" während der gesamten Dauer an und schaltet erst beim Loslassen aus. Dies gilt analog für alle anderen Taster.

### Test 4: Ungültige Kombinationen (Sicherheitsfunktion)
Werden Taster 1 und 2 gleichzeitig gedrückt, erscheint eine Fehlermeldung "FEHLER: Motor 1 würde gleichzeitig Links und Rechtslauf erhalten!" und beide Ausgänge bleiben aus. Gleiches gilt für Taster 3+4 (Motor 2) und Taster 5+6 (Motor 3).

### Test 5: Timeout-Funktion
Wird der Sender ausgeschaltet, erscheint nach ca. 2 Sekunden die Meldung "ALLE AUSGÄNGE AUSGESCHALTET (Timeout)". Wird der Sender wieder eingeschaltet, funktioniert die Kommunikation sofort wieder.

### Test 6: Relais-Funktion (mit angeschlossener Schaltung)
Bei Taster 1 zieht Relais K1 hörbar an und Motor 1 läuft links. Bei Taster 2 zieht Relais K2 an und Motor 1 läuft rechts. Bei Taster 3 zieht Relais K3 an und Motor 2 läuft links. Bei Taster 4 zieht Relais K4 an und Motor 2 läuft rechts. Bei Taster 5 zieht Relais K5 an und Motor 3 läuft links. Bei Taster 6 zieht Relais K6 an und Motor 3 läuft rechts.

### Testprotokoll
Alle Testergebnisse sollten in einem Protokoll festgehalten werden mit Datum, Name des Testers, Ergebnis und eventuellen Bemerkungen.

---

## 6. Fehlersuche

### Problem: ESP-NOW initialisiert nicht
Mögliche Ursachen sind, dass WiFi nicht gestartet werden konnte oder dass ESP-NOW bereits verwendet wird. Zur Lösung sollte geprüft werden, ob `WiFi.mode(WIFI_STA)` aufgerufen wurde. Ein Neustart des ESP32 kann helfen. Andere WLAN-Funktionen sollten deaktiviert werden.

### Problem: Keine Pakete vom Sender
Mögliche Ursachen sind eine falsch eingetragene MAC-Adresse des Senders, ein ausgeschalteter oder nicht in Reichweite befindlicher Sender oder dass beide ESP32 nicht auf dem gleichen Kanal sind. Zur Lösung muss die MAC-Adresse des Senders neu ausgelesen und geprüft werden. Der Sender sollte näher an den Empfänger gebracht werden. Der serielle Monitor des Senders kann beobachtet werden, um zu prüfen, ob der Sender überhaupt sendet.

### Problem: Unbekannter Absender wird angezeigt
Mögliche Ursachen sind, dass ein anderer ESP-NOW Sender in der Nähe ist oder dass die eigene MAC-Adresse falsch im Sender konfiguriert wurde. Zur Lösung muss die MAC-Adresse des Senders korrigiert werden. Es sollte nur der eigene Sender verwendet werden.

### Problem: Ausgänge schalten nicht
Mögliche Ursachen sind, dass GPIOs nicht als OUTPUT konfiguriert wurden, eine falsche Verdrahtung zum ULN2803 vorliegt oder der ULN2803 nicht richtig versorgt wird. Zur Lösung sollte geprüft werden, ob `pinMode(pin, OUTPUT)` aufgerufen wurde. Die Verdrahtung ESP32 → ULN2803 muss überprüft werden. Es ist zu prüfen, ob ULN2803 GND und COM richtig angeschlossen sind. Zum Test kann `digitalWrite(pin, HIGH)` im Setup eingebaut werden.

### Problem: Relais schalten nicht
Mögliche Ursachen sind, dass der ULN2803 Ausgang kein GND liefert, die Relaisspule defekt ist oder die 5V Versorgung für die Relais fehlt. Zur Lösung sollte die Spannung an der Relaisspule gemessen werden (5V zwischen Plus und ULN-Ausgang). Es ist zu prüfen, ob ULN2803 COM an 5V angeschlossen ist. Die Relais können einzeln getestet werden, indem 5V direkt an die Spule gelegt werden.

### Problem: ESP32 bootet nicht / startet neu
Mögliche Ursachen sind, dass GPIO 12 beim Booten HIGH ist (Bootstrapping-Problem) oder dass die Stromversorgung zu schwach ist (Spannungseinbrüche). Zur Lösung muss ein 10kΩ Pull-down Widerstand an GPIO 12 nach GND angeschlossen werden. Das Netzteil sollte ausreichend dimensioniert sein (mindestens 2A). Zusätzliche Kondensatoren können Spannungseinbrüche abfangen.

---

## 7. Konfigurationsmöglichkeiten

### Parameter im Quellcode
Im Quellcode können verschiedene Parameter angepasst werden. `RECEIVE_TIMEOUT` bestimmt die Zeit in Millisekunden, nach der ohne empfangenes Paket alle Ausgänge ausgeschaltet werden (Standard 2000 ms = 2 Sekunden). `senderMac[]` muss auf die tatsächliche MAC-Adresse des Senders gesetzt werden.

### Anpassungsmöglichkeiten
Für eine längere Timeout-Zeit kann `RECEIVE_TIMEOUT` auf 5000 erhöht werden (5 Sekunden statt 2). Bei anderen GPIO-Belegungen müssen die `outputPins` entsprechend angepasst werden. Die `outputNames` können für eine benutzerfreundlichere Ausgabe geändert werden. Bei abweichender Motoranzahl muss das Array `motorPairs` angepasst werden.

---

## 8. Sicherheitshinweise

### Elektrische Sicherheit
Das Projekt arbeitet mit 230V Motoren. Die Relais sind in einer bestehenden Schaltung installiert – an dieser Schaltung darf nur im spannungsfreien Zustand gearbeitet werden. Alle Arbeiten an der 230V Installation dürfen nur von Elektrofachkräften durchgeführt werden.

### Verhaltensicherheit
Die Sicherheitsfunktion gegen gleichzeitigen Links- und Rechtslauf eines Motors ist im Code implementiert und wurde getestet. Die Timeout-Funktion schaltet bei Verbindungsabbruch alle Motoren ab. Die Ausgänge sind beim Booten definiert LOW.

### Betriebssicherheit
Die Funktion sollte regelmäßig getestet werden, besonders nach Firmware-Updates. Bei Fehlverhalten ist das System sofort spannungsfrei zu schalten. Die MAC-Adresse des Senders muss eindeutig sein – es darf nur ein Sender verwendet werden.

---

## 📋 Abschließende Checkliste

### Hardware-Check
- ESP32 WROOM Board vorhanden
- ULN2803 korrekt verdrahtet
- 10kΩ Pull-down Widerstände an allen GPIOs
- GPIO 12 hat zwingend einen Pull-down nach GND
- 5V Netzteil angeschlossen
- Relais in bestehender Schaltung angeschlossen

### Software-Check
- PlatformIO-Projekt angelegt
- platformio.ini korrekt erstellt
- Sender-MAC-Adresse im Quellcode eingetragen
- Code kompiliert ohne Fehler
- Code erfolgreich hochgeladen

### Kommunikations-Check
- Eigene MAC-Adresse notiert
- Sender sendet an diese Adresse
- ESP-NOW initialisiert erfolgreich
- Pakete werden empfangen

### Funktions-Check
- Alle 6 Ausgänge schalten korrekt
- Motor 1 Links/Rechts funktionieren
- Motor 2 Links/Rechts funktionieren
- Motor 3 Links/Rechts funktionieren
- Gleichzeitige Ansteuerung wird verhindert
- Timeout-Funktion arbeitet korrekt
- Serielle Ausgabe zeigt alle Statusmeldungen

### Dokumentations-Check
- README.md im Projektordner vorhanden
- Sender-MAC-Adresse notiert
- Testprotokoll ausgefüllt

---

## 🔧 Wartungshinweise
Bei Fehlverhalten ist zuerst der serielle Monitor zu prüfen. Die Verbindung zum Sender sollte regelmäßig getestet werden. Die Kontakte der Relais können mit der Zeit verschleißen – bei Schaltproblemen Relais prüfen. Nach Firmware-Updates ist ein vollständiger Funktionstest durchzuführen.

---

## 🆘 Support
Bei Problemen sollte zuerst der serielle Monitor geöffnet werden – er gibt oft Hinweise auf Fehler. Dann sollte das Fehlersuche-Kapitel durchgegangen werden. Die Verdrahtung muss nochmals gegen den Schaltplan geprüft werden. Einzelkomponenten können getestet werden, zum Beispiel die Ausgänge direkt per `digitalWrite` ansteuern. Die MAC-Adresse des Senders sollte nochmals ausgelesen werden – das ist die häufigste Fehlerquelle.

---

**Dokumentation abgeschlossen** ✅