# SR850 ASIO bridge — release gate (NOT READY)

The existing System Audio / Equalizer APO application is separate from the proposed universal ASIO bridge. A green installer workflow does **not** mean the virtual ASIO driver works.

## Current verified scope
- Native FIR, virtual stream and simulated callback/buffer transport compile and pass their Windows/Linux tests.
- The Windows System Audio installer builds; it does not install a working virtual ASIO bridge.
- COM activation of the existing M-Audio Fast Track driver was reported successful, but no physical audio stream has been started or tested.

## Hard blockers to a usable TONEX / AmpliTube / Guitar Rig bridge
1. Obtain the official ASIO SDK legally and compile/link the virtual IASIO implementation against its exact ABI on Windows x64 and x86. The optional SDK workflow cannot pass without separately supplied headers.
2. Implement an actual COM-visible virtual IASIO driver DLL: QueryInterface, class factory, DLL exports, safe reference counting, registration/unregistration in ASIO discovery and both architectures.
3. Implement host callback execution driven by a physical audio clock; current VirtualAsioDriver::start() deliberately returns ASE_NotPresent. The COM class factory currently returns E_NOINTERFACE.
4. Connect physical Fast Track input to virtual host input unchanged, virtual host output through SR850 FIR to physical playback, handling physical buffer formats, rates, resync, callback lifetime, underruns and stop/restart safely.
5. Measure real-world latency, channel mapping and output unity gain; test with TONEX, AmpliTube and Guitar Rig.
6. Test legacy M-Audio driver stability on the user's machine with explicit opt-in, a recovery plan and a safe fallback. Its historical BSOD risk makes automatic hardware tests unacceptable.
7. Only then build and sign/package a separate ASIO installer, preserving the existing approved GUI and Equalizer APO installation.

## Safety policy
Do not publish the bridge as working, register a nonfunctional virtual ASIO driver, or start the physical Fast Track automatically. Existing audio setup must remain untouched until the bridge passes these release gates.
