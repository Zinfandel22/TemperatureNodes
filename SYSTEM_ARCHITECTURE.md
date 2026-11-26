# Complete System Architecture

## Overview

You have a **distributed IoT monitoring system** with three components:

```
┌─────────────────────────────────────────────────────────────┐
│                    Blynk Cloud                              │
│         (Central Data Repository & API Server)              │
└─────────────────────────────────────────────────────────────┘
              ↑ publishes          ↓ fetches
      (25 writes, batched)   (single HTTPS)
              │                     │
    ┌─────────┴───────┐    ┌────────┴──────────┐
    │                 │    │                   │
┌───▼──────────────┐  │  ┌─▼──────────────────▼┐
│ Temperature Nodes│  │  │ Temperature Orbs    │
│ (Hub ESP32)      │  │  │ (Display ESP32)     │
├──────────────────┤  │  ├─────────────────────┤
│ • Local SHT40    │  │  │ • 5 Round Displays  │
│ • 4 Remote Nodes │  │  │ • LVGL UI           │
│ • Publishes V0-33│  │  │ • Fetches /getAll   │
│ • ESP-NOW RX     │  │  │ • Displays data     │
└──────────────────┘  │  └─────────────────────┘
                      │
           ┌──────────┴──────────┐
           │  Mobile/Web Clients │
           │  (Blynk App)        │
           └─────────────────────┘
```

## Component Details

### 1. Temperature Nodes (Hub)

**Purpose**: Collect sensor data and publish to Blynk

**Hardware**:
- Seeed Studio XIAO ESP32S3
- SHT40 temperature/humidity sensor (local)
- ESP-NOW receiver for remote nodes

**Publishing Pattern** (with protocol-level batching):
```cpp
loopHub() {
    beginGroup();                    // Batch marker "b"
    
    // All 25+ virtualWrite() calls here
    // - Node 1–4 temps (V0–V3)
    // - Node 1–4 humidity (V4–V7)
    // - Hub local temp/humidity (V8–V9)
    // - Contact times (V10–V14)
    // - Battery voltages (V20–V23)
    // - Battery percentages (V30–V33)
    
    endGroup();                      // Batch marker "e"
    // Result: All writes grouped with shared timestamp
}
```

**Data Flow**:
1. Collects sensor data from local SHT40 and four ESP-NOW nodes
2. Tracks node contact times and offline status
3. Groups all writes within `beginGroup()` / `endGroup()`
4. Publishes to Blynk—all datastreams get same server timestamp
5. Repeats every 30–60 seconds

### 2. Temperature Orbs (Display)

**Purpose**: Display hub sensor data on beautiful round screens

**Hardware**:
- Seeed Studio XIAO ESP32S3
- 5× GC9A01 240×240 round LCD displays
- SPI shared data/clock, separate chip-select per display

**Configuration via Virtual Pins**:
- **V101**: Hub AUTH Token (user-entered, read-only External API token from hub)
- **V99**: Hub connection status ("ONLINE", "OFFLINE", "ERROR")

**Data Fetching**:
```cpp
String hubAuthToken = "";  // Read from V101

BLYNK_WRITE(V101) {
    hubAuthToken = param.asString();  // User enters Hub's token
    Serial.println("Hub token updated: " + hubAuthToken);
}

bool fetchHubReadings() {
    if (hubAuthToken.isEmpty()) {
        Blynk.virtualWrite(V99, "ERROR: No hub token");
        return false;
    }
    
    // Single HTTPS request to Blynk API
    GET /external/api/getAll?token=HUB_TOKEN
    
    // Receive JSON: { "v0": 72.5, "v1": 71.8, ... }
    // Parse and store values
    // Update all 5 displays with consistent snapshot
    
    Blynk.virtualWrite(V99, "ONLINE");
}
```

**Display Mapping**:
- Orb 1: Hub local sensors (V8, V9)
- Orb 2: Node 1 (V0, V4, V10, V30)
- Orb 3: Node 2 (V1, V5, V11, V31)
- Orb 4: Node 3 (V2, V6, V12, V32)
- Orb 5: Node 4 (V3, V7, V13, V33)

### 3. Blynk Cloud

