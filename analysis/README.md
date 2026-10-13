# Gaming Edition Investigation

Status: **partial implementation, not complete support for every edition**.
The legacy E502 now identifies its receiver and detects a 1080p60 HDMI source.
It produces H.264 and stereo AAC packets, but captured pictures remain visibly
corrupted. Do not treat the presence of stream metadata as a successful capture.

## Artifact Identity

| Supplied artifact | SHA-256 |
| --- | --- |
| mac/hdpvrcapture_v3.11.1_625.zip | eef841f65aec460df3a126962e00394e2121e153f4aefb57a3cd72c0a988a30a |
| Mac Contents/MacOS/HDPVRCapture | 2d046af7a5f68b2735f228f97a32b5221268f4e43d47c79019d360d1cef99ebb |
| win/hauppaugecapture_1_1_37227.exe | 1d705bd6ba074e774d9dcbe577e2b04b436a80995063b7cab7b1e644776f17ea |
| Extracted Win10 DriverE5/x64/HcwE5bda_64.inf | 0c6cf774a9268a7f128ad64868e34caa93ed18c0fea482c200116da0ece43389 |
| Extracted Win10 DriverE5/x64/DriverE5/64bit/HcwE5bda.sys | efac64062130f76c4ebdefd2f391c77aaa872fdd6a131df55b1c65a49f9097ef |

REA 6.4.0/Ghidra 12.1.4 performed static native analysis. Neither vendor
application nor the Windows driver was executed. The Windows EXE wraps a Wise
installer: its `.rsrc_1` resource has 260 bytes of padding before the inner PE.
7-Zip extracted that resource and WiseUnpacker 3.0.0 materialized its contents.
Those extraction/INF observations are shell evidence, not REA runtime evidence.

## Device Evidence

The Mac lookup at `0x100024f58` matches vendor, product, and device revision
against nine 32-byte records starting at `0x1000cbad0`. All nine records specify
vendor `2040` and revision `0800`. The initializer at `0x100024fdf` interprets
record offsets `19`, `1b`, `1c`, `1d`, and `1e` as the presence of CX25840,
ADV7842, TDA9955, TDA19977, and TDA19988 respectively.

| Product ID | Gaming variant | Receiver | Optical audio |
| --- | --- | --- | --- |
| e502 | Gaming Edition model 1480 / Siena1-05b+ | TDA19977; separate TDA9955/CX25840 and TDA19988 | No |
| e524 | Gaming Edition, Siena2-02 | ADV7842 | No |
| e525 | Gaming Edition with microphone, Siena2-05a | ADV7842 | No |
| e504 | Gaming Edition Plus, green LEDs | ADV7842 | Yes |
| e554 | Gaming Edition Plus, blue LEDs | ADV7842 | Yes |
| e505 | Gaming Edition Plus with microphone | ADV7842 | Yes |

The Windows INF independently maps E502 to `Siena_05b.AddReg`, E524 to
Siena2-02, and E525/E505 to Siena2-05a. Its E502 clock settings are
`ClockPulseDelay=7` and `ClockPixClkOutToggle=128`. These registry values alone
do not establish which hardware registers should be changed.

The original Linux code always instantiated `receiver_ADV7842_t`. E502 was
accepted only through a broad unknown-product fallback. Its I2C failures and
default 720p stream therefore did not prove a valid capture from the source.

## Native Evidence Ledger

Full responses are retained in `rea/`. They contain subject digests, complete
function body ranges, pseudocode, assembly, and value-flow limitations.

| Claim | Evidence ID | Retained response |
| --- | --- | --- |
| Exact product/revision lookup | ev_ca11614c3270e41a7765a140b7875cc37ae2e934606882cfda02b70f1a3f1b16 | rea/mac-find-device.json |
| Device table bytes | ev_c6a4468ba9a5c7d8c71d37aa5754df9ee545b34b30b9399a4c132c0e165b878f | rea/mac-device-table.json |
| Receiver presence flags | ev_0e8749d3dcffe729bfc6ffd5aede06e172b5553039d937bee1e863a7d87fa967 | rea/mac-init.json |
| Conditional receiver initialization | ev_82dc82f2064d6111ca576ef329f9c60a119968053c697569d9bbf5e7e89a8480 | rea/mac-configure-hardware.json |
| TDA19977 address 98 | ev_bbd96225082b3b2d7235da596cdc37889d082b6b002a68a06be629a875c071e6 | rea/mac-tda19977-init.json |
| Page-select register ff | See retained subject-bound response | rea/mac-tda-i2c-read.json |
| Power/TMDS/EDID setup | ev_edfcf45a56cd9525dee22e08a527bf9cecb5b8f29c260f5b5eb98f4da3db823a | rea/mac-tda19977-initialize.json |
| Timing-table matching | ev_8ac543a3bcb2c76add76f8ad6288c69158f2496b4518c2f2aa1173165419e7cc | rea/mac-tda-find-format.json |
| Receiver lock and measured timing fields | ev_260501d2a8a630aa2177a37955c54d2b43120feec5455824b22d7c3ff5613fcb | rea/mac-tda-status.json |
| 1080p60 receiver register sequence | ev_be084f54c3a11a02a140e5ccc1cfe2c15269990f52c26de7a3dc1f0b33e5703e | rea/mac-tda-1080p60.json |
| HDMI hotplug: port C bit 10 | ev_a3e9661a68b4c90fe7a124aff75d4e88075feffecb12ff7d5b7500c353031773 | rea/mac-hotplug.json |
| Legacy transmitter control reset | See retained subject-bound response | rea/mac-transmitter-reset.json |
| Encoder/I2C ownership before capture | See retained subject-bound response | rea/mac-usb-capture.json |
| Encoder INIT and AVL messages | See retained subject-bound responses | rea/mac-set-init.json; rea/mac-set-avl.json |
| Windows receiver clock controls | ev_45a176f93c28df20b95d12f3aa031376e51ed7a8a4dbe5eff79bd7b624532a3a | rea/win-clock-strings.json |
| Windows format-dependent clock setup, 90570 | See retained subject-bound response | rea/win-clock-program.json |
| Static hardware dispatch configuration | ev_ce59e2a29dae55e51b10be4cd8457a3dbce6e27230b2be8bffb64316da14dab8 | rea/win-hardware-config.json |
| Clock delay registers 0017/0016, 9b1e0 | ev_0f5e3044ab3221a6c2ea48238a1bca043e28d58c4d15fc93c2c03ec1136a5553 | rea/win-clock-delay-setter.json |
| Clock toggle register 00e9, 9b3c0 | ev_65185bc47ae1b0726fae1fc9563ec11dbff1419114d11c9f504dd4423f3a8213 | rea/win-clock-toggle-setter.json |
| Output formatter registers 00e5/1341, 9dca0 | ev_dea40a1f055e22adbe7f48becb1e6834fc0c12cd82b8d6c47f83114737ec8589 | rea/win-output-format-setter.json |

