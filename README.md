# ESP-NOW Temperature Nodes

An environmental monitoring project using Seeed Studio XIAO ESP32-S3 boards and SHT40 temperature and humidity sensors. Up to four battery-powered nodes send measurements over ESP-NOW to a USB-powered hub, which is intended to publish them to Blynk.

> **Project status:** The hub uses official Blynk.Edgent for BLE-assisted provisioning, connection management, and OTA. Hub provisioning and Blynk connection have succeeded on one Wi-Fi network. Node pairing, sensor delivery, reset, and OTA still require hardware testing.

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

## Blynk datastreams

Configure these virtual pins in the Blynk template:

| Source           | Temperature (°C), Double | Humidity (%RH), Double | Wake / sample count, Double | Battery voltage (V), Double | Battery level (%), Integer |
| ---------------- | ------------------------ | ---------------------- | --------------------------- | --------------------------- | -------------------------- |
| Node 1           | V0                       | V4                     | V10                         | V20                         | V30                        |
| Node 2           | V1                       | V5                     | V11                         | V21                         | V31                        |
| Node 3           | V2                       | V6                     | V12                         | V22                         | V32                        |
| Node 4           | V3                       | V7                     | V13                         | V23                         | V33                        |
| Hub local sensor | V8                       | V9                     | V14                         | -                           | -                          |

Node wake counts are retained across deep sleep; the hub's sample count resets when it restarts. If a node's SHT40 initialization fails, it sends `-999` for both temperature and humidity.

Create a Blynk event with the code `low_battery` for alerts. The hub emits it when a node reports a voltage **greater than 2.50 V and less than 3.40 V**.

| Control       | Virtual pin | Type / range | Action when sent `1`                        |
| ------------- | ----------- | ------------ | ------------------------------------------- |
| Reset pairing | V40         | Integer, 0–1 | Queue a pairing reset for all four node IDs |
| Start pairing | V41         | Integer, 0–1 | Open a two-minute pairing window            |

Use push buttons that send `1` for these controls. The hub writes `0` back after handling a `1`; this is not confirmation that all nodes have reset or paired. A pairing window requires the hub to be connected to both Wi-Fi and Blynk.

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

- `[Node 1] Starting` means the board selected node mode; connect D3 to GND and restart if it should be a hub. Nodes report sensor setup, discovery or stored pairing, radio-level delivery acknowledgement, and sleep.
- On the hub, `Wi-Fi connected to access point; waiting for IP address` means association succeeded, not that an IP address has been assigned. A successful connection continues with `Wi-Fi connected and IP address acquired`, `CONNECTING CLOUD`, and `RUNNING` / `Blynk connected`.
- If the hub stays without an IP address, check that the router's 2.4 GHz network has DHCP enabled, available addresses, and no access filter blocking the hub. A disconnect reason of `202` means authentication failed; a later successful connection means a retry recovered.
- Pairing requires an active Wi-Fi and Blynk connection. `Waiting for node pairing` appears only while the two-minute window is open; activate it with V41 or a short D0 button click on the hub.

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
