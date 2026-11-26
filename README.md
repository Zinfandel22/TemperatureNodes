# ESP-NOW Temperature Nodes

An environmental monitoring project using Seeed Studio XIAO ESP32-S3 boards and SHT40 temperature and humidity sensors. Up to four battery-powered nodes send measurements over ESP-NOW to a USB-powered hub, which is intended to publish them to Blynk.

> **Project status:** The hub uses official Blynk.Edgent for BLE-assisted provisioning, connection management, and OTA. Hub provisioning and Blynk connection have succeeded on one Wi-Fi network. Node pairing, sensor delivery, reset, and OTA still require hardware testing.

## Batch Data Publishing

The hub publishes all sensor data with **protocol-level batch grouping** via Blynk's `beginGroup()` / `endGroup()` API. This ensures all 18+ datastreams receive a shared server-side timestamp, improving data consistency for downstream consumers (e.g., the companion [Temperature Orbs](https://github.com/Zinfandel22/TemperatureOrbs) display project).

**Serial log output is significantly cleaner:**
```
[Hub] Node 1 report: 72.50 F, 45.20 %RH
[Blynk] Batch: 25 datastreams with shared timestamp
```

See [`QUICK_START.md`](QUICK_START.md) for details.

## How it works

All boards use the same firmware. A jumper sampled at boot selects the role:

| D3 (GPIO 4)      | Role |
| ---------------- | ---- |
| Connected to GND | Hub  |
| Open             | Node |

Restart the board after changing the role jumper. Node IDs are selected separately using D1 and D2.

- **Nodes** do not use Blynk or join the Wi-Fi network. They read their ID, SHT40, and battery voltage, attempt to send a measurement to the hub, then sleep for 30 minutes. The interval is measured from the start of deep sleep, not from a fixed clock schedule.
- **Hub** stays powered on, uses Blynk provisioning to join Wi-Fi, receives ESP-NOW packets asynchronously when its Wi-Fi station interface is running, and publishes the latest pending measurement for each node when connected to Blynk. It can also read a locally connected SHT40. Provisioning uses BLE, not a Wi-Fi access point.

The hub retains at most one pending packet per node; a newer packet replaces an older one. ESP-NOW packets and pairing are **unencrypted and unauthenticated**: a pairing window does not prevent impersonation or protect readings and control messages.

ESP-NOW uses the boards' Wi-Fi radios even though nodes do not connect to a Wi-Fi network. The hub and nodes must use the same radio channel; the hub's channel is determined by its Wi-Fi connection.

## Hardware

### Pin assignments

| XIAO pin | GPIO | Node                                                                   | Hub                                                  |
| -------- | ---- | ---------------------------------------------------------------------- | ---------------------------------------------------- |
| A0 / D0  | 1    | Battery voltage divider midpoint; optional pairing reset button to GND | Optional pairing / configuration reset button to GND |
| D1       | 2    | Node ID bit 0                                                          | Unused                                               |
| D2       | 3    | Node ID bit 1                                                          | Unused                                               |
| D3       | 4    | Open: select node                                                      | Connected to GND: select hub                         |
| D4       | 5    | SHT40 SDA                                                              | Optional local SHT40 SDA                             |
| D5       | 6    | SHT40 SCL                                                              | Optional local SHT40 SCL                             |
| 3V3      | -    | SHT40 power                                                            | Optional local SHT40 power                           |
| GND      | -    | Common ground                                                          | Common ground                                        |

Power the hub via USB-C. Nodes are intended to use a 3.7 V LiPo connected to the board's battery pads.

### Node ID jumpers

| Node ID | D1 (bit 0) | D2 (bit 1) |
| ------- | ---------- | ---------- |
| 1       | Open       | Open       |
| 2       | GND        | Open       |
| 3       | Open       | GND        |
| 4       | GND        | GND        |

The firmware enables pull-ups briefly to sample D1, D2, and D3, then changes the pins to inputs.

### Battery voltage measurement

For each node, connect two equal 220 kΩ resistors in series between battery positive and GND. Connect their midpoint to A0 and connect a 100 nF capacitor between A0 and GND. The firmware assumes a 2:1 divider. The divider draws approximately 9.5 µA at 4.2 V.

The reported battery percentage is a linear estimate from 3.20 V (0%) to 4.20 V (100%), clamped to that range; it is not a calibrated state-of-charge measurement.

## Blynk account requirement