Decompiler output is not original source and does not demonstrate execution.
Some calls initially omitted arguments; the timing-match arguments and paged
I2C transport were checked against assembly and their callees. Register data
export is limited to complete constant write sequences; it does not reproduce
arbitrary calls or claim that omitted transmitter configuration is implemented.

The Windows driver dispatch pointer is copied from `da140+48` to runtime
`f4668`. It selects a method table at `d9e70`, whose `+68`, `+70`, and `+f0`
entries resolve to `9b1e0`, `9b3c0`, and `9dca0`. The low-level write function
at `99020` confirms that these methods use physical paged register addresses,
not abstract register indices. The Mac sequence already sets clock delay 7
and clock-toggle bit 80. A direct Windows packed-output probe produced audio
but no video packets and was removed; it did not establish the correct bus
contract for this Linux encoder path.

## Firmware

The supplied Mac encoder blobs are identical to the upstream Linux blobs:

| Blob | SHA-256 |
| --- | --- |
| HcwE5ENC_t22_24.bin / llama_usb_vx_host_slave_t22_24.bin | e6baff2afa70b19fc81bf93c9e6c89d6a70160cb2c96aa68c6b0de2fb2a430da |
| hcwE5cpu_mips.bin / mips_vx_host_slave.bin | e7b655c5d05c3d19314f51b159529f5b589dfb4c85b74f129168cc69fa9f30fc |

The firmware identity comparison does not cover the FX2 image or prove hardware
compatibility. Merely replacing these encoder blobs would not change them.

## Runtime Findings

An E502 at revision 0800 responded with TDA19977 revision `41` after fixing the
USB open path. The application now avoids an unconditional reset, validates
interface claims, refreshes enumeration, and reopens the same serial after FX2
restart instead of continuing on a stale handle.

The source is a Nintendo Switch 2. Direct display output was confirmed at
1080p60 SDR/stereo with HDR, VRR, and ALLM disabled. The receiver then measured
1080p60 and the encoder produced 1920x1080 progressive H.264 around 29.97 fps
and 48000 Hz stereo AAC. Interior decoding succeeded, but the extracted picture
was visibly corrupted. The sample also had cutoff-tail decode errors and
firmware frame-counter warnings. The encoder-bus/I2C handoff did not fix the
corrupted picture. HDMI passthrough was reported blank.

Enabling the upstream firmware's TDA19977 AVL driver also failed to produce a
usable stream and that override was removed. Neither failed experiment is
installed in the final source. The retained legacy receiver still uses the
recovered Mac output sequences; its output-bus compatibility remains unverified.

`verification/e502-capture.json` and the PNG frames preserve this partial result.
They are evidence of the remaining failure, not proof of completed support.

## Implementation and Validation

The profile table distinguishes all Gaming variants present in these packages,
selects TDA19977 for E502, and retains ADV7842 for the existing Siena2 path.
Prototype/other legacy boards are named but explicitly unimplemented. Unknown
products are no longer silently treated as an HD PVR. Optical input is rejected
on models that lack it.

The experimental legacy path currently accepts HDMI video/audio with AAC (AUTO
falls back to AAC); it does not implement legacy component/composite, AC3
detection, or complete TDA19988 passthrough. Unknown receiver timings fail
instead of returning a fabricated video mode. Other hardware variants were not
physically available for regression testing.

After preparing the upstream source tree as documented in the main README:

```sh
make check
make
```

The focused test mocks FX2 I2C/ports and covers model profiles, wrong-chip and
transport rejection, EDID checksums/data, hotplug lifecycle, signal loss, and
1080p60/50 and 720p timing selection. Tests and compilation pass. They do not
establish picture quality, passthrough, or support for untested devices/modes.

`tools/export-tda-data.mjs` regenerates the constant register/timing/EDID facts
from retained REA responses and checks the exact source digest and completeness.
`tools/rea-session.mjs` retains complete JSON-RPC tool responses in one session;
it requires user-configured Ghidra/Java and never runs a vendor target.

Remaining work: identify/fix the Siena1 pixel sampling/bus contract, verify clean
live video/audio and passthrough, implement the other legacy inputs/audio paths,
and test the other Gaming boards. No claim of "all editions fully supported" is
made by this implementation.

The new build has **not** replaced the installed `/opt/Hauppauge` binary. A
known-working vendor capture/USB trace on Windows or macOS would provide the
missing runtime reference for the legacy sampling/pin configuration; static
register tables and valid MPEG-TS metadata alone are insufficient.