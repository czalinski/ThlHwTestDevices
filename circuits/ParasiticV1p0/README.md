# Parasitic Logger V1.0

A small logger for the voltage and current of a 12 V automotive battery,
intended for tracking down **parasitic (key-off) current draw** during HASS /
HALT and long-duration vehicle testing. Every 10 seconds it records the battery
voltage and the average current through an external shunt to a CSV file on a
micro-SD card, time-stamped by a DS3231 real-time clock. A serial terminal is
used to set the clock, watch live readings and calibrate.

```
2026-10-03 10:40:00,12.634,42.3
2026-10-03 10:40:10,12.634,41.9
(end-of-interval time, volts, mA)
```

## Hardware

| Ref | Part | Function |
|-----|------|----------|
| U2 | **PIC18F16Q41-I/SO** (see note) | Microcontroller |
| U3 | INA181A1IDBVR | Current-sense amplifier, gain 20 V/V |
| U1 | MSR7805-3.3CW | 12 V to 3.3 V switching regulator |
| J4 | DS3231 RTC module | Real-time clock (I²C) |
| J3 | Micro-SD socket module | Log storage (SPI) |
| J1 | 3-pin | +BATT / shunt sense / GND |
| J2 | 6-pin | PICkit ICSP header |
| J5 | 3-pin | UART terminal (TX / RX / GND), 3.3 V logic |
| — | RAM Meter 20M100A100 | External shunt, 100 A / 100 mV (1 mΩ) |

> **Microcontroller note:** the V1.0 schematic and PCB call for a
> PIC16F17144-I/SO. Its 7 KB of flash is too small for the firmware, so the
> board is built with a **PIC18F16Q41-I/SO** instead: same SOIC-20 footprint
> and identical pin assignment (64 KB flash, 4 KB RAM). The KiCad files have
> not been changed.

### Pin map (PIC18F16Q41)

| Pin | Port | Use |
|-----|------|-----|
| 3 | RA4 | Battery voltage divider (rework, below) |
| 4 | RA3 | MCLR / VPP |
| 6 / 7 | RC4 / RC3 | I²C SDA / SCL to DS3231 |
| 8 / 9 | RC6 / RC7 | UART TX / RX |
| 10–13 | RB7–RB4 | SD card CS / SCK / MOSI / MISO |
| 14 | RC2 | Internal op-amp output (not connected on the PCB) |
| 17 | RA2 | INA181 output (through 1 kΩ / 100 pF) |
| 18 / 19 | RA1 / RA0 | ICSP clock / data |

### Current measurement

The INA181 amplifies the shunt voltage 20× around a VDD/2 reference. In the
microcontroller, op-amp OPA1 adds a further inverting gain of about 14.3 around
an internal DAC also set to VDD/2, and the ADC samples the result against VDD.
Both VDD/2 points are ratiometric, so supply drift cancels; the true VDD is
measured every tick against the internal 2.048 V reference to set the scale.

* Resolution: about 3 mA per ADC count; each record averages about 8,000
  current conversions, so the logged value resolves well below 1 mA.
* Range: about ±5.5 A. Larger currents (cranking, charging) **saturate** —
  intended, since the logger is for parasitic draw.
* Sign: **positive = battery discharging**, negative = charging.

### Required rework on V1.0 boards

1. **Battery voltage divider (not on the PCB).** Hand-wire
   100 kΩ (1 %) from +BATT to RA4 (U2 pin 3), 12 kΩ (1 %) from RA4 to GND,
   and 100 nF from RA4 to GND. This gives a 9.333:1 ratio (full scale ≈ 30 V
   at VDD = 3.3 V) and draws about 0.1 mA from the battery. Other values work;
   set the ratio with `vcal`.
2. **ICSP VDD.** J2 pin 2 (the PICkit VDD sense pin) is not connected to
   +3.3 V; the MCLR 10 kΩ pull-up goes only to J2.2. Add a wire from J2 pin 2
   to +3.3 V. Without it the programmer cannot see target power. (The firmware
   uses an internal pull-up on MCLR, so the board runs either way.)
3. **C3** must be 22 µF, not 22 pF (noted on the schematic).
4. **U2**: fit a PIC18F16Q41-I/SO (see above).

### Wiring to the vehicle

The shunt goes in the battery **negative** lead (low side):

```
 Battery (+) ──[fuse 1 A]──────────────────────── J1.1  +BATT
 Battery (−) ──┬── shunt, battery-side terminal ─ J1.3  GND   (short, thick wire)
               │
             shunt
               │
 Vehicle ground strap ── shunt, load-side terminal ─ J1.2  sense
```

