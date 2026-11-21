# SR850 Universal Audio Bridge — native DSP groundwork

First code milestone: C++ interleaved stereo/multichannel FIR processor with independent per-channel histories, reset, bypass, and gain. It does not allocate or lock in the processing callback. Compile the standalone test on Windows with MSVC:
```
cl /std:c++17 /EHsc /W4 test_fir.cpp && test_fir.exe
```
Or GCC/Clang:
```
g++ -std=c++17 -O2 -Wall -Wextra -pedantic test_fir.cpp -o test_fir && ./test_fir
```

**Not yet a working ASIO driver or audio bridge.** Required next: licensed ASIO driver interface and registration, host compatibility, robust device discovery, duplex forwarding to native M-Audio ASIO, conversion for sample formats and planar buffers, sample-rate negotiation, realtime-safe FIR WAV loading outside callback, latency accounting and hardware tests. This straightforward FIR convolution is O(taps) per sample; a long linear-phase correction FIR will require partitioned FFT convolution and explicit latency measurement before realtime use. Do not put this prototype in the audio path yet.

Keep Equalizer APO mode and existing photographic Qt GUI untouched. The input guitar must be forwarded unprocessed; apply FIR to playback only.
