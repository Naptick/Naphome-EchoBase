# API Key Configuration Guide

## Quick Setup

### Option 1: Using menuconfig (Recommended)

1. Run menuconfig:
   ```bash
   idf.py menuconfig
   ```

2. Navigate to:
   ```
   Korvo1 LED Audio Test → Voice Assistant Configuration
   ```

3. Set the following:
   - **Google Gemini API Key**: Your API key from Google Cloud Console
   - **Gemini Model**: `gemini-1.5-flash` (default) or `gemini-1.5-pro`
   - **WiFi SSID**: Your WiFi network name
   - **WiFi Password**: Your WiFi password

4. Save and exit (`S` to save, `Q` to quit)

5. Rebuild:
   ```bash
   idf.py build
   ```

### Option 2: Using sdkconfig.defaults

Add to `sdkconfig.defaults`:

```ini
CONFIG_GEMINI_API_KEY="your-api-key-here"
CONFIG_GEMINI_MODEL="gemini-1.5-flash"
CONFIG_WIFI_SSID="your-wifi-ssid"
CONFIG_WIFI_PASSWORD="your-wifi-password"
```

**Note**: Be careful not to commit API keys to git! Add `sdkconfig` to `.gitignore` if not already there.

## Getting Your API Key

1. Go to [Google Cloud Console](https://console.cloud.google.com/)

2. Create or select a project

3. Enable the required APIs:
   - **Cloud Speech-to-Text API**
   - **Generative Language API** (for Gemini)
   - **Cloud Text-to-Speech API**

4. Create API key:
   - Go to "APIs & Services" → "Credentials"
   - Click "Create Credentials" → "API Key"
   - Copy the API key
   - (Optional) Restrict the key to specific APIs for security

5. Use the API key in menuconfig or sdkconfig.defaults

## Security Best Practices

1. **Don't commit API keys to git:**
   - Add `sdkconfig` to `.gitignore`
   - Use `sdkconfig.defaults.example` for templates
   - Store keys in environment variables or secure storage

2. **Restrict API keys:**
   - In Google Cloud Console, restrict keys to specific APIs
   - Set application restrictions if possible
   - Use separate keys for development/production

3. **Store in NVS (Future Enhancement):**
   - Consider storing encrypted API keys in NVS
   - Implement runtime configuration via web interface
   - Use secure boot for production devices

## Testing

After configuring:

1. Build and flash:
   ```bash
   idf.py build flash monitor
   ```

2. Check logs for:
   - WiFi connection status
   - Voice assistant initialization
   - API key validation

3. Test wake word detection and voice commands

## Troubleshooting

### "WiFi SSID not configured"
- Set `CONFIG_WIFI_SSID` in menuconfig
- Verify WiFi credentials are correct

### "Gemini API key not configured"
- Set `CONFIG_GEMINI_API_KEY` in menuconfig
- Verify API key is correct (no extra spaces)
- Check that required APIs are enabled in Google Cloud Console

### "Failed to connect to WiFi"
- Verify SSID and password are correct
- Check WiFi network is in range
- Verify network supports 2.4GHz (ESP32-S3 doesn't support 5GHz)

### "Voice assistant initialization failed"
- Check WiFi is connected
- Verify API key is valid
- Check Google Cloud Console for API quotas/limits
- Review serial monitor for detailed error messages

## Current Configuration

Check your current configuration:

```bash
idf.py menuconfig
```

Navigate to: `Korvo1 LED Audio Test → Voice Assistant Configuration`

Or check `sdkconfig` file (but don't commit it if it contains API keys).
