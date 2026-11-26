# Serial Debug Quick Lookup Guide

> **TL;DR**: When a node powers up and pairing starts, you'll see these messages in this order. Use this page to quickly find what you're looking for.

---

## 🔴 Node Powers Up

```
[Node 2] Starting
```

✅ **What it means**: Device just booted; determined it's a node (not hub)
📍 **Next**: SHT40 sensor initialization

---

## 🔄 Node Reads Sensor

```
[Node] Reading battery and SHT40
[Node 2] SHT40: 72.50 F, 45.23 %RH
```

✅ **What it means**: 
- Battery voltage read (220kΩ divider on A0)
- SHT40 temperature and humidity successfully read
- Values ready to send to hub

📍 **Next**: Check if hub is known from storage

---

## ⚠️ Sensor Problem?

```
[Node] SHT40 initialization failed; sending invalid sensor readings
```

❌ **What it means**: SHT40 not responding on I2C bus
🔧 **Fix**:
1. Check SDA/SCL wiring (GPIO 5 & 6)
2. Check sensor has 3.3V power
3. Try reseating sensor
4. Reflash firmware

📍 **Next**: Still proceeds with -999 as sensor values

---

## 🔎 Node Searches for Hub (First Time)

```
[Node] Searching for a hub (up to 30 seconds)
```

✅ **What it means**: 
- No hub MAC stored locally
- Starting channel scan (broadcasts on Ch1, Ch2, ... Ch14)
- Will wait up to 30 seconds for hub response

📍 **Next**: Discovery retries on each channel

---

## 🛰️ Node Discovers Hub

```
[Node] Paired with hub on channel 11
```

✅ **What it means**: 
- Hub PAIR_RESPONSE received on this channel ✅
- Hub MAC and channel saved to NVS storage
- Ready to send measurement

📍 **Next**: Send sensor packet to hub

---

## 📊 Measurement Sent Successfully

```
[Node] Measurement acknowledged at radio level
```

✅ **What it means**: 
- SensorPacket delivered to hub
- ESP-NOW callback fired (got radio ACK)
- Hub received it on its MAC address

📍 **Next**: Check for reset request (1 second wait)

---

## ❌ Measurement Failed to Send

```
[Node] ESP-NOW delivery failed
```

❌ **What it means**: 
- Hub not responding after 3 retries
- Packet never reached hub MAC
- Possible causes:
  - Hub offline
  - Hub wrong channel (changed Wi-Fi)
  - Radio interference

🔧 **Fix**:
1. Check hub is powered and running
2. Check hub has pairing window open (V41 = 1)
3. Check hub serial shows: `[Hub] Waiting for node pairing`
4. Try restarting node while pairing open

📍 **Next**: Still sleeps; will retry in 30 minutes

---

## 🔘 Button Reset Detected

```
[Node] Hold D0 for 10 seconds, then release to clear pairing
```

✅ **What it means**: D0 button detected held down at boot
📍 **User action needed**: Hold for 5-10 seconds, then release

---

## ✅ Button Reset Successful

```
[Node] Release D0 to clear pairing
(User releases button at 7 seconds)
[Node] Pairing cleared; searching for a hub
```

✅ **What it means**: 
- Stored hub MAC deleted
- Next boot: will search for ANY hub
- Begins discovery immediately

📍 **Next**: Discovery starts (up to 30 seconds)

---

## ❌ Button Reset Canceled

```
[Node] Pairing reset canceled
```

❌ **What it means**: 
- User held button for 10+ seconds
- Reset NOT executed
- Pairing still in storage

📍 **Next**: Proceeds with existing hub pairing

---

## 💤 Node Enters Sleep

```
[Node] Sleeping for 30 minutes
```

✅ **What it means**: 
- Boot cycle complete
- CPU powering down (deep sleep)
- RTC timer running (can wake in 30 min)
- Serial port STOPS responding here

🔋 **Power consumption**: ~100 µA while sleeping

📍 **Next**: Wake in ~30 minutes via timer

---

## 🟢 HUB Starts Up

```
[Hub] Starting
[Hub] Starting Blynk Edgent
[Hub] Waiting for Blynk provisioning
```

✅ **What it means**: 
- Hub booted successfully
- Blynk Edgent ready for BLE provisioning
- Waiting for user to provision via app

📍 **Next**: User opens Blynk app and scans QR code

---

## 📱 Hub Provisioning Started

```
[Hub] Wi-Fi connecting
```

✅ **What it means**: User entered Wi-Fi SSID/password in app

📍 **Next**: Attempting connection to Wi-Fi network

---

## 🌐 Hub Connected to Wi-Fi

```
[Hub] Wi-Fi connected to access point; waiting for IP address
[Hub] Station MAC: AA:BB:CC:DD:EE:FF, Wi-Fi channel: 6
```