**Keep the J1.3 wire short and heavy.** On V1.0 the INA181's negative input is
the board ground, and the logger's own supply current (≈ 20 mA) returns through
the J1.3 wire. Every milliohm in that wire reads as about 20 mA of false
current. The `zero` calibration removes the steady part of this, which is why
it must be done with the logger wired exactly as it will be used. (V1.1 should
bring the INA181 IN− out on its own Kelvin pin.)

The logger runs from the battery it is measuring but its own current is **not**
included in the reading (it returns on the battery side of the shunt).

## Firmware

Source is in [`firmware/`](firmware/). It is plain C for the
[MPLAB XC8](https://www.microchip.com/en-us/tools-resources/develop/mplab-xc-compilers/xc8)
compiler and uses ChaN's [FatFs](http://elm-chan.org/fsw/ff/) (R0.16, in
`firmware/lib/fatfs`, BSD-style 1-clause license).

### Building

Needs XC8 (v4.00 used) and the Microchip PIC18F-Q device family pack
(1.31.492 used):

```
cd firmware
make                        # -> build/parasitic.hex
make XC8=/path/to/xc8-cc DFP=/path/to/PIC18F-Q_DFP/1.31.492
```

### Programming

MPLAB X dropped PICkit 3 support in v6.25. On Linux the PICkit 3 can be used
with the open-source **pk2cmd** ([jaka-fi/pk2cmd](https://github.com/jaka-fi/pk2cmd)
with the device file from [cjacker/pk2cmd-minus](https://github.com/cjacker/pk2cmd-minus)),
after loading the PICkit 3 "scripting" firmware (PK3OSV023202.hex). With the
board powered from 12 V:

```
make flash                  # pk2cmd -PPIC18F16Q41 -W -R -F build/parasitic.hex -M -Y
```

`-W` tells pk2cmd the board supplies its own power; `-R` releases MCLR
afterwards so the new firmware starts. To return a PICkit 3 to
MPLAB mode, hold its button while plugging it in and connect with MPLAB X ≤ 6.20.

## Using the logger

### SD card

Any micro-SD card formatted **FAT32** (cards up to 32 GB come that way; larger
cards are exFAT and must be reformatted as FAT32). Records go to one file per
day in the root directory, named from the RTC date, e.g. `20261003.CSV`. A new
file starts with a `time,volts,mA` header. Each record is flushed to the card
as it is written, so the card can be removed at any time with at most one
record lost; `log off` first is cleaner. If the card is missing or fails, the
logger retries every 30 s.

At 10 s per record a day is about 310 KB.

### Terminal

Connect a 3.3 V USB-serial adapter to J5 (TX → adapter RX, RX → adapter TX,
GND) at **115200 8N1**, e.g. `picocom -b 115200 /dev/ttyUSB0`.

| Command | Action |
|---------|--------|
| `help` | List commands |
| `time` | Show the RTC time |
| `time 2026-10-03 14:05:00` | Set the RTC (24-hour) |
| `status` | Last reading, supply voltage, log file, free space, calibration |
| `live on` / `live off` | Echo each record to the terminal (default on) |
| `log on` / `log off` | Start / stop SD logging (setting is remembered) |
| `zero` | Current zero calibration (see below) |
| `vcal 12.634` | Battery voltage calibration to a meter reading in volts |
| `ical 1000` | Current gain calibration to a known current in mA |
| `defaults` | Restore default calibration |

Calibration commands take effect at the end of the next 10 s record and are
stored in EEPROM.

### Calibration

1. **Zero (required).** Wire the logger as it will be used, then disconnect the
   vehicle ground strap from the shunt so no current flows. Wait for a record,
   type `zero`, and wait for `cal saved`. Reconnect.
2. **Voltage.** Measure the battery with a good meter and enter `vcal <volts>`.
3. **Current gain (optional).** With a known steady load (e.g. 1.000 A from a
   bench supply through the shunt) enter `ical <mA>`. The default assumes the
   nominal 1 mΩ shunt and amplifier gains, typically within a few percent.

## Status

* Firmware written and compiled (25 KB of 64 KB flash, 1.2 KB of 4 KB RAM).
* **Not yet tested on hardware**, and pk2cmd programming of the PIC18F16Q41
  with a PICkit 3 is not yet confirmed.

## Changes wanted for V1.1

* Change U2 to PIC18F16Q41-I/SO in the schematic and BOM.
* Add the battery voltage divider and filter capacitor to RA4.
* Connect J2 pin 2 to +3.3 V.
* Bring INA181 IN− out on a separate J1 pin (4-pin connector) for Kelvin
  sensing at the shunt.
* Fix the DS3231 symbol pin names (pin graphics are right; names were copied
  from the SD-card symbol).
* C3 = 22 µF.

## License

The design files, firmware and documentation are released under the
[MIT License](../../LICENSE). The bundled FatFs library in
`firmware/lib/fatfs` keeps its own BSD-style license
([LICENSE.txt](firmware/lib/fatfs/LICENSE.txt)).
