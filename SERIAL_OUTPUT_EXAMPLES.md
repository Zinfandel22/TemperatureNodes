# Serial Output Examples

This document contains **real example scenarios** of serial output you can expect during node and hub operation. Use these to validate your hardware is behaving correctly.

---

## 🟢 NODE SCENARIO 1: First-Time Pairing (New Node)

**Setup**: Brand new node, never paired before. Hub is online with pairing window open.

**Terminal Output @ 115200 baud**:

```
18:30:45.123 [Node 2] Starting
18:30:45.230 [Node] Reading battery and SHT40
18:30:46.045 [Node 2] SHT40: 72.50 F, 45.23 %RH
18:30:46.123 [Node] Searching for a hub (up to 30 seconds)
18:30:47.234 [Node] Discovery send failed: ESP_ERR_ESPNOW_NO_PEER
18:30:47.892 [Node] Discovery send failed: ESP_ERR_ESPNOW_NO_PEER
18:30:52.145 [Node] Discovery send failed: ESP_ERR_ESPNOW_NO_PEER
18:31:08.456 [Node] Paired with hub on channel 11
18:31:08.567 [Node] Measurement acknowledged at radio level
18:31:09.234 [Node] Sleeping for 30 minutes
```

**Analysis**:
- **00:45**: Device powers up, determines it's a node (D3=HIGH)
- **00:46**: Sensor initialized and read successfully
- **00:46**: No hub stored locally; starts channel scanning (broadcasts on Ch1)
- **00:47-00:52**: Normal discovery retries as it scans through channels
- **01:08**: Found hub on channel 11! ✅ Saves hub MAC and channel to storage
- **01:08**: Measurement sent and ESP-NOW ACK received ✅
- **01:09**: Enters deep sleep; CPU powers down

**Expected on Hub Serial** (within 2-3 seconds):
```
[Hub] Node 2 report: 72.50 F, 45.23 %RH
[Blynk] ◀ virtualWrite(V1, 72.50)
[Blynk] ◀ virtualWrite(V5, 45.23)
...
[Blynk] ◀ endGroup()
```

**Expected in Blynk App**: Node 2 appears on dashboard with 72.50°F and 45.23% RH ✅

---

## 🟢 NODE SCENARIO 2: Subsequent Boot (Saved Hub)

**Setup**: Same node from Scenario 1, now has hub stored locally. Node boots again 30 minutes later.

**Terminal Output**:

```
19:00:12.567 [Node 2] Starting
19:00:12.645 [Node] Reading battery and SHT40
19:00:13.234 [Node 2] SHT40: 71.80 F, 46.50 %RH
19:00:13.345 [Node] Using saved hub on channel 11
19:00:13.456 [Node] Measurement acknowledged at radio level
19:00:14.123 [Node] Sleeping for 30 minutes
```

**Analysis**:
- **00:12**: Device boots, reads sensor
- **00:13**: Finds hub MAC and channel in NVS storage
- **00:13**: Skips discovery! Uses saved channel 11 directly
- **00:13**: Measurement sent and ACK'd immediately ✅
- **00:14**: Enters deep sleep

**Timing Improvement**: Compare to Scenario 1
- Scenario 1 (discovery): ~24 seconds active
- Scenario 2 (saved hub): ~2 seconds active 🚀
- **Power savings**: 12× better battery life on subsequent boots!

---

## 🔴 NODE SCENARIO 3: Hub Not Found During Discovery

**Setup**: Node boots to pair, but hub is OFFLINE or pairing window is CLOSED.

**Terminal Output**:

```
18:30:45.123 [Node 1] Starting
18:30:45.230 [Node] Reading battery and SHT40
18:30:45.890 [Node 1] SHT40: 68.30 F, 52.10 %RH
18:30:46.045 [Node] Searching for a hub (up to 30 seconds)
18:30:47.234 [Node] Discovery send failed: ESP_ERR_ESPNOW_NO_PEER
18:30:52.145 [Node] Discovery send failed: ESP_ERR_ESPNOW_NO_PEER
18:30:57.456 [Node] Discovery send failed: ESP_ERR_ESPNOW_NO_PEER
18:31:12.567 [Node] No hub found; retrying after deep sleep
18:31:13.234 [Node] Sleeping for 30 minutes
```

