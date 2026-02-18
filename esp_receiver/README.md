
---

## README.md für den Empfänger (`esp32_receiver/README.md`)

```markdown
# ESP32-Empfänger für Markisensteuerung

Netzbetriebener Empfänger mit 6 Relais-Ausgängen zur Ansteuerung einer Markise. Empfängt Taster-Signale per ESP-NOW und schaltet entsprechende Ausgänge.

## 🎯 Hardware-Empfehlung

### Empfohlene ESP32-Typen:

| Modell | Vorteile | Nachteile | Empfehlung |
|--------|----------|-----------|------------|
| **ESP32-WROOM-32D** | • Viele GPIOs<br>• 2 Kerne für Multitasking<br>• Gute Display-Unterstützung | • Höherer Strom (egal, da Netzbetrieb) | ⭐ **Beste Wahl** |
| **ESP32-S3** | • Noch mehr GPIOs<br>• Schneller<br>• Gut für Displays | • Teurer<br>• Neuere Plattform | ⭐ **Für Display + SPI** |
| **ESP32-C3** | • Kompakt<br>• Günstig | • Weniger GPIOs | Nur für kleine Projekte |

**Meine Empfehlung: ESP32-WROOM-32D** (klassischer ESP32 Dev Board, z.B. `AZ-Delivery` oder `ESP32-DevKitC`)

### 🔌 Benötigte Komponenten
- ESP32-WROOM Entwicklungsboard
- 6x Relais-Module (5V, mit Optokoppler)
- Optional: I2C-Display (z.B. SSD1306 128x64)
- 5V Netzteil (für ESP32 und Relais)
- Transistoren (BC547) + Freilaufdioden (1N4007) falls Relais direkt angesteuert

## 📺 Optionale Display-Anbindung

### I2C-Display (SSD1306)