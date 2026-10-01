# wbec – aus deiner Heidelberg Wallbox wird eine smarte Ladestation

**WLAN & Smart Charging für die Heidelberg (Amperfied) Energy Control – PV-Überschussladen, komplett lokal, ohne Cloud.**

[![Downloads](https://img.shields.io/github/downloads/steff393/wbec/total?color=blue&style=flat-square)](https://github.com/steff393/wbec/releases)
[![Stars](https://img.shields.io/github/stars/steff393/wbec?style=flat-square)](https://github.com/steff393/wbec/stargazers)
[![Lizenz](https://img.shields.io/github/license/steff393/wbec?style=flat-square)](LICENSE)

Die Heidelberg Energy Control ist eine hochwertige Ladestation „made in Germany" – ab Werk bietet sie aber **nur eine Modbus-RTU-Schnittstelle** und keinerlei Netzwerkanbindung. Damit lädt sie stur mit einem fest eingestellten Strom, egal was deine PV-Anlage gerade liefert.

**wbec** gibt ihr WLAN und macht aus der soliden, aber „stummen" Wallbox eine vollwertige smarte Ladestation: Sie tankt dein Auto mit deinem **eigenen Sonnenstrom**, fügt sich in dein Smart Home ein – und läuft dabei **vollständig bei dir zu Hause**. Keine Cloud, kein Konto, kein Abo, kein Tracking. Deine Daten bleiben deine Daten.

> **Vom Hersteller empfohlen:** wbec wird im offiziellen [Amperfied-Blog von Heidelberg](https://web.archive.org/web/20260116192803if_/https://www.amperfied.de/2022/11/21/wbec-fuer-heidelberg-wallbox-energy-control-blog/) vorgestellt.
> Im Einsatz und bewährt seit 2021 · entwickelt und gefertigt in Deutschland.

🔗 **[Homepage](https://wbec.ibferstl.de/)** · **[Modelle & Bestellung](https://wbec.ibferstl.de/modelle/)** · **[Anleitungen](https://wbec.ibferstl.de/anleitungen/)** · **[Wiki](https://github.com/steff393/wbec/wiki)**

---

## Was wbec kann

- **PV-Überschussladen.** Statt teuren Netzstroms lädt dein Auto den Strom, den deine PV-Anlage gerade übrig hat. Der Ladestrom wird dynamisch an den Überschuss angepasst – [so rechnet sich das](https://wbec.ibferstl.de/pv-laden/#rechner).
- **Dein Smart Home, deine Regeln.** Anbindung an Home Assistant, ioBroker, openWB, EVCC, Solaranzeige, Loxone, openHAB, IP-Symcon, Node-RED – per **MQTT, HTTP oder offener JSON-API**. Für Fremdsteuerungen zusätzlich go-eCharger-Emulation.
- **Viele Wechselrichter & Smartmeter.** u.a. SolarEdge, Fronius, Kostal, SMA, Huawei, Victron, GoodWe, Shelly 3EM, powerfox – die vollständige getestete Liste steht auf der [Homepage](https://wbec.ibferstl.de/pv-laden/#kompatibel). Vorab prüfen kannst du die Erreichbarkeit mit dem kostenlosen Tool [wbecModbus](https://github.com/steff393/wbecModbus).
- **Lokal & offen.** Läuft ausschließlich in deinem Netz – erreichbar unter `http://wbec.local/`. Kein Cloud-Dienst, der abgeschaltet werden kann, keine Registrierung. Volle Datenhoheit.
- **Mehrere Ladepunkte.** Steuert bis zu 16 Wallboxen am Modbus-Bus (bis zu 8 openWB-Ladepunkte), inkl. lokalem Lastmanagement für bis zu 2 Wallboxen.
- **RFID-Freischaltung** der Wallbox per Karte/Chip (optionale Zusatz-Hardware, s. Wiki).
- **Sparsam & wartungsarm.** Unter 1 W Verbrauch. Software-Updates bequem über den Browser (Over-the-Air).

Die aktuellen und neuesten Funktionen findest du unter [→ Funktionen](https://wbec.ibferstl.de/funktionen/).

---

## Zwei Wege zu wbec

**1. Selbst bauen (Open Source).**
Der komplette Quellcode für den ESP8266 liegt hier offen. Freuen würde ich mich über einen ⭐ hier auf GitHub.

**2. Fertig kaufen – geprüft, sofort einsatzbereit, mit Support.**
Nicht jeder will basteln. Die fertigen Module kommen **einzeln an der Heidelberg Energy Control getestet**, fertig programmiert, mit 12-seitiger Anleitung und persönlichem Support per E-Mail. Die aktuellen ESP32-Modelle bieten zudem Funktionen, die die freie Version nicht hat (z.B. Touch-Display, Ladelog, Zeitladen, §14a EnWG). Mit dem Kauf unterstützt du direkt die Weiterentwicklung.

➡️ **[Modelle & Preise ansehen](https://wbec.ibferstl.de/modelle/)**

---

## Beispiele

Einfaches Web-Interface (für jeden Browser, Smartphone wie PC) unter `http://wbec.local/` – hier die neue Oberfläche der ESP32-Modelle (die freie ESP8266-Version nutzt die klassische Oberfläche):

<p align="center">
  <img src="https://wbec.ibferstl.de/assets/img/ui/uebersicht.webp" width="800" alt="Übersicht der wbec-Oberfläche mit Wallbox-Status, Verlauf, Ladestrom, Lademodus und Zeitsteuerung">
</p>

Offene JSON-API unter `http://wbec.local/json` – ein Auszug:

```jsonc
{
  "wbec": { "version": "v0.3.0", "bldDate": "2021-06-10" },
  "box": [
    {
      "busId":   1,     // Modbus-Bus-ID (per DIP-Schalter)
      "chgStat": 2,     // Ladezustand
      "currL1":  0,     // Strom L1 (in 0,1 A)
      "power":   0,     // Leistung L1+L2+L3 (in VA)
      "energyI": 0.003, // Energie seit Installation (in kWh)
      "currLim": 130,   // aktueller Ladestrom-Grenzwert (in 0,1 A)
      "resCode": "0"    // Ergebnis der letzten Modbus-Nachricht (0 = ok)
    }
  ],
  "wifi": { "rssi": -76, "signal": 48, "channel": 11 }
}
```

Maximalen Ladestrom setzen (hier: 12 A auf der Box mit Bus-ID 1):

```text
http://wbec.local/json?currLim=120
http://wbec.local/json?currLim=60&id=2   // 6 A auf der Box mit id=2
```

---

## Kontakt & Support

Fragen, Probleme oder unsicher, ob dein Setup passt? Schreib mir einfach eine Mail an **wbec393@gmail.com** – ich helfe gerne weiter. Ein Blick ins [Wiki](https://github.com/steff393/wbec/wiki) beantwortet die häufigsten Fragen bereits vorab.

Weitere Projekte von mir: [wbecModbus](https://github.com/steff393/wbecModbus) (Modbus-Kompatibilitätscheck) und [selbst-ableser.de](https://selbst-ableser.de) (Heizkostenverteiler und Wasserzähler selbst verwalten).

---

## Danksagung

wbec nutzt und passt folgende Open-Source-Projekte an:
[modbus-esp8266](https://github.com/emelianov/modbus-esp8266) ·
[ESP Async WebServer](https://github.com/me-no-dev/ESPAsyncWebServer) ·
[ArduinoJson](https://github.com/bblanchon/ArduinoJson) ·
[PubSubClient](https://github.com/knolleary/PubSubClient) ·
[NTPClient](https://github.com/arduino-libraries/NTPClient) ·
[MFRC522](https://github.com/miguelbalboa/MFRC522) ·
[RTCVars](https://github.com/highno/RTCVars) ·
[arduinoWebSockets](https://github.com/Links2004/arduinoWebSockets) ·
[WiFiManager](https://github.com/tzapu/WiFiManager) ·
[AsyncElegantOTA](https://github.com/ayushsharma82/AsyncElegantOTA) ·
Web-Interface nach [RandomNerdTutorials](https://RandomNerdTutorials.com)

Besonderer Dank an die frühen Tester und Unterstützer: mli987, profex1337, Clanchef und viele mehr!

---

## Projekt unterstützen

wbec gefällt dir? Dann gib dem Projekt einen [⭐ auf GitHub](https://github.com/steff393/wbec/stargazers) – oder mach es wie über 1000 begeisterte Kunden und [hol dir ein fertiges Modul](https://wbec.ibferstl.de/modelle/).  
