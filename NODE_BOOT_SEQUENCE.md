# Node Pairing Boot Sequence - Serial Output Map

## Complete Serial Output Flow: Node Power-Up → Pairing → Sleep

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        🔴 NODE POWER-UP BOOT                                │
└─────────────────────────────────────────────────────────────────────────────┘

[Node 1] Starting                                          ← 🟢 Boot detected
                                                             Role pin = HIGH = Node
                                                             RTC boot count incremented

                      ┌──────────────────────────────────┐
                      │ Check Pairing Storage            │
                      └──────────────────────────────────┘
                                  │
                    ┌─────────────┴─────────────┐
                    │                           │
            🟢 Has Hub Stored          🔴 No Hub Stored
                    │                           │
         [Node] Reading battery                 │
         and SHT40                              │
                    │                           │
         [Node 1] SHT40:                        │
         72.50 F, 45.23 %RH                     │
                    │                           │
         [Node] Using saved hub on              [Node] Searching for a hub
         channel 6 ◀──────────────────────────▶ (up to 30 seconds)
                    │                           │
         ┌──────────┴───────────┐       ┌───────┴──────────┐
         │  Channel Scan Loop   │       │ Full Scan: Ch1-14│
         │  (single channel)    │       │                  │
         │                      │       │                  │
         │ Set channel to 6     │       │ Set channel 1    │
         │ Send PAIR_REQUEST    │       │ Send PAIR_REQUEST│
         │ Wait 450ms           │       │ Wait 450ms       │
         │ Check for response   │       │                  │
         └──────────┬───────────┘       │ No response?     │
                    │                   │ Try next channel │
                    │                   │                  │
                    │                   └───────┬──────────┘
                    │                           │
            ┌───────┴───────────────────────────┘
            │
            └───────────────────────────────────┐
                                                │
                      ┌──────────────────────────┴──────────────┐
                      │                                         │
                   🟢 Hub Found                           🔴 No Hub Found
                      │                                         │
              [Node] Paired with hub                 [Node] No hub found;
              on channel 6                           retrying after deep sleep
                      │                                         │
              Save to NVS storage ◀──────────────────────────┐  │
                      │                                      │  │
                      └──────────────────────────────────────┘  │
                                                                 │
┌──────────────────────────────────────────────────────────────────────────────┐
│                 📡 SEND MEASUREMENT TO HUB                                   │
└──────────────────────────────────────────────────────────────────────────────┘

Add hub as ESP-NOW peer
Create SensorPacket:
  - nodeId = 1
  - temperatureF = 72.50
  - humidityPct = 45.23
  - batteryVolts = 4.15
  - bootCount = 1234

Send packet (up to 3 retries)
                │
    ┌───────────┴─────────────┐
    │                         │
  🟢 ACK Received         🔴 No ACK
    │                         │
[Node] Measurement        [Node] ESP-NOW
acknowledged at radio     delivery failed
level                         │
    │                         │
    └────────────┬────────────┘
                 │
         (Both paths: check for RESET_REQUEST from hub for 1 second)
                 │
         ┌───────┴───────┐
         │               │
    🟢 Idle      🔴 Reset received
         │               │
         │        [Node] Pairing cleared;
         │        searching for a hub
         │               │
         └───────┬───────┘
                 │
┌────────────────────────────────────────────────────────────────────────────┐
│          🔘 BUTTON RESET CHECK (D0 held at boot)                           │
└────────────────────────────────────────────────────────────────────────────┘

If node is ALREADY PAIRED and D0 button is HELD:
                 │
         [Node] Hold D0 for 10 seconds,
         then release to clear pairing
                 │
         (Button held continuously)
                 │
         At 5 seconds:
         [Node] Release D0 to clear pairing
                 │
         ┌───────┴──────────┐
         │                  │
    Released?        Still holding?
         │                  │
    🟢 YES                🔴 YES
    (5-10s)         (10+ seconds)
         │                  │
    [Node] Pairing      [Node] Pairing reset
    cleared; searching  canceled
    for a hub               │
         │                  │
         └────────────┬─────┘
                      │
         (Both paths: proceed with measurement send)

┌──────────────────────────────────────────────────────────────────────────────┐
│                          💤 DEEP SLEEP                                       │
└──────────────────────────────────────────────────────────────────────────────┘

[Node] Sleeping for 30 minutes                        ← Serial stops here
  │
  ├─ Serial.flush() called
  ├─ WiFi stopped (esp_wifi_stop())
  ├─ Timer wakeup set (30 min = 1,800,000 seconds)
  └─ esp_deep_sleep_start() invoked
      │
      └─ CPU powered down
         Timer module only stays powered (~100 µA)
         Next wakeup: ~30 minutes later
         Next serial output: "[Node X] Starting" at boot