A [Blynk](https://www.blynk.io/) account is required to provision the hub and view its measurements or use the pairing controls. The nodes communicate with the hub over ESP-NOW and do not need separate Blynk accounts or devices.

1. [Sign up for Blynk](https://blynk.cloud/dashboard/register) or sign in to [Blynk.Console](https://blynk.cloud/dashboard/login). See [Blynk's getting started guide](https://docs.blynk.io/en/getting-started) for account and app setup.
2. Create or access a device template in the console, configure the datastreams and event below, and set up dashboard widgets as needed. Copy that template's ID and name into [`src/hub/BlynkSettings.h`](src/hub/BlynkSettings.h); the values currently in this project refer to an existing template and may not be accessible from your account.
3. Install the Blynk IoT mobile app and sign in to an account with access to the template. Use the app to provision the hub as described below.

Blynk offers a free plan and paid plans; check [Blynk pricing](https://www.blynk.io/pricing) for current limits and available features.

## Blynk datastreams

Configure these virtual pins in the Blynk template:

| Source           | Temperature (°F), Double | Humidity (%RH), Double | Last contact (HH:MM AM/PM), String      | Battery voltage (V), Double | Battery level (%), Integer |
| ---------------- | ------------------------ | ---------------------- | --------------------------------------- | --------------------------- | -------------------------- |
| Node 1           | V0                       | V4                     | V10                                     | V20                         | V30                        |
| Node 2           | V1                       | V5                     | V11                                     | V21                         | V31                        |
| Node 3           | V2                       | V6                     | V12                                     | V22                         | V32                        |
| Node 4           | V3                       | V7                     | V13                                     | V23                         | V33                        |
| Hub local sensor | V8                       | V9                     | V14                                     | -                           | -                          |

Node contact time is the time the hub last received a sensor report; V14 shows the time of the hub's last local reading. Times are US Pacific, including daylight saving time, and do not include a date. The hub synchronizes its clock over the internet using NTP. `--:--` means there is no reading since the hub restarted, or that the reading arrived before the clock was synchronized; a new reading after synchronization supplies the time. After 90 minutes without a report (approximately three 30-minute wake periods), the node datastream shows `OFFLINE` followed by the last contact time, and the hub sends a `node_offline` event once until that node reports again. Offline status is checked about once a minute while connected to Blynk. If a node's SHT40 initialization or read fails, it sends `-999` for both temperature and humidity. The hub does not publish local readings if its sensor fails initialization; a subsequent read failure reports `-999`.

Create Blynk events with the codes `node_offline` and `low_battery` for alerts. The hub emits `low_battery` when a node reports a voltage **greater than 2.50 V and less than 3.40 V**.

| Control       | Virtual pin | Type / range | Action when sent `1`                        |
| ------------- | ----------- | ------------ | ------------------------------------------- |
| Reset pairing | V40         | Integer, 0–1 | Queue a pairing reset for all four node IDs |
| Start pairing | V41         | Integer, 0–1 | Open a two-minute pairing window            |
| Demo mode     | V100        | Integer, 0–1 | Enable simulated readings; `0` restores real monitoring |

Use push buttons that send `1` for the pairing controls. The hub writes `0` back after handling a `1`; this is not confirmation that all nodes have reset or paired. A pairing window requires the hub to be connected to both Wi-Fi and Blynk. For V100 use a switch that sends `1` when on and `0` when off. Configure its default as `0`; the hub requests the saved V100 value on each Blynk connection, including after a restart.

In demo mode the hub publishes independently randomized temperatures (65.0–80.0 °F) and humidity (35.0–65.0 %RH) for itself and all four nodes immediately and every 30 seconds. Node battery values are simulated as 3.8 V and 75%, and V10–V14 show `ONLINE` and a simulated contact time (`--:--` if the clock is not synchronized). These values do not reflect actual sensor, connection, or battery status. Pairing and real measurements continue, but low-battery and offline alerts are suppressed until demo mode is turned off.

When demo mode is turned off, the hub republishes the latest real readings and contact status; it checks for overdue offline alerts and reports low battery on any new node report received during the demo. If there is no real reading since the hub restarted, it publishes `-999` for that source's temperature and humidity, and for an unseen node's battery voltage and percentage; contact is `--:--`. Configure the relevant numeric datastreams to accept `-999`, and ensure the contact string datastreams and widgets can display `ONLINE` followed by the time. Real measurements may otherwise take up to 30 minutes to arrive; restart a node to make it report sooner.

## Configuration and build

PlatformIO installs Blynk 1.4.0 and the official Blynk.Edgent 0.9.0 release as declared in `platformio.ini`.

1. Install [PlatformIO](https://platformio.org/install) and open this project.
2. Set the template ID and name in [`src/hub/BlynkSettings.h`](src/hub/BlynkSettings.h) to match the Blynk Console template. Do not commit Wi-Fi credentials or auth tokens to a public repository.
3. Configure the datastreams above in the matching Blynk template.
4. Build the shared firmware:

   ```sh
   pio run -e firmware
   ```

5. Flash the same binary to the hub and each node, for example:

   ```sh
   pio run -e firmware -t upload
   ```

6. Connect the role and node ID jumpers as shown above, assigning a unique ID to each node, then restart each board. On the hub, use the Blynk IoT app's **Add new device** / **Find devices nearby** flow with Bluetooth enabled to provision Wi-Fi and the device auth token. The app must have access to the matching template.
7. Once the hub is connected, open a pairing window with V41 or a short click of the D0 button. Power up or restart each unpaired node during that window. Each node scans for up to 30 seconds, discovers the hub's MAC and channel, and saves them across deep sleep and restarts. If discovery fails, it sleeps for 30 minutes before trying again; open another window around its next wake or restart it during an open window.

### Resetting or changing the hub

Send `1` to V40 while the original hub is online. It saves a pending reset for each node ID and sends the request when that node next reports. A node clears its saved pairing upon receiving the request; missed requests are retried on later reports. Nodes then require an open pairing window to pair again. A node already asleep cannot receive a reset until it wakes, and an off node cannot receive one until powered. Serial output can help monitor progress; V40 is not a completion indicator.

To reset a node without the original hub, connect a momentary button between its D0 and GND. Hold it while restarting or powering the node, then release after the serial message `Release D0 to clear pairing` appears and before 20 seconds. The node clears its saved hub and searches for a new one immediately. Open the new hub's pairing window before releasing the button. A hold of 20 seconds or longer cancels the reset; if the button is still held at that point, the node sleeps. The button does not wake a sleeping node. D0 is shared with the battery divider; the firmware restores it for measurement after the button check. Test this button and battery measurement on the actual hardware.

Nodes save the Wi-Fi channel at pairing, so keep the router's 2.4 GHz channel fixed. If it changes, reset and re-pair the nodes using their buttons, or restore the original channel and use V40. Discovery scans the node's configured Wi-Fi country channel range; the hub's channel must be in that range.

The serial monitor uses 115200 baud. A momentary button between D0 and GND opens a pairing window on a short click. To reset Blynk configuration, hold it for more than 10 seconds and release before 20 seconds; this is separate from resetting node pairing. Wi-Fi scanning or a channel mismatch can interrupt ESP-NOW reception.

### Serial diagnostics

- The hub logs every Blynk virtual-pin write as `[Blynk] V<pin> = <value>`, including real and demo readings, contact status, battery values, and control resets. Blynk events are logged too. A log records an attempted write, not confirmation of cloud delivery.
- `[Node 1] Starting` means the board selected node mode; connect D3 to GND and restart if it should be a hub. Nodes report sensor setup, discovery or stored pairing, radio-level delivery acknowledgement, and sleep.
- On the hub, `Wi-Fi connected to access point; waiting for IP address` means association succeeded, not that an IP address has been assigned. A successful connection continues with `Wi-Fi connected and IP address acquired`, `CONNECTING CLOUD`, and `RUNNING` / `Blynk connected`.
- If the hub stays without an IP address, check that the router's 2.4 GHz network has DHCP enabled, available addresses, and no access filter blocking the hub. A disconnect reason of `202` means authentication failed; a later successful connection means a retry recovered.
- Pairing requires an active Wi-Fi and Blynk connection. `Waiting for node pairing` appears only while the two-minute window is open; activate it with V41 or a short D0 button click on the hub.
- Hub and node serial output includes SHT40 readings in °F and %RH; the hub also logs received node readings when publishing them. Both use `°F = °C × 1.8 + 32` (0 °C is 32 °F, and 32 °C is 89.6 °F). There is no firmware default of 32 °F or 89.6 °F: the hub publishes node temperatures only after receiving reports, and Blynk may display a previously stored or configured value before then. Compare the node, hub, and dashboard readings, check V10–V14 for contact times, and verify widget/datastream pin assignments, units, and ranges. Restart a node to sample immediately rather than waiting up to 30 minutes. If a sensor fails, check its power and I2C wiring; if the hub reads warmer than a nearby node, check sensor placement and heat from the powered board.

## Project layout

| Path                                                       | Purpose                                               |
| ---------------------------------------------------------- | ----------------------------------------------------- |
| [`platformio.ini`](platformio.ini)                         | Shared firmware build and dependencies                |
| [`src/main.cpp`](src/main.cpp)                             | Role selection, ESP-NOW, and Blynk publishing         |
| [`src/sensor_hal.cpp`](src/sensor_hal.cpp)                 | SHT40 and battery measurements                        |
| [`src/node_pairing_reset.cpp`](src/node_pairing_reset.cpp) | Node boot-time pairing reset                          |
| [`src/hub/BlynkSettings.h`](src/hub/BlynkSettings.h)       | Blynk template and firmware configuration             |
| [`src/hub/Interaction.h`](src/hub/Interaction.h)           | Edgent reset-button interaction                       |
| [`include/shared_protocol.h`](include/shared_protocol.h)   | ESP-NOW sensor and pairing/control packet definitions |

## Documentation

- **[QUICK_START.md](QUICK_START.md)** – Getting started guide with step-by-step setup
- **[SYSTEM_ARCHITECTURE.md](SYSTEM_ARCHITECTURE.md)** – Complete system design, data flow, and batching analysis
- **[TEST_PLAN.md](TEST_PLAN.md)** – Comprehensive test plan covering all scenarios, error cases, and validations