**Analysis**:
- **00:45-01:12**: Scans all channels (usually completes in 25-30 seconds)
- **01:12**: NO PAIR_RESPONSE received from any hub ❌
- **01:13**: Enters deep sleep; will retry in 30 minutes

**Troubleshooting**:
1. ✅ Check hub is powered and running
2. ✅ Check hub has pairing window open (V41 = 1 in app, or D0 button pressed)
3. ✅ Check hub serial output shows: `[Hub] Waiting for node pairing`
4. ✅ Restart node while pairing window is open (not from cold boot; use RST button)

---

## 🔴 NODE SCENARIO 4: SHT40 Sensor Failure

**Setup**: Node boots but SHT40 sensor doesn't respond (bad I2C connection).

**Terminal Output**:

```
18:30:45.123 [Node 3] Starting
18:30:45.230 [Node] Reading battery and SHT40
18:30:46.045 [Node] SHT40 initialization failed; sending invalid sensor readings
18:30:46.123 [Node] Using saved hub on channel 6
18:30:46.234 [Node] Measurement acknowledged at radio level
18:30:46.890 [Node] Sleeping for 30 minutes
```

**Analysis**:
- **00:45**: Device boots
- **00:46**: SHT40 I2C communication fails ❌
- **00:46**: Still proceeds with measurement (temperature = -999, humidity = -999)
- **00:46**: Sends to hub with invalid values

**What you'll see in Blynk**:
```
Node 3 Temperature: -999.00 °F
Node 3 Humidity: -999.00 %RH
```

**Troubleshooting**:
1. ✅ Check SHT40 wiring on XIAO board
   - SDA = D4 (GPIO 5) ← pull-up resistor on board
   - SCL = D5 (GPIO 6) ← pull-up resistor on board
   - 3.3V = power
   - GND = common ground
2. ✅ Check for loose connector or cold solder joint on sensor
3. ✅ Try reseating the sensor (remove and reinstall)
4. ✅ Reflash firmware if issue persists

---

## 🟡 NODE SCENARIO 5: Battery Low Warning

**Setup**: Node with low battery (3.2–3.4V) sends measurement.

**Terminal Output**:

```
18:30:45.123 [Node 4] Starting
18:30:45.230 [Node] Reading battery and SHT40
18:30:45.890 [Node 4] SHT40: 70.15 F, 48.75 %RH
18:30:46.045 [Node] Using saved hub on channel 8
18:30:46.234 [Node] Measurement acknowledged at radio level
18:30:46.890 [Node] Sleeping for 30 minutes
```

**On Hub Serial** (within 2 seconds):
```
[Hub] Node 4 report: 70.15 F, 48.75 %RH
[Blynk] ◀ virtualWrite(V3, 70.15)
...
[Blynk] logEvent("low_battery", "Node 4 low: 3.25V")
```

**What you'll see in Blynk**:
```
Node 4 Battery Voltage: 3.25 V
Node 4 Battery Level: 5 %
⚠️ LOW_BATTERY event: "Node 4 low: 3.25V"
```

**Troubleshooting**:
- Node battery voltage is between 2.50V and 3.40V
- Charge the LiPo battery back to 4.2V
- If battery won't hold charge, may need battery replacement

---

## 🟢 HUB SCENARIO 1: Provisioning Sequence

**Setup**: Fresh hub just powered on; user provisions via BLE and app.

**Terminal Output**:

