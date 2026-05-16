# Geplante Arbeiten

Stand: 16.05.2026

## 1. Batteriespannungsüberwachung

Die Batteriespannung soll später über einen freien ADC-Pin des ESP32-C3 gemessen und auf dem TFT-Display angezeigt werden. Die Umsetzung wird erst begonnen, nachdem die Hardware entsprechend erweitert wurde.

### Geplante Hardware

- Akku-Plus wird direkt am Lötpunkt des Akkuanschlusses abgegriffen.
- `IO1` ist aktuell unbelegt und soll als ADC-Eingang verwendet werden.
- Zwischen Akku-Plus und `IO1` wird ein Spannungsteiler vorgesehen:
  - `BAT+ -> 1 MOhm -> IO1 -> 1 MOhm -> GND`
- Optional wird ein Kondensator mit `100 nF` von `IO1` nach `GND` ergänzt, um die ADC-Messung zu beruhigen.
- Bei einem 1S-LiPo mit maximal `4,2 V` liegen durch den Teiler etwa `2,1 V` an `IO1` an. Das bleibt im sicheren Messbereich des ADC.
- Der Spannungsteiler zieht bei vollem Akku ungefähr `2,1 uA` Dauerstrom. Das ist für dieses Projekt voraussichtlich akzeptabel.

### Geplante Software

- In `miniapp/src/main.cpp` eine Funktion zur Messung der Batteriespannung ergänzen, zum Beispiel `readBatteryVoltage()`.
- ADC auf `IO1` konfigurieren und mehrere Messwerte mitteln.
- Gemessene ADC-Spannung mit dem Teilerfaktor `2` auf die echte Akkuspannung zurückrechnen.
- Anzeige auf dem TFT ergänzen, wahlweise als:
  - Spannung in Volt,
  - Batteriesymbol,
  - Prozentwert nach einfacher LiPo-Kennlinie.
- Optional eine Warnschwelle anzeigen, zum Beispiel unter `3,5 V` oder `3,4 V`.

### Offene Punkte vor der Umsetzung

- Hardwareumbau abschließen und Messpunkt `BAT+` prüfen.
- Mit Multimeter kontrollieren, welche Spannung an `IO1` bei vollem oder teilgeladenem Akku anliegt.
- Danach erst Software ändern und den ADC-Wert gegen die reale Multimeter-Messung kalibrieren.
