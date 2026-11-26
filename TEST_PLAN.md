# Test Plan: Temperature Nodes & Orbs System

## Quick Navigation

📌 **[Test Overview](#overview)** | 🏗️ **[Hub Tests](#part-1-hub-setup--configuration-tests)** | 📡 **[Node Tests](#part-2-node-pairing--operation-tests)** | 🌐 **[Blynk Tests](#part-3-blynk-publishing--batch-tests)** | 📺 **[Orbs Tests](#part-4-orbs-display-tests)** | ⚠️ **[Error Cases](#part-5-error--edge-cases)** | 🔒 **[Security](#part-6-configuration--security-tests)** | ✅ **[Checklist](#checklist-summary)**

---

## Overview

This test plan validates all device behaviors, configurations, data flows, and error conditions for the Temperature Nodes hub, four remote nodes, and Temperature Orbs display system.

**Test Environment**:
- 1 Hub ESP32-S3 with local SHT40 sensor
- 4 Node ESP32-S3 devices with SHT40 sensors (or simulated)
- 1–5 Orbs ESP32-S3 displays (optional)
- Blynk account with hub and orbs templates
- 2.4 GHz Wi-Fi network
- Serial monitor (115200 baud) on each device
- Blynk Console and mobile app

**Total Test Cases**: 35  
**Critical Tests**: 7 (marked ⭐)  
**Expected Duration**: 2-4 hours for full validation

---

## Part 1: Hub Setup & Configuration Tests

### HUB-1: Template Configuration Validation

**Objective**: Verify Blynk template has all required datastreams

**Prerequisites**:
- Blynk account created
- Blynk Console accessible

**Steps**:
1. Sign in to Blynk Console
2. Create new template: "Temperature Hub"
3. Add all datastreams per QUICK_START.md section 1
4. Copy template ID and name

**Expected Results**:
- ✅ Template created successfully
- ✅ All 10 datastreams present (V0-V3, V4-V7, V8-V9, V10-V14, V20-V23, V30-V33, V40-V41, V100)
- ✅ Correct data types: double, integer, string
- ✅ Template ID and name copied correctly

**Pass/Fail**: PASS if all datastreams listed correctly

---

### HUB-2: Firmware Configuration & Build

**Objective**: Verify firmware compiles and deploys to hub

**Prerequisites**:
- PlatformIO installed
- Repository cloned
- Template ID and name from HUB-1

**Steps**:
1. Open `src/hub/BlynkSettings.h`
2. Update `BLYNK_TEMPLATE_ID` and `BLYNK_TEMPLATE_NAME`
3. Run: `pio run -e firmware`
4. Connect hub ESP32 via USB (D3 jumper to GND)
5. Run: `pio run -e firmware -t upload`
6. Open serial monitor (115200 baud)

**Expected Results**:
- ✅ Compilation succeeds (0 errors, 0 warnings)
- ✅ Binary size < 50% flash (file shows ~1.4 MB / 3.8 MB)
- ✅ Upload succeeds
- ✅ Serial output shows: `[Hub] Starting`

**Pass/Fail**: PASS if binary uploads and shows hub startup message

---

### HUB-3: BLE Provisioning (Blynk Edgent)

**Objective**: Verify hub can be provisioned via Blynk app

**Prerequisites**:
- Hub running latest firmware
- Blynk IoT app installed
- Bluetooth enabled on phone
- 2.4 GHz Wi-Fi available (DHCP enabled)

**Steps**:
1. Open Blynk app → Add device → Find devices nearby
2. Enable Bluetooth on phone
3. Select temperature hub from list
4. Follow in-app wizard to enter Wi-Fi SSID and password
5. Complete provisioning
6. Watch serial monitor during process

**Expected Results**:
- ✅ App scans and finds hub via BLE
- ✅ App successfully enters Wi-Fi credentials
- ✅ Hub associates to Wi-Fi
- ✅ Serial shows: `[Hub] Wi-Fi connected and IP address acquired`
- ✅ Serial shows: `[Hub] RUNNING`
- ✅ App shows device "online"

**Pass/Fail**: PASS if hub connects to Wi-Fi and shows RUNNING

---

### HUB-4: Local SHT40 Sensor Reading

**Objective**: Verify hub's local SHT40 sensor initializes and reads

**Prerequisites**:
- Hub provisioned and running
- SHT40 wired to D4 (SDA) and D5 (SCL), 3V3 power
- 100 nF capacitor on SDA
- Blynk connection active

**Steps**:
1. Watch serial output for sensor initialization
2. Wait 30-60 seconds for hub update cycle
3. Check serial for temperature/humidity readings
4. Check Blynk app: V8 (temp), V9 (humidity) should have values

**Expected Results**:
- ✅ Serial shows: `[Hub] SHT40 initialized`
- ✅ Periodic logs show: `[Hub] Hub local: XX.XX F, YY.YY %RH`
- ✅ Blynk V8/V9 show values (not -999)
- ✅ Values are reasonable (50-85°F, 20-70%RH for typical environment)

**Pass/Fail**: PASS if sensor reads and publishes to Blynk

---

## Part 2: Node Pairing & Operation Tests

### NODE-1: Node Firmware Flash & Role Selection

**Objective**: Verify node firmware deploys and node role is selected

**Prerequisites**:
- Firmware built successfully (HUB-2)
- Node 1 ESP32 connected via USB

**Steps**:
1. Connect Node 1 ESP32 (D3 open, D1 open, D2 open = ID 1)
2. Run: `pio run -e firmware -t upload`
3. Open serial monitor (115200 baud) on node
4. Watch startup messages

**Expected Results**:
- ✅ Upload succeeds
- ✅ Serial shows: `[Node 1] Starting`
- ✅ Serial shows: `[Node] Reading sensors...`
- ✅ No "Connected to GND" or hub mode messages

**Pass/Fail**: PASS if node identifies as node and reads sensors

---

### NODE-2: Node Sensor Reading (SHT40 & Battery)

**Objective**: Verify node reads SHT40 and battery voltage

**Prerequisites**:
- Node 1 running
- SHT40 wired to D4 (SDA) and D5 (SCL), 3V3 power
- Battery voltage divider wired to A0 (optional for initial test)

**Steps**:
1. Watch serial output on node
2. Look for temperature, humidity, battery values
3. If divider present, verify battery percentage (0-100%)
4. If no divider, should show -999 for battery

**Expected Results**:
- ✅ Serial shows: `[Node 1] Temp: XX.XX F, Humidity: YY.YY %RH`
- ✅ Serial shows: `[Node 1] Battery: Z.ZZ V, PP%` (if divider present)
- ✅ Serial shows: `[Node 1] Battery: -1.00 V, -999%` (if no divider)
- ✅ No sensor errors (no "failed to initialize")

**Pass/Fail**: PASS if sensor values read without errors

---

### NODE-3: Node Deep Sleep Cycle

**Objective**: Verify node sleeps for ~30 minutes and wakes correctly

**Prerequisites**:
- Node 1 running with serial connected

**Steps**:
1. Watch serial output
2. After sensor read, look for sleep message
3. Note timestamp when sleep begins
4. Wait 30 seconds, check if node is still sleeping (no output)
5. Force wake by holding D0 for <1 second (if pairing button connected)

**Expected Results**:
- ✅ Serial shows: `[Node 1] Entering deep sleep for 30 minutes`
- ✅ No serial output for ~30 seconds (sleeping)
- ✅ Waking works if button pressed
- ✅ No unexpected resets during sleep

**Pass/Fail**: PASS if node sleeps and wakes as expected

---

### NODE-4: Hub Pairing Window & Node Discovery

**Objective**: Verify node discovers hub during pairing window

**Prerequisites**:
- Hub running and connected to Blynk
- Node 1 flashed and ready
- Serial connected to both devices

**Steps**:
1. On hub serial, send command or click V41 button to open pairing window
2. Watch hub serial: should show `[Hub] Waiting for node pairing`
3. Power-on or restart Node 1 within 2 minutes
4. Watch Node 1 serial for discovery messages
5. Watch hub serial for pairing acknowledgment

**Expected Results**:
- ✅ Hub serial shows: `[Hub] Pairing window open (120 seconds)`
- ✅ Node serial shows: `[Node 1] Scanning for hub...`
- ✅ Node finds hub MAC and channel: `[Node 1] Found hub: MAC XX:XX:XX:XX:XX:XX, Ch N`
- ✅ Node shows: `[Node 1] Hub pairing successful`
- ✅ Hub shows: `[Hub] Node 1 paired`
- ✅ Pairing closes after 2 minutes (if node found before)

**Pass/Fail**: PASS if node discovers and pairs with hub

---

### NODE-5: ESP-NOW Packet Delivery

**Objective**: Verify node sends data to hub via ESP-NOW

**Prerequisites**:
- Node 1 paired
- Hub running and Blynk connected

**Steps**:
1. Node goes through read/sleep cycle
2. When node wakes, watch serial on both devices
3. Node should send ESP-NOW packet to hub
4. Hub should log receipt

**Expected Results**:
- ✅ Node serial shows: `[Node 1] Sending ESP-NOW to hub...`
- ✅ Node shows: `[Node 1] ESP-NOW sent (ACK received)`
- ✅ Hub serial shows: `[Hub] ESP-NOW from Node 1: XX.XX F, YY.YY %RH`
- ✅ Blynk V0 (Node 1 temp), V4 (humidity), V20 (battery) updated

**Pass/Fail**: PASS if data reaches hub and publishes to Blynk

---

## Part 3: Blynk Publishing & Batch Tests

### HUB-5: Batch Publishing Validation

**Objective**: Verify hub publishes all datastreams with shared timestamp

**Prerequisites**:
- Hub running with ≥1 node reporting data
- Blynk connection active

**Steps**:
1. Watch hub serial for batch messages
2. Count datastreams per batch
3. Check timing of batch messages (should be ~30-60s apart)
4. In Blynk app, check V0-V33 all updated

**Expected Results**:
- ✅ Serial shows: `[Blynk] Batch: NN datastreams with shared timestamp` (every 30-60s)
- ✅ Datastream count is 18-25+ (nodes + hub local)
- ✅ No spam of individual `[Blynk] V...` messages (old behavior)
- ✅ Blynk app shows all values updated within same second

**Pass/Fail**: PASS if one batch message per cycle with all datastreams

---

### HUB-6: Demo Mode Toggle

**Objective**: Verify demo mode generates simulated data

**Prerequisites**:
- Hub running and Blynk connected

**Steps**:
1. In Blynk app, send `1` to V100 (Demo mode)
2. Watch serial output
3. Check Blynk app for data updates
4. Wait 10 seconds
5. Send `0` to V100 to disable demo

**Expected Results**:
- ✅ Serial shows: `[Hub] Demo mode: ON`
- ✅ Temperatures show 65-80°F range (simulated)
- ✅ Humidity shows 35-65%RH range (simulated)
- ✅ Battery shows 3.8V, 75% (simulated)
- ✅ Updates every ~30 seconds
- ✅ Blynk app shows updated values
- ✅ Serial shows: `[Hub] Demo mode: OFF` when disabled
- ✅ Real data resumes when disabled

**Pass/Fail**: PASS if demo generates realistic simulated data

---

### HUB-7: Low Battery Event

**Objective**: Verify hub detects and reports low battery

**Prerequisites**:
- Hub running with Blynk connected
- Node 1 reporting data
- Blynk template has low_battery event

**Steps**:
1. Create low_battery event in Blynk Console (code: `low_battery`)
2. Watch for node with battery between 3.0-3.4V
3. If not naturally occurring, simulate by modifying code temporarily (testing only)
4. Check serial for event log
5. Check Blynk Console event log

**Expected Results**:
- ✅ Serial shows: `[Blynk] Event low_battery = Node N low: X.XXV`
- ✅ Blynk Console shows event triggered
- ✅ Event appears only once per transition (not spammed)
- ✅ Clears when battery recovers above 3.4V

**Pass/Fail**: PASS if low battery event triggers and logs correctly

---

### HUB-8: Node Offline Detection

**Objective**: Verify hub detects when node hasn't reported for 90+ minutes

**Prerequisites**:
- Hub running
- Node 1 pairing established but not reporting (simulate by moving out of range)

**Steps**:
1. Establish pairing, then move node out of ESP-NOW range
2. Wait ~5 minutes (node won't report)
3. Watch hub serial for offline detection
4. Check Blynk app V10 (Node 1 contact time)

**Expected Results**:
- ✅ After ~90 minutes without report, hub marks node offline
- ✅ Serial shows: `[Blynk] Event node_offline = Node 1 offline`
- ✅ Blynk V10 shows: `OFFLINE HH:MM AM/PM` (last contact time)
- ✅ Event triggered only once per transition
- ✅ When node reports again, status clears

**Pass/Fail**: PASS if offline detection and event fire correctly

---

## Part 4: Orbs Display Tests

### ORBS-1: Template Configuration

**Objective**: Verify Orbs Blynk template created with correct pins

**Prerequisites**:
- Blynk Console accessible
- Hub template already created

**Steps**:
1. Create new template: "Temperature Orbs"
2. Add datastreams:
   - V0-V9: Display values (double/string)
   - V10-V14: Contact times (string)
   - V30-V33: Battery % (integer)
   - V99: Hub status (string)
   - V101: Hub token (string)
3. Copy template ID and name

**Expected Results**:
- ✅ Template created successfully
- ✅ All 18 datastreams present
- ✅ V99 "Hub Status" configured
- ✅ V101 "Hub Token" configured as string (0-128 chars)
- ✅ Template ID/name copied

**Pass/Fail**: PASS if all pins configured correctly

---

### ORBS-2: Orbs Firmware Build & Flash

**Objective**: Verify Orbs firmware compiles and deploys

**Prerequisites**:
- Temperature Orbs repository cloned
- Template ID and name from ORBS-1
- Orbs ESP32 connected via USB

**Steps**:
1. Open Orbs firmware config (BlynkSettings.h or equivalent)
2. Update template ID and name
3. Build: `pio run -e firmware`
4. Flash: `pio run -e firmware -t upload`
5. Open serial monitor

**Expected Results**:
- ✅ Compilation succeeds (0 errors)
- ✅ Binary size reasonable (<50% flash)
- ✅ Upload completes
- ✅ Serial shows Orbs startup message

**Pass/Fail**: PASS if firmware builds and deploys

---

### ORBS-3: Orbs BLE Provisioning

**Objective**: Verify Orbs can be provisioned via Blynk app

**Prerequisites**:
- Orbs running firmware
- Blynk app installed
- Bluetooth enabled
- Wi-Fi available

**Steps**:
1. Open Blynk app → Add device → Find devices nearby
2. Enable Bluetooth
3. Select Orbs device
4. Follow provisioning wizard
5. Enter Wi-Fi credentials

**Expected Results**:
- ✅ Orbs appears in BLE scan
- ✅ App successfully enters credentials
- ✅ Orbs connects to Wi-Fi
- ✅ Serial shows: `[Orbs] Wi-Fi connected and IP acquired`
- ✅ Orbs device appears online in app

**Pass/Fail**: PASS if Orbs provisions and connects

---

### ORBS-4: Hub Token Entry (V101)

**Objective**: Verify Orbs can receive Hub's AUTH token via V101

**Prerequisites**:
- Orbs provisioned and running
- Hub created in Blynk Console
- Hub's AUTH token copied

**Steps**:
1. In Blynk app, go to Orbs device settings
2. Find V101 "Hub AUTH Token"
3. Paste Hub's AUTH token
4. Save/Confirm
5. Watch Orbs serial for token reception

**Expected Results**:
- ✅ Blynk app accepts string input (up to 128 chars)
- ✅ Orbs firmware reads V101
- ✅ Serial shows: `[Orbs] Hub token updated: XXXXXXXX...` (first 8 chars)
- ✅ Token stored in memory for use

**Pass/Fail**: PASS if V101 successfully transmits token to Orbs

---

### ORBS-5: Hub Data Fetching via External API

**Objective**: Verify Orbs fetches hub data using External API token

**Prerequisites**:
- Orbs provisioned with V101 populated
- Hub running and publishing data to Blynk
- Both devices on same Wi-Fi

**Steps**:
1. Watch Orbs serial output
2. Should show fetch attempts every ~10 seconds
3. Check if connection succeeds
4. Watch for JSON response parsing
5. Check V99 status

**Expected Results**:
- ✅ Serial shows: `[Orbs] Fetching hub data...`
- ✅ HTTPS connection to blynk.cloud succeeds
- ✅ Serial shows: `[Orbs] Hub request complete (200)` or similar
- ✅ JSON response received and parsed
- ✅ V99 updated to "ONLINE"
- ✅ Display values (V0-V9) updated with hub data

**Pass/Fail**: PASS if External API fetch succeeds and V99 shows "ONLINE"

---

### ORBS-6: Display Update Synchronization

**Objective**: Verify all 5 Orbs displays update synchronously

**Prerequisites**:
- 1-5 Orbs devices provisioned with hub token
- Hub publishing data
- All Orbs on same Wi-Fi

**Steps**:
1. Start all Orbs devices
2. Watch serial on each device
3. Observe fetch timing
4. Check if all show same hub data values
5. Wait 30 seconds for hub to publish new data
6. Check if all Orbs update within 10 seconds

**Expected Results**:
- ✅ All Orbs fetch data every ~10 seconds
- ✅ All Orbs show same V0-V9 values (within 1-2 seconds of each other)
- ✅ All Orbs update when hub publishes new batch
- ✅ No display lag or desync
- ✅ Timestamps match (last update time same on all)

**Pass/Fail**: PASS if all Orbs sync and display consistently

---

## Part 5: Error & Edge Cases

### ERR-1: Hub Wi-Fi Disconnect

**Objective**: Verify hub handles Wi-Fi disconnection gracefully

**Prerequisites**:
- Hub running and connected to Blynk

**Steps**:
1. Disconnect Wi-Fi (turn off router or move hub out of range)
2. Watch hub serial for disconnect messages
3. Watch V99 status on hub
4. Wait 30 seconds
5. Restore Wi-Fi

**Expected Results**:
- ✅ Serial shows: `[Hub] Wi-Fi disconnected`
- ✅ Hub doesn't crash or hang
- ✅ Hub attempts reconnection
- ✅ Serial shows: `[Hub] Wi-Fi connected...` when restored
- ✅ Data resumes publishing

**Pass/Fail**: PASS if hub recovers gracefully

---

### ERR-2: Orbs Hub Token Missing

**Objective**: Verify Orbs behaves correctly if V101 not configured

**Prerequisites**:
- Orbs provisioned but V101 empty

**Steps**:
1. Watch Orbs serial
2. Try to fetch hub data
3. Check V99 status

**Expected Results**:
- ✅ Serial shows: `[Orbs] ERROR: No hub token`
- ✅ V99 shows: "ERROR: No hub token" or similar
- ✅ Orbs doesn't crash
- ✅ Displays show placeholder or old data
- ✅ Retries periodically

**Pass/Fail**: PASS if Orbs handles gracefully

---

### ERR-3: Invalid Hub Token

**Objective**: Verify Orbs detects invalid token

**Prerequisites**:
- Orbs provisioned

**Steps**:
1. Enter fake/wrong token in V101 (e.g., "invalidtoken123")
2. Wait for fetch attempt
3. Watch Orbs serial

**Expected Results**:
- ✅ HTTPS request fails or returns 401/403
- ✅ Serial shows: `[Orbs] Hub request failed (40x)` or similar
- ✅ V99 shows: "OFFLINE" or "ERROR"
- ✅ Orbs doesn't crash
- ✅ Displays show "data unavailable" or last cached values

**Pass/Fail**: PASS if Orbs handles authentication error

---

### ERR-4: Hub SHT40 Sensor Failure

**Objective**: Verify hub handles sensor read failure

**Prerequisites**:
- Hub running
- SHT40 wired (or disconnect wire to simulate failure)

**Steps**:
1. If sensor present, disconnect I2C wire temporarily
2. Wait for hub update cycle
3. Check serial and Blynk V8/V9

**Expected Results**:
- ✅ Serial shows: `[Hub] SHT40 read failed`
- ✅ Blynk V8/V9 show: `-999` (error value)
- ✅ Batch still publishes
- ✅ Other datastreams unaffected

**Pass/Fail**: PASS if sensor failure doesn't crash hub

---

### ERR-5: Multiple Nodes Pairing Simultaneously

**Objective**: Verify hub can pair multiple nodes in one window

**Prerequisites**:
- Hub with open pairing window
- 2-4 nodes ready to pair

**Steps**:
1. Open hub pairing window (V41)
2. Power on Node 1, wait 5 seconds
3. Power on Node 2, wait 5 seconds
4. Power on Node 3, wait 5 seconds
5. Watch hub and node serials

**Expected Results**:
- ✅ Each node discovers hub independently
- ✅ Each shows successful pairing
- ✅ Hub acknowledges each pairing
- ✅ All nodes eventually report data
- ✅ No conflicts or errors

**Pass/Fail**: PASS if multiple nodes pair in sequence

---

## Part 6: Configuration & Security Tests

### CONFIG-1: Template Mismatch Detection

**Objective**: Verify firmware detects template mismatch

**Prerequisites**:
- Hub running with correct template

**Steps**:
1. Change `BLYNK_TEMPLATE_ID` to random value
2. Recompile and flash
3. Watch serial

**Expected Results**:
- ✅ Blynk connection fails or shows template mismatch
- ✅ Serial shows connection error
- ✅ App shows device "offline" or "needs update"
- ✅ Error is recoverable (fix template ID, reflash)

**Pass/Fail**: PASS if template mismatch detected

---

### CONFIG-2: AUTH Token Security

**Objective**: Verify hub token not exposed in firmware binary

**Prerequisites**:
- Hub firmware built

**Steps**:
1. Extract binary: `pio run -e firmware`
2. Use hex viewer to search for Blynk token pattern
3. Search for `BLYNK_AUTH_TOKEN` or similar

**Expected Results**:
- ✅ Token NOT visible in binary (encrypted in NVS storage)
- ✅ No hardcoded auth strings
- ✅ Firmware is portable/shareable

**Pass/Fail**: PASS if no auth token in binary

---

### CONFIG-3: Orbs Token in Firmware

**Objective**: Verify Orbs token (Hub's token) is read-only safe

**Prerequisites**:
- Orbs V101 populated with hub token

**Steps**:
1. Document that token is Hub's read-only External API token
2. Test that Orbs can only READ data
3. Attempt to modify hub datastream with Orbs (should fail)

**Expected Results**:
- ✅ Orbs successfully fetches hub data (read)
- ✅ Orbs cannot publish to hub datastreams (no write permission)
- ✅ Orbs cannot control hub features (V40, V41, V100)
- ✅ Read-only token is safe to have in firmware

**Pass/Fail**: PASS if read-only External API validated

---

## Part 7: Integration & Scenario Tests

### INT-1: Complete End-to-End Flow

**Objective**: Validate entire system in normal operation

**Prerequisites**:
- Hub provisioned and running
- 1-4 nodes paired
- 1 Orbs device provisioned with hub token
- All on same Wi-Fi
- Blynk connected

**Steps**:
1. Node 1 wakes, reads sensor, sends to hub
2. Hub receives and publishes batch
3. Orbs fetches data via External API
4. Displays update on all 5 screens
5. Repeat for 2-3 cycles (~1-2 minutes)

**Expected Results**:
- ✅ Node data reaches hub within ~5 seconds
- ✅ Hub publishes within ~10 seconds
- ✅ Orbs fetches and displays within 10 seconds
- ✅ No data loss or corruption
- ✅ All values match (temp, humidity, battery, contact time)
- ✅ Timestamps consistent

**Pass/Fail**: PASS if complete flow works end-to-end

---

### INT-2: Hub Role to Node Role Switch

**Objective**: Verify changing jumper correctly switches roles

**Prerequisites**:
- Hub firmware flashed and running

**Steps**:
1. With D3 connected to GND, hub runs (verify serial shows hub mode)
2. Connect D0 button, hold for 10-20 seconds (reset config)
3. Disconnect GND from D3 (open), restart
4. Node mode should activate

**Expected Results**:
- ✅ With D3→GND: `[Hub] Starting`
- ✅ With D3 open: `[Node 1] Starting` (with correct ID based on D1/D2)
- ✅ Switching roles works multiple times
- ✅ No cross-contamination between roles

**Pass/Fail**: PASS if role switching works reliably

---

### INT-3: Node ID Jumper Configuration

**Objective**: Verify D1/D2 correctly set Node IDs

**Prerequisites**:
- Node firmware flashed

**Steps**:
1. Set D1=Open, D2=Open → Restart → Expect Node 1
2. Set D1=GND, D2=Open → Restart → Expect Node 2
3. Set D1=Open, D2=GND → Restart → Expect Node 3
4. Set D1=GND, D2=GND → Restart → Expect Node 4

**Expected Results**:
- ✅ Node 1: `[Node 1] Starting`
- ✅ Node 2: `[Node 2] Starting`
- ✅ Node 3: `[Node 3] Starting`
- ✅ Node 4: `[Node 4] Starting`
- ✅ Each ID configurable independently

**Pass/Fail**: PASS if all 4 node IDs work correctly

---

## Part 8: Performance & Stress Tests

### PERF-1: Sustained Data Publishing

**Objective**: Verify hub can publish continuously without memory leaks

**Prerequisites**:
- Hub running with ≥2 nodes
- Monitor free heap memory

**Steps**:
1. Let hub run for 30 minutes
2. Check serial every 5 minutes for heap memory
3. Verify no crashes or hangs
4. Check if memory trends downward (leak) or stable

**Expected Results**:
- ✅ Hub publishes batch every 30-60 seconds
- ✅ Blynk connection remains stable
- ✅ Free heap memory stable (no progressive decrease)
- ✅ No crashes or unexpected resets
- ✅ Datastreams remain up-to-date

**Pass/Fail**: PASS if 30+ minutes stable operation

---

### PERF-2: Orbs Continuous Fetching

**Objective**: Verify Orbs can fetch data continuously

**Prerequisites**:
- Orbs running with hub token
- Monitor free heap memory

**Steps**:
1. Let Orbs run for 30 minutes
2. Check serial every 5 minutes for heap and fetch success
3. Count successful fetches vs failures
4. Verify displays update consistently

**Expected Results**:
- ✅ Fetches every ~10 seconds
- ✅ >95% fetch success rate
- ✅ Free heap memory stable
- ✅ No crashes or hangs
- ✅ Display updates consistently

**Pass/Fail**: PASS if 30+ minutes stable fetching

---

## Checklist Summary

### Must Pass (Critical):
- [ ] HUB-2: Firmware builds and flashes
- [ ] HUB-3: BLE provisioning works
- [ ] HUB-5: Batch publishing with shared timestamp
- [ ] NODE-4: Node discovers and pairs with hub
- [ ] NODE-5: ESP-NOW data delivery to hub
- [ ] ORBS-5: Hub data fetch via External API
- [ ] INT-1: Complete end-to-end flow

### Should Pass (Important):
- [ ] HUB-4: Local sensor reads correctly
- [ ] NODE-2: Remote sensor reads correctly
- [ ] HUB-7: Low battery event detection
- [ ] ORBS-6: Display synchronization
- [ ] CONFIG-2: Auth token security

### Nice to Pass (Enhancement):
- [ ] HUB-8: Offline detection after 90 min
- [ ] HUB-6: Demo mode generates data
- [ ] PERF-1: 30+ min stable publishing
- [ ] PERF-2: 30+ min stable fetching
- [ ] INT-3: All 4 node IDs work

---

## Test Execution Template

```
Device: [Hub/Node 1/Orbs 1]
Test ID: [HUB-1, NODE-5, etc]
Date: [YYYY-MM-DD]
Tester: [Name]

Preconditions Met: Y/N
Steps Completed: Y/N
Expected Results Met: Y/N

PASS / FAIL / SKIP

Comments:
```

---

## Sign-Off

| Phase | Tester | Date | Status |
|-------|--------|------|--------|
| Hub Setup | | | |
| Node Pairing | | | |
| Blynk Publishing | | | |
| Orbs Integration | | | |
| Full System | | | |

