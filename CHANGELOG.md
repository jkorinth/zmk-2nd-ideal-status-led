# Changelog

## 1.0.0 (2026-11-02)


### Features

* add gaming layer color ([c70007d](https://github.com/jkorinth/zmk-2nd-ideal-status-led/commit/c70007dd5fa05d34755aa3a2e7b396c84ff41e70))
* add led strip test feature ([1338398](https://github.com/jkorinth/zmk-2nd-ideal-status-led/commit/13383984d09278c709f2f4a05db1b8e31b47b338))
* **backlight:** add per-LED underglow color tuning behaviors ([bc64587](https://github.com/jkorinth/zmk-2nd-ideal-status-led/commit/bc64587ffae436c4ec8dd71673475f84a8de8927))
* **backlight:** persist and restore on/off state across reboot ([aeb9668](https://github.com/jkorinth/zmk-2nd-ideal-status-led/commit/aeb9668860853a38dfaea1d76231cadc65290fb1))
* **behvavior:** add a behavior that supports cmds (on/off for now) ([3d52265](https://github.com/jkorinth/zmk-2nd-ideal-status-led/commit/3d5226560068c22bf58a7509b7778e51e9dfe351))
* initial commit ([f313cf5](https://github.com/jkorinth/zmk-2nd-ideal-status-led/commit/f313cf50efe35a7c65702ea82241549f9d75b556))
* make status_led work on both halves ([8fddcac](https://github.com/jkorinth/zmk-2nd-ideal-status-led/commit/8fddcac30d7168a6f5e9b843b21453cc09cbcd0a))
* **proto:** add compact SetColorsBulk message, one write for all 6 LEDs ([17da157](https://github.com/jkorinth/zmk-2nd-ideal-status-led/commit/17da157770ba883598e4fb1d863b91f183ae7fc2))
* **proto:** improve and refine the protobuf definitions ([de3d6d6](https://github.com/jkorinth/zmk-2nd-ideal-status-led/commit/de3d6d63b749bbb24eee6a52e3b4500f2ffc5a2b))
* **status-led:** add opt-in bt_disable() diagnostic for BLE-vs-flicker A/B test ([9af8ddc](https://github.com/jkorinth/zmk-2nd-ideal-status-led/commit/9af8ddc013b0362277e5bf06f02e706d1a612ae0))
* **status-led:** add per-layer indicator colors for bl_select/bl_adjust ([0e778b4](https://github.com/jkorinth/zmk-2nd-ideal-status-led/commit/0e778b4fdcfe3e6ee8ccec23040bf44c6348b5a1))
* **status-led:** add rainbow hue-shift test mode ([091a683](https://github.com/jkorinth/zmk-2nd-ideal-status-led/commit/091a68390191262606df268eb5c32a34f6931882))
* **status-led:** sync peripheral LED with central's layer color ([6b3aab6](https://github.com/jkorinth/zmk-2nd-ideal-status-led/commit/6b3aab616b633937e29b7165dfd3c12ad90f5a1d))


### Bug Fixes

* **backlight:** fix stuck layer on commit/discard, add one-shot color reset ([6480d64](https://github.com/jkorinth/zmk-2nd-ideal-status-led/commit/6480d647674b8886c6ce53d5aaf7c1b8812f9683))
* **backlight:** make SL_TURN_ON/OFF toggle the static backlight on both halves ([fad865e](https://github.com/jkorinth/zmk-2nd-ideal-status-led/commit/fad865e680e483d9ae1ed3d99562604c4a939cb1))
* **backlight:** restore peripheral color push, root cause was missing data ([9528606](https://github.com/jkorinth/zmk-2nd-ideal-status-led/commit/95286060c15ac231cc395ceae8205c49f9f7fc8f))
* **backlight:** send peripheral colors as 6 small writes, not 1 big one ([fd34ab5](https://github.com/jkorinth/zmk-2nd-ideal-status-led/commit/fd34ab59e64483e4bd7373dccd24392bd49cc5a0))
* **backlight:** stop double-sending on SL_TURN_ON, restore known-good path ([1a75aaf](https://github.com/jkorinth/zmk-2nd-ideal-status-led/commit/1a75aafa1f85d5aac708ec2638b81cd3663e1dbe))
* **peripheral:** check for errors in connected callback ([8fbe22c](https://github.com/jkorinth/zmk-2nd-ideal-status-led/commit/8fbe22c72602b51afb2a90e02fb4c473e6aec825))
* **status-led:** compile-time layer/color-array check, exact default brightness ([4a549e6](https://github.com/jkorinth/zmk-2nd-ideal-status-led/commit/4a549e68c61ccdf7069490c180fddfdfd53f3434))
* **status-led:** run TEST_NO_BLE's bt_disable() after BT is actually enabled ([6dbdfcc](https://github.com/jkorinth/zmk-2nd-ideal-status-led/commit/6dbdfcce17cb8720bf120fda6c90ac78c20f0ac6))
* **status-led:** walk rainbow in physical zigzag order, not chain index ([d1968a7](https://github.com/jkorinth/zmk-2nd-ideal-status-led/commit/d1968a7d66fc2637201daf3582ce9cfb122bea76))