```

---

## Expected Serial Messages by Device State

### STATE 1: Initialization ✓
```
[Node 1] Starting
[Node] Reading battery and SHT40
[Node 1] SHT40: 72.50 F, 45.23 %RH
```
**What it means**: Device started, sensor initialized, readings taken
**Expected duration**: ~1 second
**Next state**: Pairing check

---

### STATE 2A: First-Time Pairing ✓
```
[Node] Searching for a hub (up to 30 seconds)
```
**What it means**: No hub found in NVS storage; starting channel scan
**Expected duration**: 30 seconds (or less if hub found)
**What happens**: Sends PAIR_REQUEST on each Wi-Fi channel; waits for PAIR_RESPONSE

---

### STATE 2B: Using Saved Hub ✓
```
[Node] Using saved hub on channel 6
```
**What it means**: Hub MAC and channel found in NVS storage from prior pairing
**Expected duration**: <100 ms
**Next state**: Measurement send (skips discovery)

---

### STATE 3: Hub Discovered ✓
```
[Node] Paired with hub on channel 6
```
**What it means**: PAIR_RESPONSE received from hub on this channel
**Expected duration**: Usually 5-15 seconds (depends on when hub is on this channel)
**What happens next**:
1. Hub MAC saved to NVS storage
2. Node adds hub as ESP-NOW peer
3. Proceeds to send measurement

---

### STATE 4: Sending Measurement ✓
```
[Node] Measurement acknowledged at radio level
```
**What it means**: SensorPacket delivered to hub via ESP-NOW
**Expected duration**: <100 ms
**Important**: This is radio-level ACK; hub still needs to process and publish

---

### STATE 5: Checking for Reset ✓
(Usually silent; waits 1 second for RESET_REQUEST from hub)

**If hub sent reset**:
```
[Node] Pairing cleared; searching for a hub
```
**What it means**: Hub commanded pairing reset
**Next boot**: Will scan all channels (first-pairing mode)

---

### STATE 6: Deep Sleep ✓
```
[Node] Sleeping for 30 minutes
```
**What it means**: 
- Measurement cycle complete
- About to power down
- Serial port stops responding (CPU off)
- Wakes in ~30 minutes

---

## Pairing Success Indicators

### ✅ SUCCESS Checklist

Look for **ALL** of these in order:

```
[Node 1] Starting                                       ✅ Boot detected
[Node 1] SHT40: 72.50 F, 45.23 %RH                    ✅ Sensor OK
[Node] Searching for a hub (up to 30 seconds)         ✅ Discovery starting
[Node] Paired with hub on channel 6                    ✅ Hub found and channel saved
[Node] Measurement acknowledged at radio level         ✅ Data sent
[Node] Sleeping for 30 minutes                         ✅ Ready for next cycle
```

**Result**: Node will appear in Hub's Blynk app within 5-10 seconds ✅

---

### ❌ FAILURE Indicators

| Message | Problem | Fix |
|---------|---------|-----|
| `[Node] SHT40 initialization failed` | I2C not responding | Check SDA/SCL wiring |
| `[Node] ESP-NOW initialization failed` | Radio won't start | Reflash firmware |
| `[Node] Searching for hub` → `[Node] No hub found` | Hub offline or wrong channel | Power-cycle hub; check Wi-Fi channel in serial |
| `[Node] ESP-NOW delivery failed` | Hub not responding on channel | Hub may have changed Wi-Fi channel; re-pair |
| `[Node] Failed to save hub pairing` | Storage corrupted | Erase NVS; reflash |
| No `[Node] Measurement acknowledged` | Packet not reaching hub | Check hub is listening (pairing window open?) |

---

## Serial Output Timing

| Phase | Expected Duration | Serial Activity |
|-------|-------------------|-----------------|
| Boot → Sensor read | ~500 ms | 2-3 messages |
| Pairing discovery | 5-30 seconds | Occasional "Discovery send failed" (normal) |
| Measurement send + ACK wait | ~50-100 ms | 1 message |
| Reset check wait | ~1000 ms | (Silent) |
| Deep sleep entry | ~100 ms | 1 message, then stops |
| **Total active time** | **5-35 seconds** | Node draws ~80-150 mA |

---

## Detailed First-Pairing Walkthrough

### Scenario: Brand New Node Powers Up

**Terminal output (live watching @ 115200 baud)**:

```
18:30:45 [Node 2] Starting
18:30:45 [Node] Reading battery and SHT40
18:30:46 [Node 2] SHT40: 72.50 F, 45.23 %RH          ← Sensor good
18:30:46 [Node] Searching for a hub (up to 30 seconds) ← No hub stored; starting scan
18:30:47 [Node] Discovery send failed: ...             ← Normal on channel switches
18:30:52 [Node] Discovery send failed: ...             ← Trying different channel
18:31:08 [Node] Paired with hub on channel 11          ← FOUND! ✅
18:31:08 [Node] Measurement acknowledged at radio level ← Sent! ✅
18:31:09 [Node] Sleeping for 30 minutes               ← Done
```

**Analysis**:
- Boot to discovery: ~1 second ✓
- Discovery to pairing: ~22 seconds (depends on when hub is on correct channel)
- Total cycle: ~24 seconds ✓
- Node now paired and will re-use channel 11 on next boot
- Hub should show Node 2 in app within 5-10 seconds with 72.50°F, 45.23% RH

---

## Serial Output by Hub Events (for reference when checking logs)

When a node sends data to hub, you'll see on **HUB SERIAL**:

```
[Hub] Node 2 report: 72.50 F, 45.23 %RH
[Blynk] ◀ virtualWrite(V1, 72.50)
[Blynk] ◀ virtualWrite(V5, 45.23)
...
[Blynk] ◀ endGroup()
```

This confirms hub received the measurement and published to Blynk ✓

