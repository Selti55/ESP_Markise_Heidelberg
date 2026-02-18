# ESP32-Markisensteuerung

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

Zweiteiliges ESP32-Projekt zur kabellosen Steuerung einer Markise mit 6 Tastern. Das System besteht aus einem batteriebetriebenen Sender (Deep Sleep) und einem netzbetriebenen Empfänger mit 6 Relais-Ausgängen.

## 🏗️ Projektstruktur

## 📋 Systemübersicht

| Komponente | ESP32-Typ | Stromversorgung | Funktion |
|------------|-----------|-----------------|----------|
| **Sender** | ESP32-S2 oder ESP32-C3 | Batterie (3.3V) | 6 Taster abfragen, Deep Sleep, ESP-NOW senden |
| **Empfänger** | ESP32-WROOM-32D | 230V Netzteil | 6 Ausgänge schalten, optional Display |

### 🔧 Kommunikation
- **Protokoll:** ESP-NOW (Peer-to-Peer ohne WLAN-Router)
- **Reichweite:** ca. 30-50 Meter (je nach Umgebung)
- **Daten:** 1 Byte Tasterstatus (Bitmask)

## 🚀 Erste Schritte

### Voraussetzungen
- [Visual Studio Code](https://code.visualstudio.com/)
- [PlatformIO Extension](https://platformio.org/install/ide?install=vscode)

### Installation

1. **Repository klonen:**
   ```bash
   git clone https://github.com/IhrUsername/esp32-awning-control.git
   cd esp32-awning-control