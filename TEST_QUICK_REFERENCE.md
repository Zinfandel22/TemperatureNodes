# Test Quick Reference Card

## 7 Critical Tests (Must Pass)

| ID | Test | What It Validates | Fail Impact |
|----|------|-------------------|------------|
| **HUB-2** | Firmware Build | Compilation, binary size, flashing | Cannot proceed with any other tests |
| **HUB-3** | BLE Provisioning | Hub connects to Wi-Fi via Blynk app | Hub cannot connect to cloud |
| **NODE-4** | Node Pairing | Node discovers hub and exchanges MAC/channel | Nodes cannot communicate with hub |
| **NODE-5** | ESP-NOW Delivery | Node sends sensor data to hub | Hub receives no node data |
| **HUB-5** | Batch Publishing | Hub sends all 25+ datastreams with shared timestamp | Data inconsistency, Orbs get stale data |
| **ORBS-5** | External API Fetch | Orbs retrieves hub data via `/getAll?token=` | Orbs displays are blank |
| **INT-1** | End-to-End Flow | Node → Hub → Blynk → Orbs complete cycle | Entire system nonfunctional |

---

## Test Execution Workflow

```
START
  ↓
HUB-1: Template config ✓
  ↓
HUB-2: Build firmware ✓ ⭐ CRITICAL
  ↓
HUB-3: BLE provision ✓ ⭐ CRITICAL
  ↓
NODE-1: Flash firmware ✓
  ↓
NODE-4: Pair with hub ✓ ⭐ CRITICAL
  ↓
NODE-5: Send ESP-NOW ✓ ⭐ CRITICAL
  ↓
HUB-5: Batch publish ✓ ⭐ CRITICAL
  ↓
ORBS-1-4: Setup Orbs ✓
  ↓
ORBS-5: Fetch hub data ✓ ⭐ CRITICAL
  ↓
INT-1: End-to-end ✓ ⭐ CRITICAL
  ↓
ERROR/EDGE: Run all ERR/PERF tests
  ↓
PASS: Ready for deployment
```

---

## Expected Serial Output Patterns

### Hub Startup
```
[Hub] Starting
[Hub] Wi-Fi connecting...
[Hub] Wi-Fi connected and IP address acquired
[Hub] Blynk RUNNING
[Hub] Node 1 report: 72.50 F, 45.20 %RH
[Blynk] Batch: 25 datastreams with shared timestamp
```

### Node Startup
```
[Node 1] Starting
[Node 1] Reading sensors...
[Node 1] Temp: 72.50 F, Humidity: 45.20 %RH
[Node 1] Battery: 3.85 V, 85%
[Node 1] Entering deep sleep for 30 minutes
```

### Pairing Flow
```
[Hub] Pairing window open (120 seconds)
[Node 1] Scanning for hub...
[Node 1] Found hub: MAC XX:XX:XX:XX:XX:XX, Ch 6
[Node 1] Hub pairing successful
[Hub] Node 1 paired
```

### Orbs Fetch
```
[Orbs] Fetching hub data...
[Orbs] Hub request complete (200)
[Orbs] Received: V0, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V30, V31, V32, V33
[Orbs] Hub data received and displayed
[Blynk] V99 = ONLINE
```

---

## Common Issues & Quick Fixes

| Symptom | Likely Cause | Quick Fix |
|---------|--------------|-----------|
| Hub won't BLE scan | Bluetooth off on phone | Enable Bluetooth settings |
| Hub won't connect Wi-Fi | Wrong password or 5 GHz network | Verify 2.4 GHz SSID/password |
| Node won't pair | Pairing window closed | Re-open V41 or D0 button |
| ESP-NOW not received | Hub not on same Wi-Fi channel | Pairing establishes channel automatically |
| Hub not publishing to Blynk | Template ID mismatch | Check BLYNK_TEMPLATE_ID in config |
| Orbs show "ERROR" in V99 | Hub token (V101) missing or wrong | Paste correct hub AUTH token in V101 |
| Orbs don't update | Hub not publishing data | Check hub serial for batch messages |
| Battery shows -999 | Voltage divider not wired | Connect two 220kΩ resistors to A0 |

---

## Serial Monitor Commands

| Task | Command |
|------|---------|
| View hub logs | PlatformIO Terminal: `pio device monitor -p /dev/ttyUSB0 -b 115200` |
| View node logs | Connect via USB, monitor at 115200 baud |
| Reset Blynk config on hub | Hold D0 button for 10+ seconds (Edgent reset) |
| Reset node pairing | Hold D0 button on node for 10-20 seconds |
| Trigger hub pairing window | Send `1` to V41 in Blynk app (or click D0 button) |
| Enable hub demo mode | Send `1` to V100 in Blynk app |