✅ **What it means**: 
- Successfully joined Wi-Fi network
- Station MAC shown (hub's Wi-Fi address)
- Using channel 6 (nodes will search this channel)
- Waiting for DHCP IP address

📍 **Next**: DHCP lease acquired

---

## ☁️ Hub Connected to Blynk

```
[Hub] Blynk connected; open a pairing window with V41 or the D0 button
```

✅ **What it means**: 
- TLS connection to Blynk servers established ✅
- Hub is provisioned and online
- Ready to receive nodes
- Ready to publish data

📍 **Next**: User opens pairing window (V41 or D0)

---

## 👂 Hub Pairing Window Open

```
[Hub] Waiting for node pairing (window open for 2 minutes)
```

✅ **What it means**: 
- V41 button was pressed (or D0 clicked)
- Pairing window OPEN for 120 seconds
- Hub listening for PAIR_REQUEST from nodes
- Nodes can now be discovered

📍 **Next**: Nodes can pair during this 2-minute window

---

## 🚪 Hub Pairing Window Closes

```
[Hub] Pairing window closed
```

✅ **What it means**: 
- 120 seconds elapsed since window opened
- Window CLOSED
- Nodes can no longer pair until window opens again

📍 **Next**: Nodes must wait for next pairing window

---

## 📥 Hub Receives Node Data

```
[Hub] Node 1 report: 72.50 F, 45.23 %RH
```

✅ **What it means**: 
- Hub received SensorPacket from Node 1
- Temperature: 72.50°F
- Humidity: 45.23%
- Next: Hub publishes to Blynk

📍 **Next**: Blynk virtualWrite calls

---

## 📤 Hub Publishes to Blynk (Batch)

```
[Blynk] ◀ virtualWrite(V0, 72.50)
[Blynk] ◀ virtualWrite(V4, 45.23)
...
[Blynk] ◀ virtualWrite(V10, "03:16 PM")
...
[Blynk] ◀ endGroup()
```

✅ **What it means**: 
- 25+ virtualWrite calls batched together
- All writes get same server timestamp
- Orbs see consistent data when fetching
- One transaction to Blynk cloud

📍 **Next**: Data visible in Blynk app

---

## 📵 Node Goes Offline

```
[Hub] Node 2 offline after 3 missed reports
[Blynk] logEvent("node_offline", "Node 2 offline after 3 missed reports")
```

❌ **What it means**: 
- Node hasn't reported for 90+ minutes
- Missed 3 × 30-min sleep cycles
- Likely causes:
  - Battery dead
  - Node powered off
  - Persistent radio issues

🔧 **Fix**:
1. Check node battery voltage
2. Power cycle node or restart with RST button
3. Check hub still online and pairing open
4. Wait for node's next wake-up (up to 30 min)

📍 **Next**: Blynk shows "OFFLINE HH:MM PM"

---

## 🔄 Pairing Reset Queued

```
[Hub] Pairing reset queued for all nodes
```

✅ **What it means**: 
- V40 button pressed in Blynk app
- All nodes have reset request queued
- Sent when each node next reports (up to 30 min per node)

📍 **Next**: Nodes receive reset on their next boot

---

## 🧊 Node Receives Reset Request

```
[Node 1] Pairing cleared; searching for a hub
```

✅ **What it means**: 
- Hub sent RESET_PAIRING packet
- Node deleted stored hub MAC
- Next boot: will search for hub (any hub)

📍 **Next**: Node sleeps; wakes in 30 min to search

---

---

## 🎯 Find Your Error Quickly

| Error Message | Meaning | Fix |
|---|---|---|
| `[Node] SHT40 initialization failed` | I2C not responding | Check GPIO 5/6 wiring |
| `[Node] ESP-NOW initialization failed` | Radio won't start | Reflash firmware |
| `[Node] No hub found; retrying...` | Hub offline/pairing closed | Power hub; open V41 |
| `[Node] ESP-NOW delivery failed` | Hub not responding | Check hub channel; re-pair |
| `[Node] Pairing requires active Wi-Fi` | Can't open pairing | Reconnect Wi-Fi first |
| `[Hub] Node X offline after 3 missed` | 90+ min no reports | Power cycle node; re-pair |

---

## ⏱️ Expected Timing

| Event | Duration | Notes |
|-------|----------|-------|
| **New Node Pairing** | 5-30 sec | Depends on when hub is on target channel |
| **Using Saved Hub** | <2 sec | Much faster! 12× better battery life |
| **Measurement Send** | <100 ms | Radio-level ACK |
| **Deep Sleep** | 30 min | No serial output during sleep |
| **Hub Provisioning** | ~30 sec | BLE + Wi-Fi connection + Blynk TLS |
| **Pairing Window** | 2 min | From V41 press to close |

---

## 📌 Bookmark This When Testing

**Troubleshooting Node?** → Jump to "Serial Output by Event" → Find your message

**Testing Pairing?** → Jump to "Hub Pairing Window Open" → Follow expected flow

**Hub Offline?** → Jump to "Node Goes Offline" → Check troubleshooting steps

**Unexpected Error?** → Jump to "Find Your Error Quickly" → Table lookup

---

## 🔗 Full Documentation

For complete details, see:
- **[SERIAL_DEBUG_REFERENCE.md](SERIAL_DEBUG_REFERENCE.md)** – Full message reference (19 KB)
- **[NODE_BOOT_SEQUENCE.md](NODE_BOOT_SEQUENCE.md)** – Flow diagram (11 KB)
- **[SERIAL_OUTPUT_EXAMPLES.md](SERIAL_OUTPUT_EXAMPLES.md)** – Real scenarios (16 KB)
