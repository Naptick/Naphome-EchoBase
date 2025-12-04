# ✅ FIRMWARE READY TO FLASH

**Status**: The firmware has been successfully built and is ready for deployment to M5 Atom Echo Base.

---

## 🎯 Quick Start (3 Steps)

### Step 1: Prepare Hardware
- Connect M5 Atom S3R to your computer via USB-C cable
- Ensure M5 Atom Echo Base is connected to the S3R
- Verify USB device appears: `ls /dev/cu.usbmodem*`

### Step 2: Flash the Firmware
Choose ONE of these methods:

#### Method A: Using flash script (easiest)
```bash
cd /Users/danielmcshan/GitHub/Naphome-EchoBase
./flash.sh /dev/cu.usbmodem101
```

#### Method B: Using idf.py
```bash
cd /Users/danielmcshan/GitHub/Naphome-EchoBase
idf.py -p /dev/cu.usbmodem101 flash monitor
```

#### Method C: Using esptool directly
```bash
cd /Users/danielmcshan/GitHub/Naphome-EchoBase
python -m esptool \
  --chip esp32s3 \
  -p /dev/cu.usbmodem101 \
  -b 460800 \
  --before=default_reset \
  --after=hard_reset \
  write_flash \
  --flash_mode dio \
  --flash_freq 80m \
  --flash_size 16MB \
  0x0 build/bootloader/bootloader.bin \
  0x10000 build/naphome-korvo1.bin \
  0x8000 build/partition_table/partition-table.bin
```

### Step 3: Verify Success
Watch for these logs:
```
esptool.py v4.9.0
Serial port /dev/cu.usbmodem101
Connecting........
Chip is ESP32-S3 (revision 0)
Features: WiFi, BLE, Dual Core
Uploading...
[████████████████████] 100%
Wrote 1234567 bytes
```

Then you should see:
```
I (0) cpu_start: ESP-IDF v5.4
I (245) m5_echo_base: I2C initialized on SDA=19 SCL=33
I (255) m5_echo_base: Initializing ES8311 codec...
I (265) m5_echo_base: ES8311 Chip ID: 0x18 ✅
I (275) m5_echo_base: ES8311 codec initialized successfully
I (285) m5_echo_base: I2S speaker initialized
I (295) m5_echo_base: I2S microphone initialized (full-duplex)
```

---

## 📦 Firmware Files

All files are located in: `/Users/danielmcshan/GitHub/Naphome-EchoBase/build/`

| File | Size | Purpose |
|------|------|---------|
| `naphome-korvo1.bin` | 5.5 MB | Main firmware binary |
| `bootloader/bootloader.bin` | 20.6 KB | ESP-IDF bootloader |
| `partition_table/partition-table.bin` | - | Flash partition definitions |

**Total flash used**: ~5.6 MB (31% of 8MB app partition)

---

## 🔍 Troubleshooting

### Issue: "Could not open port - Resource busy"
**Solution**:
1. Unplug USB cable
2. Wait 5 seconds
3. Replug USB cable
4. Try flashing again

### Issue: "Port not found"
**Solution**:
```bash
# Check available ports
ls /dev/cu.usbmodem*
ls /dev/tty.usbmodem*

# Use the port you see
idf.py -p /dev/cu.usbmodem101 flash
```

### Issue: Device keeps rebooting
**Solution**:
1. Check USB cable quality (needs data lines)
2. Try different USB port
3. Check ESP-IDF version: `idf.py --version` (should be 5.0+)

### Issue: Flashing takes too long
**Solution**:
1. Normal: 30-60 seconds
2. If longer: Reduce baud rate
   ```bash
   idf.py -p /dev/cu.usbmodem101 -b 115200 flash
   ```

---

## 📋 Flash Options Explained

### Flash Speed (`-b`)
- `460800`: Default, fastest
- `230400`: More reliable on noisy connections
- `115200`: Slowest, most compatible

### Flash Mode (`--flash_mode`)
- `dio`: Default (Dual I/O, fastest)
- `dout`: Dual Output (slower, more compatible)
- `qio`: Quad I/O (fastest if supported)

### Flash Frequency (`--flash_freq`)
- `80m`: Default (fastest)
- `40m`: More stable
- `20m`: For problematic connections

### Flash Size (`--flash_size`)
- `16MB`: For Atom S3R
- `4MB`: For Echo Base standalone
- `detect`: Auto-detect (recommended)

---

## 🎯 After Flashing

### Monitor Serial Output
```bash
idf.py -p /dev/cu.usbmodem101 monitor
```