```
19:15:30.123 [Hub] Starting
19:15:30.567 [Hub] Starting Blynk Edgent
19:15:31.234 [Hub] Waiting for Blynk provisioning
(User opens Blynk app and scans QR code...)
19:15:45.567 [Hub] Wi-Fi connecting
19:15:48.234 [Hub] Wi-Fi connected to access point; waiting for IP address
19:15:48.345 [Hub] Station MAC: AA:BB:CC:DD:EE:FF, Wi-Fi channel: 6
19:15:52.456 [Hub] Wi-Fi connected and IP address acquired
19:15:53.123 [Hub] Connecting to Blynk
(Blynk SSL handshake happens...)
19:16:05.789 [Blynk] Requesting V100 sync
19:16:06.234 [Hub] Blynk connected; open a pairing window with V41 or the D0 button
```

**Analysis**:
- **15:30**: Device powers up, detects hub mode (D3=LOW)
- **15:31**: Blynk Edgent starts (ready for BLE provisioning)
- **15:45**: User enters Wi-Fi SSID and password
- **15:48**: Connected to Wi-Fi AP on channel 6
- **15:52**: DHCP lease obtained, IP address assigned
- **15:53**: Initiating connection to Blynk servers
- **16:06**: Connected to Blynk! ✅

**Expected in Blynk App**:
- Device shows as "ONLINE"
- Ready to open pairing window for nodes

---

## 🟢 HUB SCENARIO 2: Normal Operation Loop

**Setup**: Hub is provisioned and connected. Node 1 and Node 2 have already paired.

**Terminal Output (recurring every 30-60 seconds)**:

```
19:16:30.123 [Hub] Node 1 report: 72.50 F, 45.23 %RH
19:16:30.234 [Blynk] ◀ virtualWrite(V0, 72.50)
19:16:30.245 [Blynk] ◀ virtualWrite(V4, 45.23)
19:16:30.256 [Blynk] ◀ virtualWrite(V10, "03:16 PM")
19:16:30.267 [Blynk] ◀ virtualWrite(V20, 4.10)
19:16:30.278 [Blynk] ◀ virtualWrite(V30, 85)
19:16:30.289 [Blynk] ◀ virtualWrite(V1, 68.30)
19:16:30.300 [Blynk] ◀ virtualWrite(V5, 52.10)
19:16:30.311 [Blynk] ◀ virtualWrite(V11, "03:15 PM")
19:16:30.322 [Blynk] ◀ virtualWrite(V21, 3.95)
19:16:30.333 [Blynk] ◀ virtualWrite(V31, 75)
19:16:30.344 [Blynk] ◀ virtualWrite(V8, 71.20)
19:16:30.355 [Blynk] ◀ virtualWrite(V9, 46.50)
19:16:30.366 [Blynk] ◀ virtualWrite(V14, "03:16 PM")
19:16:30.377 [Blynk] ◀ endGroup()
```

**Analysis**:
- **30:123**: Node 1 measurement arrives (from 30 min ago)
- **30:234-30:378**: Hub publishes all 25+ datastreams
  - V0–V3: Node temperatures
  - V4–V7: Node humidity
  - V8–V9: Hub local sensor
  - V10–V14: Contact times
  - V20–V23: Battery voltages
  - V30–V33: Battery percentages
- **30:377**: endGroup() batches all writes with shared timestamp

**What you'll see in Blynk App**:
- All 4 nodes showing latest readings
- Hub local sensor showing (if configured)
- All contact times matching last measurement time
- Battery voltages and levels updating

---

## 🟡 HUB SCENARIO 3: Node Goes Offline

**Setup**: Node 2 hasn't reported for 90+ minutes (expected: 3 × 30-min cycles).

**Terminal Output**:

```
(Normal operation continues for 90+ minutes with no Node 2 report)

19:50:15.123 [Hub] Node 2 report: 68.30 F, 52.10 %RH    ← LAST report (1.5h ago)
19:50:15.234 [Blynk] ◀ virtualWrite(V1, 68.30)
19:50:15.245 [Blynk] ◀ virtualWrite(V5, 52.10)
19:50:15.256 [Blynk] ◀ virtualWrite(V11, "03:15 PM")

(Time passes... no new Node 2 reports arrive)

21:30:12.345 [Hub] Node 2 offline after 3 missed reports    ← OFFLINE!
21:30:12.456 [Blynk] ◀ virtualWrite(V11, "OFFLINE 03:15 PM")
21:30:12.567 [Blynk] logEvent("node_offline", "Node 2 offline after 3 missed reports")
```