**Services**:
- `/external/api/getAll?token=X` – Fetch all datastream values (used by Orbs)
- `/external/api/batch/update?token=X&V0=val0&...` – Batch HTTP API (not used in this implementation)
- Device provisioning and authentication
- Data storage with timestamps

**Data Storage**: 18+ datastreams per hub device with shared timestamp when published within batch markers

---

## Batching Implementation

### Why Batching Matters

**Without batching**, the hub sends 25+ virtualWrite calls sequentially over ~100 ms:
- Each call has slightly different server timestamp
- Orbs fetch `/getAll` and might receive a mix of old/new values
- Displays show inconsistent snapshots (some fresh data, some stale)

**With protocol-level batching**:
- All writes are sent between `beginGroup()` and `endGroup()` markers
- Blynk server groups them and assigns a single shared timestamp
- Orbs always fetch a coherent snapshot from one moment in time
- All displays show consistent, synchronized data

### Implementation Details

**Code changes**:
```cpp
// src/hub/LoggedBlynk.h – Wrapper class adds:
void beginGroup() { /* send batch marker "b" */ }
void endGroup()   { /* send batch marker "e", log count */ }

// src/main.cpp loopHub() – Wrapped publishing:
loggedBlynk.beginGroup();     // Line ~378
  // 25+ virtualWrite() calls
loggedBlynk.endGroup();       // Line ~477
```

**Serial output improvement**:

| Before | After |
|--------|-------|
| `[Blynk] V0 = 72.5` | `[Blynk] Batch: 25 datastreams` |
| `[Blynk] V4 = 45.2` | `with shared timestamp` |
| `[Blynk] V10 = ...` | (single line, much cleaner) |
| ... 20+ lines total | |

### Why Not HTTP Batch Endpoint?

Blynk offers `/external/api/batch/update?token=X&V0=val0&V1=val1&...` to batch writes in a single HTTP request. This would:
- ✅ Reduce network traffic
- ✅ Further reduce latency
- ❌ Require device auth token (not exposed by BlynkEdgent)
- ❌ Token is encrypted in NVS storage for security
- ❌ Extracting it would violate encapsulation

**Solution**: Protocol-level batching provides optimal results without security concerns.

---

## Orbs Configuration: Hub Token Setup

### How It Works

Orbs devices need the Hub's read-only AUTH token to fetch data via Blynk's External API. Instead of hardcoding secrets, the token is **entered once by the user in the Blynk app** via virtual pin V101:

```
User Setup Flow:
    ↓
User copies Hub's AUTH token from Blynk Console
    ↓
Opens Blynk app → Orbs device settings
    ↓
Enters token in V101 "Hub AUTH Token" field
    ↓
Orbs firmware reads V101 on startup and periodically
    ↓
Stores token in memory: String hubAuthToken
    ↓
Uses token for all /external/api/getAll requests
    ↓
Updates V99 "Hub Status" to "ONLINE" / "OFFLINE" / "ERROR"
    ↓
Fetches hub data every 10 seconds
    ↓
Display 5 round screens with synchronized data
```

### Implementation in Orbs Firmware

```cpp
#define V_HUB_TOKEN    101
#define V_HUB_STATUS   99

String hubAuthToken = "";

// Read Hub token when user enters it in app
BLYNK_WRITE(V_HUB_TOKEN) {
    hubAuthToken = param.asString();
    Serial.println("[Orbs] Hub token updated: " + hubAuthToken.substring(0, 8) + "...");
    Blynk.logEvent("hub_token_configured");
}

// Request saved token on connection
void onBlynkConnected() {
    Blynk.syncVirtual(V_HUB_TOKEN);  // Load saved token from cloud
}

// Fetch hub data using stored token
bool fetchHubReadings(HubReadings& readings) {
    if (hubAuthToken.isEmpty()) {
        Blynk.virtualWrite(V_HUB_STATUS, "ERROR: No hub token");
        return false;
    }
    
    HTTPSClient http;
    String path = "/external/api/getAll?token=" + hubAuthToken;
    
    if (!http.connect("blynk.cloud", 443)) {
        Blynk.virtualWrite(V_HUB_STATUS, "OFFLINE");
        return false;
    }
    
    http.print(String("GET ") + path + " HTTP/1.1\r\n" +
               "Host: blynk.cloud\r\n" +
               "Connection: close\r\n\r\n");
    
    if (parseHubData(http, readings)) {
        Blynk.virtualWrite(V_HUB_STATUS, "ONLINE");
        return true;
    }
    
    Blynk.virtualWrite(V_HUB_STATUS, "ERROR");
    return false;
}
```