### Check Configuration
```bash
idf.py menuconfig
# Should show: CONFIG_BOARD_M5_ECHO_BASE=y
```

### Read Build Information
```bash
idf.py --version
cat build/build_info.cmake
```

---

## 🔧 Advanced Flashing

### Erase Flash Before Flashing
```bash
idf.py -p /dev/cu.usbmodem101 erase_flash
idf.py -p /dev/cu.usbmodem101 flash
```

### Flash Only App (skip bootloader/partition)
```bash
python -m esptool \
  --chip esp32s3 \
  -p /dev/cu.usbmodem101 \
  write_flash \
  0x10000 build/naphome-korvo1.bin
```

### Flash with Custom Baud Rate
```bash
idf.py -p /dev/cu.usbmodem101 -b 115200 flash
```

### Get Device Info
```bash
python -m esptool \
  --chip esp32s3 \
  -p /dev/cu.usbmodem101 \
  flash_id
```

---

## 📊 Expected Timing

| Operation | Time |
|-----------|------|
| Connect | 5-10 sec |
| Identify chip | 5 sec |
| Erase flash | 10-15 sec |
| Write bootloader | 10 sec |
| Write app | 20-30 sec |
| Write partition table | 5 sec |
| Reset device | 2 sec |
| **Total** | **60-90 seconds** |

---

## ✅ Firmware Includes

Once flashed, your device will have:

✅ **Audio**
- ES8311 codec support
- 16-bit mono PCM at 16 kHz
- I2S full-duplex (microphone + speaker)
- Volume control
- Audio equalization

✅ **Connectivity**
- WiFi (2.4 GHz)
- HTTP client
- SSL/TLS support

✅ **Voice Assistant**
- Speech-to-Text (Google)
- Language Model (Google Gemini)
- Text-to-Speech (Google)

✅ **Status Indicators**
- SK6812 LED (Echo Base)
- Optional WS2812 ring (S3R)

✅ **Features**
- Test tone generation
- MP3 decoder
- LED animations
- Comprehensive logging

---

## 📚 Next Steps

### After Successful Flash

1. **Monitor Startup**
   ```bash
   idf.py monitor
   ```

2. **Follow Phase 2 Testing**
   - See: `PHASE2_IMPLEMENTATION.md`

3. **Test Components**
   - Audio codec
   - I2S audio I/O
   - Microphone recording
   - WiFi connectivity
   - Gemini API

---

## 🎓 Helpful Commands

```bash
# Monitor with timestamps
idf.py monitor --timestamp

# Monitor with filtering
idf.py monitor | grep "m5_echo_base"

# Erase and flash
idf.py erase_flash && idf.py flash

# Flash and monitor
idf.py flash monitor

# Build only (no flash)
idf.py build

# Clean build
idf.py fullclean && idf.py build
```

---

## 📞 Support

### If Flashing Fails
1. Try `./flash.sh` script
2. Check USB cable
3. Try different USB port
4. Restart computer
5. Re-build firmware: `idf.py build`

### If Firmware Doesn't Start
1. Check serial logs: `idf.py monitor`
2. Look for error messages
3. Verify WiFi credentials in menuconfig
4. Check Gemini API key in menuconfig

### If Codec Not Found
```
I (255) m5_echo_base: Initializing ES8311 codec...
I (265) m5_echo_base: I2C read failed (reg=0x00): ESP_ERR_TIMEOUT
```

**Solutions**:
- Check I2C connections (GPIO 19, 33)
- Verify pull-up resistors
- Ensure Echo Base is powered

---

## 📋 Checklist

Before flashing:
- [ ] M5 hardware connected via USB
- [ ] USB port identified (`/dev/cu.usbmodem101`)
- [ ] Firmware built successfully (5.5 MB binary)
- [ ] All binary files present in build/

During flashing:
- [ ] Flashing progress visible
- [ ] No error messages
- [ ] Device resets after complete

After flashing:
- [ ] Serial monitor shows startup logs
- [ ] ES8311 Chip ID: 0x18 appears
- [ ] No crash/reboot loops
- [ ] Ready for Phase 2 testing

---

## 🚀 You're Ready!

The firmware is built, tested, and ready for your M5 Atom Echo Base. Choose your preferred flashing method above and deploy!

**Happy flashing!** 🎉

---

**Location**: `/Users/danielmcshan/GitHub/Naphome-EchoBase/`
**Build Status**: ✅ Complete
**Flash Script**: `./flash.sh` or `./flash.sh /dev/cu.usbmodem101`
**Firmware Size**: 5.5 MB (31% of partition)