**What you'll see in Blynk App**:
```
Node 2 Last Contact: "OFFLINE 03:15 PM"
Node 2 Temperature: 68.30 °F (stale value from 1.5h ago)
⚠️ NODE_OFFLINE event: "Node 2 offline after 3 missed reports"
```

**Troubleshooting**:
1. ✅ Check node battery (may be dead)
2. ✅ Check node is powered on
3. ✅ Restart node or press RST button to wake from sleep
4. ✅ Verify hub pairing window is open (Node 2 may need to re-pair)
5. ✅ Check hub still connected to Blynk and Wi-Fi

---

## 🟢 NODE SCENARIO 6: D0 Button Reset

**Setup**: Node is paired and boots with D0 button held down.

**Terminal Output**:

```
18:30:45.123 [Node 1] Starting
18:30:45.234 [Node] Hold D0 for 10 seconds, then release to clear pairing
(User continues holding D0...)
18:30:50.456 [Node] Release D0 to clear pairing      ← 5 seconds elapsed
(User releases D0 at ~7 seconds...)
18:30:52.789 [Node] Reading battery and SHT40
18:30:53.345 [Node 1] SHT40: 72.50 F, 45.23 %RH
18:30:53.456 [Node] Pairing cleared; searching for a hub
18:30:53.567 [Node] Searching for a hub (up to 30 seconds)
...
(Discovery continues as normal)
18:31:12.890 [Node] Paired with hub on channel 6
18:31:13.123 [Node] Measurement acknowledged at radio level
18:31:13.890 [Node] Sleeping for 30 minutes
```

**Analysis**:
- **00:45**: Device boots, button is held
- **00:45**: Firmware detects hold and waits for release
- **00:50**: At 5 seconds, tells user to release button
- **~00:52**: User releases button (at 7 seconds, within 5-10s window)
- **00:53**: Pairing cleared from storage ✅
- **00:53**: Enters discovery mode (like first pairing)
- **01:12**: Finds new hub and pairs ✅

**Next boot**: Node will use new hub MAC/channel (old pairing forgotten)

---

## 🔴 NODE SCENARIO 7: Button Held Too Long

**Setup**: Node boots with D0 button, user holds too long.

**Terminal Output**:

```
18:30:45.123 [Node 1] Starting
18:30:45.234 [Node] Hold D0 for 10 seconds, then release to clear pairing
(User keeps holding past 10 seconds...)
18:30:55.890 [Node] Pairing reset canceled
18:30:56.123 [Node] Reading battery and SHT40
18:30:56.789 [Node 1] SHT40: 72.50 F, 45.23 %RH
18:30:56.890 [Node] Using saved hub on channel 6
18:30:57.123 [Node] Measurement acknowledged at radio level
18:30:57.890 [Node] Sleeping for 30 minutes
```

**Analysis**:
- **00:45**: Button held down
- **00:55**: User still holding at 10+ seconds
- **00:55**: Reset canceled; node keeps existing pairing ✅
- **00:57**: Node proceeds with measurement using old hub

**Result**: Pairing NOT cleared; node continues with existing hub

---

## 🔄 HUB SCENARIO 4: Pairing Reset via V40

**Setup**: User sends pairing reset from Blynk app while hub is online.

**Hub Serial**:

```
19:50:15.123 [Hub] Pairing reset queued for all nodes
(Time passes... node 1 wakes and reports...)
20:20:30.456 [Hub] Node 1 report: 72.50 F, 45.23 %RH
20:20:30.567 [Hub] Pairing cleared for node 1
(Time passes... node 2 wakes...)
20:50:45.789 [Hub] Node 2 report: 68.30 F, 52.10 %RH
20:50:45.890 [Hub] Pairing cleared for node 2
```