---

## Checklist: Before Starting Tests

**Hardware**:
- [ ] Hub ESP32-S3 connected via USB
- [ ] Node 1-4 ESP32-S3 available (can be sequential)
- [ ] SHT40 sensors wired (optional for initial build test)
- [ ] Battery dividers wired (optional for initial test)
- [ ] Jumpers set: Hub (D3→GND), Node 1 (D3 open, D1 open, D2 open)

**Software**:
- [ ] PlatformIO installed
- [ ] Repository cloned
- [ ] Blynk account created
- [ ] Blynk app installed on phone
- [ ] Bluetooth enabled on phone

**Network**:
- [ ] 2.4 GHz Wi-Fi available (DHCP enabled)
- [ ] SSID and password known
- [ ] All devices on same network

**Documentation**:
- [ ] README.md read
- [ ] QUICK_START.md reviewed
- [ ] TEST_PLAN.md printed or accessible

---

## Test Execution Template (Copy for Each Test)

```
╔══════════════════════════════════════════════════════════════╗
║                     TEST EXECUTION LOG                       ║
╚══════════════════════════════════════════════════════════════╝

Test ID:          [e.g., HUB-5]
Test Name:        [e.g., Batch Publishing Validation]
Device:           [Hub / Node 1-4 / Orbs 1-5]
Date:             [YYYY-MM-DD]
Time Started:     [HH:MM]
Tester Name:      [Name]

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Pre-Test Checklist:
  [ ] All prerequisites met
  [ ] Device powered and connected
  [ ] Serial monitor open and showing output
  [ ] Blynk app ready (if needed)

Steps Completed:
  [ ] Step 1: ___________________
  [ ] Step 2: ___________________
  [ ] Step 3: ___________________
  [ ] Step 4: ___________________

Expected Results Check:
  [ ] Expected result 1: ✓ / ✗
  [ ] Expected result 2: ✓ / ✗
  [ ] Expected result 3: ✓ / ✗

RESULT:    ✅ PASS / ❌ FAIL / ⏭️ SKIP

Time Ended:    [HH:MM]
Duration:      [MM minutes]

Comments/Notes:
_________________________________________________________________
_________________________________________________________________
_________________________________________________________________

If FAIL, Root Cause:
_________________________________________________________________

Action Taken:
_________________________________________________________________

Signed: ________________________   Date: __________
```

---

## Success Criteria Summary

| Phase | Criteria | Metric |
|-------|----------|--------|
| **Hub** | Blynk connectivity | Device shows "RUNNING" in app |
| **Hub** | Batch publishing | Serial shows "Batch: N datastreams" every 30-60s |
| **Hub** | Sensor accuracy | Values reasonable for environment (50-85°F, 20-70%RH) |
| **Nodes** | Pairing success | All 4 nodes pair and report data |
| **Nodes** | ESP-NOW delivery | Node data reaches hub within 5 seconds |
| **Orbs** | External API | V99 shows "ONLINE", V0-V9 show hub data |
| **Orbs** | Synchronization | All Orbs update within 10 seconds of hub publish |
| **System** | End-to-end latency | Node → Hub (5s) + Hub → Blynk (10s) + Orbs fetch (10s) = ~25s max |

---

## Sign-Off

| Phase | Tests | Tester | Date | Status |
|-------|-------|--------|------|--------|
| Hub Setup | HUB-1 to HUB-8 | | | |
| Node Pairing | NODE-1 to NODE-5 | | | |
| Blynk & Batch | HUB-5, HUB-6 | | | |
| Orbs Integration | ORBS-1 to ORBS-6 | | | |
| Error Handling | ERR-1 to ERR-5 | | | |
| Security | CONFIG-1 to CONFIG-3 | | | |
| Integration | INT-1 to INT-3 | | | |
| Performance | PERF-1 to PERF-2 | | | |
| **SYSTEM READY** | **ALL PASS** | | | |

---

## Notes

- **Critical Tests** must all PASS before proceeding to next phase
- **Should Pass** tests should PASS but can be deferred if infrastructure issue
- **Nice to Pass** tests are for validation but not blockers
- If FAIL: Document root cause and retry after fix
- Run INT-1 (end-to-end) last to validate complete system
- Keep logs from all tests for 30-day retention period
