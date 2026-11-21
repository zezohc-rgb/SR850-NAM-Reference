# TONEX Standalone + SR850 correction (experimental)

Goal: route TONEX guitar input and headphone output through Windows WASAPI Shared so the already-installed SR850 Equalizer APO correction can process the playback. **This does not proxy the native M-Audio ASIO driver.** No driver replacement or native ASIO interception is implemented.

## Prerequisites
- Keep the working M-Audio driver installed. Do not uninstall or overwrite it.
- Equalizer APO must be installed on the **Fast Track playback endpoint** used by headphones; SR850 correction must be enabled and working there in Windows shared-mode playback.
- Install FlexASIO from its official GitHub releases: https://github.com/dechamps/FlexASIO/releases
- Identify exact input/output names with PortAudioDevices (FlexASIO tools). They must correspond to the Fast Track; do not assume the default Windows devices are correct.
- Back up any existing `%USERPROFILE%\FlexASIO.toml` before changing it.

## Initial FlexASIO.toml template
Copy `FlexASIO.toml.example` to `%USERPROFILE%\FlexASIO.toml`, replacing the device-name placeholders with names returned by PortAudioDevices. Use the same interface for input and output. WASAPI **shared**, not exclusive, is required on the output to reach APO processing. Input must remain uncorrected.

## Test (reversible)
1. Close TONEX. Ensure SR850 correction works on Fast Track Windows shared playback.
2. Install FlexASIO, configure the two exact devices, restart TONEX.
3. In TONEX Audio Settings choose **FlexASIO**, not M-Audio ASIO; set 48 kHz initially if Windows input and output endpoints are configured for 48 kHz.
4. Start with buffer 256 samples, then 128 if stable; if clicks/dropouts, try 512. These are trial values, not latency guarantees.
5. With guitar connected, verify clean input and headphone output. Compare SR850 Power ON/OFF at matched loudness. Do not use direct hardware monitoring for the comparison.
6. Measure round-trip latency and check CPU/dropouts. If worse than native ASIO, restore M-Audio ASIO in TONEX.

## Known limitations
- FlexASIO with WASAPI Shared can reach APO processing, but whether the SR850 filter actually processes TONEX depends on the selected Windows playback endpoint, APO installation, and driver behavior.
- WASAPI Shared may increase latency compared with native M-Audio ASIO. This is **not** a transparent ASIO-to-ASIO bridge.
- Full-duplex input/output can be unstable at small buffers, particularly on older hardware.
- No installer, automatic configuration, hardware test, or integration with SR850 Qt UI is claimed at this stage.
- A future native-ASIO bridge would require its own low-latency proxy driver/host architecture and real hardware validation.

Sources: https://github.com/dechamps/FlexASIO/blob/master/BACKENDS.md ; https://github.com/dechamps/FlexASIO/blob/master/CONFIGURATION.md
