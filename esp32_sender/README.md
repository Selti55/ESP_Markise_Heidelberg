# ESP32-Sender für Markisensteuerung

Batteriebetriebener Sender mit 6 Tastern zur Ansteuerung einer Markise. Optimiert für geringsten Stromverbrauch durch Deep Sleep. **Betrieb mit 18650 Li-Ion Akku (3.7V - 4.2V) und integrierter Ladeschaltung.**

## 🔋 Anforderungen an das Sender-Board

Da Sie einen **18650 Akku mit integrierter Ladeschaltung** wünschen, muss das Entwicklungsboard folgende Kriterien erfüllen:
- Integrierter 18650 Batteriehalter
- Integrierte Ladeschaltung (über USB-C oder Micro-USB)
- Überspannungs- und Tiefentladeschutz
- Effiziente Spannungsregelung für 3.3V (LDO oder Schaltregler)

## 🎯 Hardware-Empfehlung mit 18650-Support

### 🥇 Platz 1: **LILYGO T-Energy-S3** (ESP32-S3) ⭐ **Meine Top-Empfehlung**

| Eigenschaft | Beschreibung |
|-------------|--------------|
| **Modell** | LILYGO T-Energy-S3 |
| **ESP32-Typ** | ESP32-S3 (Dual-Core, 240 MHz) |
| **Batteriehalter** | Integrierter 18650 Halter auf der Rückseite |
| **Ladeschaltung** | HX6610S Laderegler, USB-C Ladung |
| **Schutzfunktionen** | Verpolungsschutz, Überladung, Tiefentladung |
| **Besonderheiten** | Battery-ADC an GPIO3 (Spannungsmessung), Qwiic/STEMMA Anschluss |
| **Deep Sleep** | Sehr geringer Stromverbrauch |
| **Preis** | ca. 17-20€ [citation:4] |
| **Bezug** | Aliexpress, LilyGO Shop, Amazon |

**Begründung:** Der T-Energy-S3 wurde speziell für 18650 Akkus entwickelt. Er hat einen fest integrierten Halter, keine wackeligen Kabel. Die Spannungsmessung ist bereits vorgesehen (GPIO3). Perfekt für Ihren Anwendungsfall [citation:4].

### 🥈 Platz 2: **WEMOS/LOLIN ESP32 mit 18650 Shield** (Klassischer ESP32)

| Eigenschaft | Beschreibung |
|--------------|-------------|
| **Modell** | WEMOS WiFi + Bluetooth Battery ESP32 |
| **ESP32-Typ** | ESP32-WROOM-32 (klassisch) |
| **Batteriehalter** | Integrierter 18650 Halter |
| **Ladeschaltung** | Ja (0.5A Ladestrom, 1A Ausgang) |
| **Schutzfunktionen** | Überladung, Tiefentladung |
| **Besonderheiten** | Programmierbare LED an GPIO16, Power-Schalter, alle Pins gebroakt |
| **Deep Sleep** | ca. 10-15 µA (mit Optimierung) |
| **Preis** | ca. 12-15€ [citation:2][citation:3] |
| **Bezug** | Amazon, AliExpress, Maker-Shops |

**Begründung:** Günstige, bewährte Lösung. Das Board ist eigentlich ein klassisches ESP32 Dev Board mit einem 18650 Shield kombiniert. Funktioniert zuverlässig, ist aber etwas größer [citation:2][citation:3].

### 🥉 Platz 3: **OLIMEX ESP32-DevKit-LiPo** (Professionelle Lösung)

| Eigenschaft | Beschreibung |
|--------------|-------------|
| **Modell** | OLIMEX ESP32-DevKit-LiPo |
| **ESP32-Typ** | ESP32-WROOM-32 |
| **Batterieanschluss** | JST PH 2.0 Anschluss für externen 18650 (separater Halter nötig) |
| **Ladeschaltung** | BL4054B Charger (bis 500mA) |
| **Schutzfunktionen** | Integrierter Schutz, optionale Batteriespannungsmessung |
| **Besonderheiten** | Open Source Hardware, sehr gute Dokumentation |
| **Deep Sleep** | ca. 20 µA |
| **Preis** | ca. 25-30€ [citation:10] |
| **Bezug** | Mouser, Farnell, OLIMEX Shop |

