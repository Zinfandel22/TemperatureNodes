# Quick Start Guide

## Prerequisites

1. **Hardware**:
   - Seeed Studio XIAO ESP32-S3 (one hub + up to four nodes + optional Orbs display)
   - SHT40 temperature/humidity sensors
   - 3.7V LiPo batteries for nodes (optional, can use USB for testing)
   - Resistors and capacitors per [hardware section](README.md#hardware)

2. **Software**:
   - [PlatformIO](https://platformio.org/install) installed
   - [Blynk account](https://blynk.cloud/dashboard/register)

3. **Network**:
   - 2.4 GHz Wi-Fi (5 GHz not supported by ESP32)

## Setup Steps

### 1. Configure Blynk Hub Template

1. Sign in to [Blynk.Console](https://blynk.cloud/dashboard/login)
2. Create a new device template for the **Hub** with these datastreams:
   - V0–V3: Node 1–4 temperatures (double)
   - V4–V7: Node 1–4 humidity (double)
   - V8–V9: Hub local temp/humidity (double)
   - V10–V13: Node 1–4 contact time (string)
   - V14: Hub local sensor contact time (string)
   - V20–V23: Node 1–4 battery voltage (double)
   - V30–V33: Node 1–4 battery percentage (integer)
   - V40: Reset pairing (integer)
   - V41: Start pairing (integer)
   - V100: Demo mode toggle (integer)

3. Copy the **template ID** and **template name** from the console

### 2. Configure Hub Firmware

1. Open `src/hub/BlynkSettings.h` in this project
2. Update `BLYNK_TEMPLATE_ID` and `BLYNK_TEMPLATE_NAME` with your template values
3. **Do not commit credentials** to version control

### 3. Build and Flash Hub

```sh
# Build the firmware
pio run -e firmware

# Flash to hub (connected via USB, D3 jumper connected to GND)
pio run -e firmware -t upload

# Flash to each node (D3 jumper open, D1/D2 set for unique node ID)
pio run -e firmware -t upload
```

### 4. Configure Hub Hardware

Connect jumpers and components per the [hardware table](README.md#pin-assignments):
- **D3 to GND**: Selects hub mode
- **D1 & D2**: Node ID bits (only needed for nodes)
- **A0 battery divider**: Two 220 kΩ resistors for voltage measurement
- **D4 & D5**: SHT40 I2C (optional local sensor)

Restart the board after changing jumpers.

### 5. Provision Hub to Blynk

1. Launch the Blynk IoT mobile app
2. Go to **Add new device** → **Find devices nearby**
3. Enable Bluetooth and select your hub
4. Follow the in-app wizard to connect to Wi-Fi
5. The app installs the device auth token automatically

### 6. Test Serial Connection

Connect the hub via USB and open the serial monitor at **115200 baud**:

```
Expected output:
[Hub] Starting
[Hub] Wi-Fi connecting...
[Hub] Wi-Fi connected and IP address acquired
[Hub] Blynk RUNNING
[Hub] Node 1 report: 72.50 F, 45.20 %RH
[Blynk] Batch: 25 datastreams with shared timestamp
```

### 7. Set Up Pairing Windows for Nodes

**First-time pairing**:
1. Send `1` to V41 (or click D0 button on hub) to open pairing window
2. Power or restart each unpaired node within 2 minutes
3. Node LED indicates pairing progress (varies by node firmware)
4. Node scans for hub, discovers MAC address and Wi-Fi channel
5. If unsuccessful, restart node and try again

**Pairing takes ~30 seconds per node**; each node wakes periodically to report or receive commands.

### 8. Verify Data Flow

In the Blynk app dashboard:
- ✅ All V0–V9 show temperature and humidity values
- ✅ V10–V14 show "ONLINE" and a recent contact time
- ✅ V20–V23 show battery voltages (3.0–4.2 V)
- ✅ V30–V33 show battery percentages (0–100%)

### 9. Test Demo Mode (Optional)

Send `1` to V100 to enable demo mode (simulated readings). Send `0` to restore real measurements.

## Orbs Display Setup (Optional)

If you have [Temperature Orbs](https://github.com/Zinfandel22/TemperatureOrbs) display devices, connect them to display hub data:

### 1. Create Orbs Blynk Template

1. In Blynk Console, create a new template for **Orbs** with these datastreams:
   - V0–V9: Display values (double/string) – Mirror hub data
   - V10–V14: Contact times (string) – Mirror hub data
   - V30–V33: Battery percentages (integer) – Mirror hub data
   - **V99**: Hub connection status (string, 0–64 chars)
   - **V101**: Hub AUTH Token (string, 0–128 chars)

2. Copy the **Orbs template ID** and **template name**

### 2. Configure Orbs Firmware

1. Update Orbs firmware with its own template ID and name
2. Build and flash to Orbs ESP32-S3 devices
3. Use Blynk app to provision each Orbs device

### 3. Enter Hub's AUTH Token into Orbs

1. Get Hub's AUTH token:
   - Sign in to Blynk Console
   - Find your Temperature Nodes Hub device
   - Device info → Copy **Auth Token**

2. Enter token into each Orbs device:
   - Open Blynk app → Orbs device
   - Settings (gear icon) → V101 "Hub AUTH Token"
   - Paste the Hub's token
   - Save/Confirm

3. Verify connection:
   - Orbs will fetch hub data every 10 seconds
   - V99 "Hub Status" should show "ONLINE"
   - All 5 displays update with hub values

### 4. Display Updates

The Orbs will now:
- Fetch all hub sensor data via Blynk External API
- Display temperatures, humidity, battery, and contact times
- Update synchronized with hub's batched publishes
- Show "ONLINE" / "OFFLINE" / "ERROR" status

**To switch Orbs to a different hub**:
- Just update V101 with the new Hub's token
- No firmware reflash needed—changes immediately

## Troubleshooting

| Problem | Cause | Solution |
|---------|-------|----------|
| Hub not provisioning | Bluetooth off or wrong device selected | Restart app, enable Bluetooth, re-scan |
| Hub not finding Wi-Fi | Wrong network, 5 GHz only | Confirm 2.4 GHz network is available |
| Nodes not pairing | Pairing window closed or hub offline | Open V41 and restart node within 2 min |
| No sensor readings | SHT40 I2C issue | Check D4/D5 wiring and power (3V3) |
| Battery stuck at -999 | Divider misconfigured | Verify 220 kΩ resistors and A0 connection |
| Serial spam with V writes | Old firmware | Rebuild and re-upload latest binary |
| Orbs show "ERROR" in V99 | Hub token not entered | Enter Hub's AUTH token in Orbs V101 |
| Orbs show "OFFLINE" | Hub token incorrect or hub disconnected | Verify token is correct, check hub connection |
| Orbs don't update | Hub not publishing data | Check hub serial log for batch messages |

## Next Steps

- **System architecture & data flow**: See [SYSTEM_ARCHITECTURE.md](SYSTEM_ARCHITECTURE.md)
- **Integrating with Temperature Orbs display**: Visit [Temperature Orbs](https://github.com/Zinfandel22/TemperatureOrbs) project
- **Hardware schematics & pin details**: See [README.md hardware section](README.md#hardware)