### User Setup Instructions

1. **Get Hub's AUTH token**:
   - Sign in to [Blynk.Console](https://blynk.cloud/dashboard/login)
   - Find your Temperature Nodes Hub device
   - Click device → **Device info**
   - Copy the **Auth Token** (32-character string)

2. **Enter token into Orbs device**:
   - Open Blynk IoT mobile app
   - Navigate to your Temperature Orbs device
   - Tap **Settings** (gear icon)
   - Find **V101 "Hub AUTH Token"**
   - Paste the Hub's token
   - Tap Save/Confirm

3. **Verify connection**:
   - Orbs will automatically fetch hub data
   - V99 "Hub Status" should show "ONLINE"
   - All 5 displays update with hub values

4. **To change hub later**:
   - Just update V101 with new Hub's token
   - **No firmware reflash needed** – changes immediately

---

## Communication Flow

### Hub Publishing Cycle

```
Data Collection
    ↓
Check pending ESP-NOW packets
Check local SHT40
Check node offline status
    ↓
Blynk Library Layer
    ↓
beginGroup()           ← Send batch marker "b"
    ↓
25+ virtualWrite() calls via WiFi
    ↓
endGroup()            ← Send batch marker "e"
    ↓
Blynk Cloud Server
    ↓
Store with shared timestamp
(All writes assigned same server time)
```

**Frequency**: Every 30–60 seconds (measured from start of deep sleep)

### Orbs Fetching Cycle

```
Timer fires (every 10 seconds)
    ↓
Check WiFi & NTP sync
    ↓
HTTPS GET /external/api/getAll?token=HUB_TOKEN
    ↓
Blynk Cloud returns JSON
    ↓
Parse JSON → HubReadings struct
    ↓
Update all 5 displays
    ↓
Display updated values + timestamp
```

**Frequency**: Every 10 seconds

---

## Serial Output Examples

### Hub (Before Batching)
```
[Hub] Node 1 report: 72.50 F, 45.20 %RH
[Blynk] V0 = 72.50
[Blynk] V4 = 45.20
[Blynk] V10 = ONLINE 10:23 PM
[Blynk] V20 = 3.21
[Blynk] V30 = 84
[Hub] Node 2 report: 71.80 F, 46.30 %RH
[Blynk] V1 = 71.80
...
← 20+ lines (verbose and hard to scan)
```

### Hub (After Batching)
```
[Hub] Node 1 report: 72.50 F, 45.20 %RH
[Hub] Node 2 report: 71.80 F, 46.30 %RH
[Blynk] Event low_battery = Node 1 low: 3.05V
[Blynk] Batch: 25 datastreams with shared timestamp
← 4 lines (clean and clear)
```

### Orbs (Unchanged)
```
Hub data check starting...
Waiting for time synchronization
Connected to Blynk
Hub HTTPS request complete (200)
Received: V0, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V30, V31, V32, V33
Hub data received and displayed
```

---

## Network & Performance

### Hub to Blynk

**Data per cycle**:
- 25 `virtualWrite()` commands, each ~200 bytes
- Total: ~5 KB per 30–60 second cycle
- Bandwidth: ~100 bytes/second

**With/without batching**:
- **Without**: 25 separate TCP connections, 25 packet events
- **With**: Same 25 commands, one TCP session, batch markers "b" and "e"
- **Result**: Better efficiency, same bandwidth, shared timestamp

### Orbs to Blynk

**Data per fetch**:
- Single HTTPS request: ~1 KB response
- Frequency: Every 10 seconds
- Bandwidth: ~100 bytes/second

**Result**: Unchanged (Orbs always did single fetch)

---

## Data Integrity & Reliability

### Consistency Improvements

✅ **With batching**:
- All datastreams receive shared server timestamp
- Orbs always see coherent snapshot
- No "partial updates" mixing old/new data

### Error Handling

✅ **Unchanged**:
- Orbs detect missing/incomplete values and show placeholders
- Hub detects node offline (90+ minutes without report)
- Offline events only sent once per node transition
- Both log errors to serial

### Failover

✅ **Same**:
- Hub: If Blynk connection fails, retries automatically
- Orbs: If fetch fails, keeps displaying cached values, marks as "stale"
- Both: Reconnect when network recovers

---

## Verification Checklist

### Hub Side
- [ ] Serial shows: `[Blynk] Batch: N datastreams with shared timestamp`
- [ ] One batch message per update cycle
- [ ] Demo data populates correctly on first run
- [ ] Contact times update periodically

### Orbs Side
- [ ] All 5 displays show data
- [ ] Updates every ~10 seconds
- [ ] No "Hub data stale" message after initial sync
- [ ] Battery percentages are 0–100%
- [ ] Contact times match (within ~1 minute)

### Integration
- [ ] Hub publishes, Orbs display same values
- [ ] No data gaps or inconsistencies
- [ ] Demo mode on Hub shows on Orbs
- [ ] Node offline detection works

---

## Specifications

### Hub Datastreams (18 total)
- **V0–V3**: Node 1–4 temperatures (°F, double)
- **V4–V7**: Node 1–4 humidity (%RH, double)
- **V8–V9**: Hub local temp/humidity (double)
- **V10–V13**: Node 1–4 contact time (string, HH:MM AM/PM)
- **V14**: Hub local sensor contact time (string)
- **V20–V23**: Node 1–4 battery voltage (V, double)
- **V30–V33**: Node 1–4 battery percentage (%, integer)
- **V40**: Reset pairing (integer, 0–1)
- **V41**: Start pairing (integer, 0–1)
- **V100**: Demo mode toggle (integer, 0–1)

### Orbs Datastreams (Configuration & Status)
- **V0–V9**: Display data (mirrored from hub—temperatures, humidity, hub local)
- **V30–V33**: Battery percentages (mirrored from hub)
- **V10–V14**: Contact times (mirrored from hub)
- **V99**: Hub connection status (string: "ONLINE", "OFFLINE", "ERROR")
- **V101**: Hub AUTH Token (string, user-entered, 0–128 characters)

### Complete Orbs Template Configuration

| Virtual Pin | Name | Type | Purpose | Direction |
|-------------|------|------|---------|-----------|
| V0–V9 | Display values | Double/String | Hub sensor readings | Read (from hub API) |
| V30–V33 | Battery % | Integer | Node battery levels | Read (from hub API) |
| V10–V14 | Contact times | String | Last reading times | Read (from hub API) |
| **V99** | **Hub Status** | **String** | **Connection state** | **Write** (Orbs reports) |
| **V101** | **Hub AUTH Token** | **String** | **API token for /getAll** | **Read** (User enters) |

### Update Frequencies
- **Hub publish**: 30–60 seconds (from start of deep sleep)
- **Orbs fetch**: 10 seconds
- **Node report**: ~30 minutes (nodes sleep between readings)

### Hardware Requirements
- **Hub**: ESP32-S3, Wi-Fi 2.4 GHz, Blynk connection, ESP-NOW RX
- **Orbs**: ESP32-S3, 5× SPI displays, Wi-Fi 2.4 GHz, TLS/SSL support

### Dependencies
- **Hub**: BlynkEdgent 0.9.0, Blynk 1.4.0
- **Orbs**: BlynkEdgent, ArduinoJson, TFT_eSPI, LVGL 8, ESP_SSLClient

---

## Future Enhancements

### Short Term
1. ✅ Implement batch grouping (DONE)
2. Add more nodes (beyond 4)
3. Add additional sensors (pressure, light, motion)

### Medium Term
1. Implement HTTP batch endpoint (if token access available in future)
2. Add data logging and historical views
3. Add predictive low-battery alerts

### Long Term
1. Add multiple display sets (more orbs)
2. Network time server override
3. Custom UI themes and animations

---

## Conclusion

The Temperature Nodes & Orbs system is a **well-designed distributed IoT platform**:

- **Nodes** efficiently collect and publish data
- **Hub** aggregates and publishes with protocol-level batching
- **Orbs** consume and display beautifully with consistent timestamps
- **Blynk** provides secure cloud backbone
- **Batching** ensures data consistency and clean observability
