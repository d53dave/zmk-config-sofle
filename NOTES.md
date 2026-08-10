# Session notes (2026-08-09/10)

Context dump for picking this back up.

## Where things stand

`master` builds green ([run 31338583910](https://github.com/d53dave/zmk-config-sofle/actions/runs/31338583910),
commit `1df41a0`). Confirmed working on hardware:

- RGB underglow: on, toggle, brightness, color all work. Devicetree
  comes from mainline ZMK's own `sofle` shield overlay (not hand-
  rolled). `CONFIG_ZMK_RGB_UNDERGLOW_EXT_POWER=n` keeps `RGB_TOG` from
  also cutting `ext_power` (which feeds the displays).
- RGB idle timeout raised to 25 min (`CONFIG_ZMK_IDLE_TIMEOUT=1500000`,
  5x the previous 5 min default).
- Display: back to ZMK's **built-in** status screen (not custom). Has
  a known cosmetic bug - see TODO below - but boots and works.
- **New**: underglow color now changes based on the active layer
  (`config/src/rgb_layer_color.c`) - teal (default) / amber (lower) /
  violet (raise). Plain event listener calling the existing
  `zmk_rgb_underglow_set_hsb()` API, no display/LVGL involved.

## TODO: custom OLED status screen (paused, twice crashed on hardware)

Wanted: replace the built-in status screen's icons (which render as
garbled static/noise on both halves - photographed and confirmed) with
custom text + a small animated "cat" widget. Attempted twice, both
crashed on real hardware in the same way:

- **Symptom**: firmware boots, but keyboard input is completely dead,
  and the display shows **static noise** (steady garbage, not
  flickering/blinking - correction from earlier description). This
  points more toward a stuck/corrupted framebuffer or a one-time init
  fault than a reboot loop, but that's not confirmed.
- **Attempt 1**: full version - text info widgets (layer/WPM/output)
  plus an animated "cat" built from plain LVGL shapes (`lv_obj_create`
  circle + 2 rects) driven by an `lv_timer`. Crashed.
- **Attempt 2**: stripped down to *just* the text labels
  (`lv_label_create` only, same pattern ZMK's own built-in widgets
  use) - no shapes, no `lv_timer`, nothing beyond what
  `zmk_display_status_screen()` + `ZMK_DISPLAY_WIDGET_LISTENER`-based
  labels do. **Also crashed, same symptom.** This rules out the cat/
  timer as the sole cause - something more fundamental is wrong, e.g.
  in the custom Zephyr module wiring itself (`config/zephyr/module.yml`
  + `config/CMakeLists.txt` + `config/Kconfig`,
  `choice ZMK_DISPLAY_STATUS_SCREEN default ZMK_DISPLAY_STATUS_SCREEN_CUSTOM`),
  or in how `zmk_display_status_screen()`'s weak-symbol override
  interacts with something on this specific build. CI compiling and
  linking cleanly does **not** catch this - it's a runtime fault.
- Both attempts were reverted; both are still in git history if useful
  as reference (search commit messages for "custom OLED status
  screen" / "custom status screen").
- **Before trying again**: get more diagnostic signal first rather
  than iterating blind on hardware the user can't type on -
  ideas: serial/RTT console log if accessible, an even more minimal
  test (e.g. just `lv_obj_create(NULL)` + a solid fill color, no
  labels/widgets/events at all, to check whether the crash is in
  `zmk_display_status_screen()` itself vs. the widget/listener layer),
  or checking whether `CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM` alone
  (built-in screen's C code, unmodified, just recompiled as "custom")
  reproduces it.

## Experiment: zmk-nice-oled (in progress)

Trying [mctechnology17/zmk-nice-oled](https://github.com/mctechnology17/zmk-nice-oled)
(`nice_oled` shield) instead of hand-rolling the custom status screen again.
It targets the exact display this board has (SSD1306 128x32 I2C) and its
`nice_oled.overlay` is empty - it relies on the base `sofle` shield's own
`oled` devicetree node, same as today, so no devicetree conflict.

Important caveat: its `zmk_display_status_screen()` override uses the same
`CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM` weak-symbol mechanism that
crashed twice before (see TODO below) - so this isn't guaranteed to avoid
that crash, but it doubles as a test of whether the crash was in the
hand-rolled widget code specifically or something more fundamental. Also
worth noting its "static image" mode still renders through a canvas +
`lv_img`/buffer-rotation pipeline (`screen_peripheral.c`), not the truly
minimal `lv_obj_create(NULL)`-only test originally suggested - it's a
different, heavier code path than anything tried on this board before.

Added as **new** build targets (`sofle_left nice_oled` / `sofle_right
nice_oled`) alongside the existing plain ones in `build.yaml`, so the
known-working firmware is still available to reflash if this crashes.
Config kept deliberately minimal for the first test
(`config/nice_oled.conf`): static "vim" image on the peripheral (no
animation), just the layer widget on central, no RAW HID.

**Update**: first CI run failed - confirmed real LVGL API drift, not a
config mistake (`LV_IMG_CF_INDEXED_1BIT` undeclared, `lv_draw_img_dsc_t`
renamed to `lv_draw_image_dsc_t`, etc. - this module's generated image
assets were built against the older LVGL bundled with ZMK `v0.3.0`, and
`main` has since moved on). Checked: `v0.3.0` (tagged 2025-08-01) is
actually the *newest* official ZMK release - `main` is 184 commits ahead
but none of those have ever been cut into a tagged release, and Studio
(`studio-rpc-usb-uart` snippet + `app/src/studio`) is already present at
`v0.3.0`, so pinning isn't a feature downgrade. Pinned `config/west.yml`'s
ZMK revision to `v0.3.0` repo-wide (also closes the pre-existing TODO
below about not tracking `main` forever). This affects every build
target, not just the nice_oled experiment, so **everything needs
re-flashing and re-verifying on hardware** after this, not just the
display.

Also open: separately from this module, the *built-in* status screen's
icons (battery/output symbol glyphs specifically, not the layer/WPM text)
render as noise on the left half only, both halves running identical
firmware/Kconfig/fonts (verified via CI build logs). RGB on/off doesn't
affect it (tested). Points at something physical to the left OLED module
or its wiring, not firmware - not yet root-caused.

## Still open / next steps

- OLED custom status screen - see TODO above.
- Tune the per-layer RGB colors in `config/src/rgb_layer_color.c` to
  taste (currently arbitrary: teal/amber/violet).
- Kailh clicky switch swap (the original reason for revisiting this
  repo) hasn't been touched — hardware-only, no firmware config needed.
- Consider pinning `west.yml`'s `revision: main` to a tagged ZMK
  release instead of tracking `main`, so future CI runs don't silently
  break on upstream changes the way the `//zmk` board-variant migration
  did earlier this session.

## Fallback branch: not usable (as CI)

`legacy-pre-studio` branch (points at the last pre-migration commit,
old `infused-kim` fork, `nice_nano_v2`, no Studio) **does not build in
GitHub Actions**: `ModuleNotFoundError: No module named 'distutils'` -
that old toolchain predates Python removing `distutils`; the current
runner image doesn't have it. Pure bit rot. Its *source* was still
useful as a reference for diffing against the old working RGB config
earlier this session, even though CI can't build it.

## Key combos (current keymap, raise_layer)

- **RGB underglow toggle**: hold RAISE (right thumb key next to
  spacebar) + tap the key in the same column as `6`/`Y`/`H`/`N` (top
  row, right half, first column).
- **ZMK Studio unlock**: hold RAISE + tap the key in the same column as
  `-`/`BKSPC` (top row, right half, last column).