**Begründung:** Hochwertige, professionelle Lösung. Allerdings ohne festen 18650-Halter - Sie bräuchten einen externen Batteriehalter mit JST-Stecker. Dafür exzellente Dokumentation und Support [citation:10].

### Platz 4: **OLIMEX ESP32-S2-DevKit-LiPo-USB** (Modernere Alternative)

| Eigenschaft | Beschreibung |
|--------------|-------------|
| **Modell** | OLIMEX ESP32-S2-DevKit-LiPo-USB |
| **ESP32-Typ** | ESP32-S2 (Single-Core, USB-OTG) |
| **Batterieanschluss** | JST PH 2.0 Anschluss |
| **Ladeschaltung** | Integrierter Li-Po Charger |
| **Besonderheiten** | Native USB, sehr geringer Deep-Sleep-Strom (ca. 5-20 µA) |
| **Preis** | ca. 25-30€ [citation:7][citation:9] |
| **Bezug** | Mouser, Farnell, OLIMEX Shop |

**Begründung:** Wenn Sie wirklich das Optimum an Stromsparen wollen, ist der ESP32-S2 die bessere Wahl als der klassische ESP32. Allerdings auch hier: Kein fester 18650-Halter [citation:9].

## 📊 Vergleichstabelle

| Board | ESP32 Typ | 18650 Halter | Lader | Deep Sleep | GPIOs | Preis |
|-------|-----------|--------------|-------|------------|-------|-------|
| **LILYGO T-Energy-S3** | S3 | **✅ Fest integriert** | ✅ | Sehr gut | 32 | €€ |
| **WEMOS 18650 Board** | ESP32 | **✅ Fest integriert** | ✅ | Gut | Alle | € |
| **OLIMEX DevKit-LiPo** | ESP32 | ❌ (JST) | ✅ | Sehr gut | Alle | €€€ |
| **OLIMEX S2 DevKit** | S2 | ❌ (JST) | ✅ | **Hervorragend** | 27 | €€€ |

## 💡 Meine klare Kaufempfehlung

**Für Ihren Anwendungsfall: LILYGO T-Energy-S3**

Warum?
1. ✅ **Fester 18650-Halter** - Kein Gepfriemel mit externen Batteriefächern
2. ✅ **Integrierte Ladeschaltung** mit USB-C - Modern und schnell
3. ✅ **ESP32-S3** - Moderner Chip, gute Stromsparoptionen
4. ✅ **Battery-ADC vorhanden** - Sie können den Akkustand im Code auslesen (GPIO3)
5. ✅ **Qwiic/STEMMA Anschluss** - Falls Sie später Sensoren erweitern wollen [citation:4]

**Alternative für schmales Budget: WEMOS 18650 Board**
- Günstiger, ebenfalls mit festem Halter
- Aber: Älterer ESP32-Chip (etwas höherer Stromverbrauch) [citation:2][citation:3]

## 🔌 Anschluss der Taster am LILYGO T-Energy-S3

| Taster | GPIO | Interne Pull-ups |
|--------|------|------------------|
| Taster 1 | GPIO 1 | Ja (über INPUT_PULLUP) |
| Taster 2 | GPIO 2 | Ja |
| Taster 3 | GPIO 4 | Ja |
| Taster 4 | GPIO 5 | Ja |
| Taster 5 | GPIO 6 | Ja |
| Taster 6 | GPIO 7 | Ja |
| **Battery ADC** | GPIO 3 | - |

## ⚙️ Aktualisierte PlatformIO-Konfiguration für LILYGO T-Energy-S3

```ini
[env:lilygo_tenergy_s3]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200

; Board-spezifische Einstellungen für LILYGO T-Energy-S3
build_flags = 
    -DCORE_DEBUG_LEVEL=0
    -DBOARD_HAS_PSRAM
    -DARDUINO_RUNNING_CORE=0
    -DUSER_SETUP_LOADED
    -DLILYGO_TENERGY_S3

; Benötigte Bibliotheken
lib_deps = 
    espressif/ESP-NOW@^2.0.0
    ; Für Batteriemessung (optional)
    loboris/ESP32_Battery_ADC@^1.0.0

; Partition-Schema für optimierte Speichernutzung
board_build.partitions = default_16MB.csv
board_upload.flash_size = 16MB