**On Node 1 Serial** (when it wakes and receives reset):

```
20:20:30.123 [Node 1] Starting
20:20:30.234 [Node] Reading battery and SHT40
20:20:30.890 [Node 1] SHT40: 72.50 F, 45.23 %RH
20:20:30.901 [Node] Using saved hub on channel 6
20:20:30.950 [Node] Measurement acknowledged at radio level
20:20:31.012 [Node] Pairing cleared; searching for a hub  ← Reset received!
20:20:31.123 [Node] Sleeping for 30 minutes
```

**Next Node 1 Boot** (30 minutes later):
```
20:50:30.123 [Node 1] Starting
20:50:30.234 [Node] Reading battery and SHT40
20:50:30.890 [Node 1] SHT40: 72.50 F, 45.23 %RH
20:50:30.901 [Node] Searching for a hub (up to 30 seconds)  ← No saved hub!
...
(Discovery starts, finds new hub)
```

**Timeline**:
- T=0: User sends V40=1 from app
- T=0: Hub queues reset for all nodes
- T=~30min: Node 1 wakes, receives reset, clears pairing
- T=~60min: Node 2 wakes, receives reset, clears pairing
- T=~90min: Nodes wake again, search for hub, find new one

---

## 📊 Serial Output Comparison Table

| Scenario | Active Time | Key Message | Outcome |
|----------|-------------|-------------|---------|
| **New Node** | ~25 sec | "Paired with hub on channel 11" | ✅ Pairing successful |
| **Saved Hub** | ~2 sec | "Using saved hub on channel 11" | ✅ Fast reboot |
| **Hub Offline** | ~30 sec | "No hub found; retrying..." | ⏳ Sleep 30min; retry |
| **Sensor Fail** | ~1 sec | "SHT40 init failed" | ⚠️ Sends -999 values |
| **Low Battery** | ~2 sec | "Node low: 3.25V" | ⚠️ Event triggered |
| **Button Reset** | ~25 sec | "Pairing cleared; searching" | 🔄 Re-pairs to new hub |
| **Pairing Window** | Variable | "Waiting for node pairing" | 👂 Listening 2 minutes |
| **Node Offline** | Variable | "offline after 3 missed" | 📵 No contact >90min |

---

## 🎯 Real-World Testing Guide

### Test Case 1: Validate First Pairing
1. Power hub; wait for provisioning
2. Connect to Blynk; open pairing window (V41)
3. Power node; watch serial output
4. ✅ Expect: Node discovers hub in 5-15 seconds
5. ✅ Expect: Hub receives measurement within 2 seconds
6. ✅ Expect: Blynk app shows node data

### Test Case 2: Validate Saved Hub
1. Power hub
2. Power same node
3. ✅ Expect: "Using saved hub" message
4. ✅ Expect: Measurement in <3 seconds
5. 🚀 Compare to Test Case 1 (12× faster!)

### Test Case 3: Validate Battery Low Warning
1. Use power supply to drop node battery to 3.25V
2. Power node
3. Watch hub serial for low_battery event
4. ✅ Expect: "low_battery" event in Blynk

### Test Case 4: Validate Node Offline Detection
1. Keep hub running 90+ minutes without node reporting
2. ✅ Expect: "offline after 3 missed reports"
3. ✅ Expect: "OFFLINE HH:MM PM" in Blynk

---

## 🔗 Related Documentation

- **[SERIAL_DEBUG_REFERENCE.md](SERIAL_DEBUG_REFERENCE.md)** – Complete message reference
- **[NODE_BOOT_SEQUENCE.md](NODE_BOOT_SEQUENCE.md)** – Flow diagram and timing
- **[TEST_PLAN.md](TEST_PLAN.md)** – Full test suite with expected results
- **[README.md](README.md)** – Hardware setup and configuration
