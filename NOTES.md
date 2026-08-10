# Session notes (2026-08-09/10)

Context dump for picking this back up.

## Where things stand

`master` builds green ([run 31381150680](https://github.com/d53dave/zmk-config-sofle/actions/runs/31381150680),
commit `8a02cc4`). Both halves reflashed and confirmed working, twice
now - both needed a `settings_reset` cycle both times, **including the
side that never had its firmware changed**. Confirmed working on
hardware:

- RGB underglow: on, toggle, brightness, color all work. Devicetree
  comes from mainline ZMK's own `sofle` shield overlay (not hand-
  rolled). `CONFIG_ZMK_RGB_UNDERGLOW_EXT_POWER=n` keeps `RGB_TOG` from
  also cutting `ext_power` (which feeds the displays).
- RGB idle timeout raised to 25 min (`CONFIG_ZMK_IDLE_TIMEOUT=1500000`,
  5x the previous 5 min default).
- Display: back to ZMK's **built-in** status screen (not custom). Has
  a known cosmetic bug - see TODO below - but boots and works.
- Underglow color changes based on the active layer
  (`config/src/rgb_layer_color.c`) - purple (default) / amber (lower) /
  teal (raise), matching the purple already used elsewhere in the keymap.
  Plain event listener calling the existing `zmk_rgb_underglow_set_hsb()`
  API, no display/LVGL involved.

## Operational note: reflashing one half can require resetting both

Split communication between the halves is still BLE-based even though
power is wired via TRRS (see top of this file / repo context - no
batteries fitted, but BLE is still used for split data transport). The
two halves maintain a BLE bond with each other. Confirmed twice now:
after flashing broken/crashed firmware to one half (left) and then
reflashing known-good firmware, **both halves needed a `settings_reset`
cycle to recover, including the right half which never had its firmware
touched**. Working theory: the left booting into crashed/different
firmware and failing to (re)connect to the right over BLE can leave bad
bond/connection state on the right side too, independent of what
firmware is actually on it.

**Practical takeaway**: after any experiment that changes one half's
firmware to something that might crash or change split behavior, expect
to `settings_reset` *both* halves to fully recover, not just the one
that changed - don't waste time assuming a same-firmware half is
unaffected.

## In progress: custom OLED status screen, take 3

Target design (no battery widget - this is a cable-only build, no
battery fitted):
- **Left (central)**: WPM indicator, layer indicator, static bitmap (TBD).
- **Right (peripheral)**: connection indicator, static bitmap (TBD).

Plain text/digits only for the indicators - deliberately avoiding
`LV_SYMBOL_*` icon glyphs this time, since those are the ones confirmed
broken in the built-in screen (see TODO below). Bitmaps come last, after
the text-based widgets are confirmed stable - most new risk (image
asset + decoder), most sequenced.

Going incrementally given the history below - two earlier attempts both
crashed hardware (dead keyboard input, static-noise display) and the
cause was never isolated, so this restart starts from a mechanism-only
test rather than jumping to the full design:

- **Step 0 - PASSED**: `lv_obj_create(NULL)` + solid white bg fill, zero
  widgets/labels/events. Flashed on hardware: **keyboard stayed fully
  functional** - the override mechanism itself
  (`zephyr/module.yml` + `CMakeLists.txt` + `Kconfig`'s
  `choice ZMK_DISPLAY_STATUS_SCREEN default ZMK_DISPLAY_STATUS_SCREEN_CUSTOM`
  + the weak-symbol `zmk_display_status_screen()`) is safe on this
  hardware. This was the real unknown from the two earlier crashes -
  resolved. Screen came out blank/black instead of the intended lit
  white though - likely color-polarity (devicetree has `inversion-on`,
  and the known-good built-in-screen look is dark bg + light text, the
  opposite of a white *background fill*) rather than anything wrong with
  rendering itself.
- **Step 0b - FAILED, reverted**: swapped the bg-fill approach for a
  single static text label (`lv_label_create`, default theme styling, no
  bg color override, no events/timers). Also needed
  `CONFIG_LV_USE_LABEL=y` added explicitly to `sofle.conf` first - the
  peripheral build failed to link (`undefined reference to
  lv_label_create`) without it, since that symbol was only being pulled
  in via `CONFIG_ZMK_WIDGET_WPM_STATUS`'s `select`, which is
  central-only (fixed, harmless, stays either way). With that fixed and
  flashed: **crashed hardware** - keyboard non-functional on both
  halves. Left showed "mostly white with black static" (not the
  legible label text). Right showed white with *faint ghosted
  battery/wifi icons* - stale content from the old built-in screen
  bleeding through, meaning the new screen wasn't cleanly flushing/
  redrawing there either. Reverted `config/Kconfig`'s choice override
  (commented out) back to the built-in screen to restore a working
  keyboard; `config/src/status_screen.c` left in place (unused while
  the choice is off) for reference.
- **This is the tightest isolation of the mystery crash so far**: the
  *only* difference between working step 0 and crashing step 0b is one
  static `lv_label_create` call. Not shapes, not timers, not events -
  just a label. Given the built-in screen's own labels/text otherwise
  render fine (aside from the separate icon-glyph-noise bug below),
  something about *our* label specifically - or the interaction between
  `CONFIG_LV_USE_LABEL` being newly enabled and something else in this
  build - is implicated. Not guessing further without real signal.
- **Next: get actual diagnostic signal.** Added a diagnostic-only
  `build.yaml` target (`sofle_left` + `zmk-usb-logging` snippet +
  `-DCONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM=y`) that forces the
  crashing custom screen on and routes the console over USB serial, so
  the actual fault/crash output can be captured (register dump, stack
  trace, assert message) instead of inferring from symptoms. Flash to
  the **left half only**, with a serial terminal already connected and
  capturing *before* it boots (crash likely happens at/near boot).
  Reflash normal `sofle_left` firmware afterwards - this build is
  diagnostic-only, not meant to be run normally.
- **Attempted to capture the crash log, hit a bigger finding instead**:
  flashed `sofle_left_debug_usb_logging.uf2`, set up a watcher polling
  for `/dev/cu.usbmodem*` (macOS) across multiple unplug/replug cycles -
  **no USB device ever appeared at all**. No connect sound, nothing in
  the USB device list either. The normal `sofle_left` build (Studio
  snippet) enumerates over USB every time, without exception, all
  session - so this isn't a cable/port issue, it's specific to the
  diagnostic build. This means the crash likely happens before USB even
  initializes (much earlier than assumed), *or* the `zmk-usb-logging`
  snippet itself doesn't work on this board independent of the custom
  screen - can't tell which yet.
- **Added a second diagnostic target to isolate that**:
  `sofle_left_debug_usb_logging_builtin_screen` - same `zmk-usb-logging`
  snippet, but the *built-in* status screen (no
  `-DCONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM=y`). If USB still doesn't
  enumerate with this one, `zmk-usb-logging` itself is broken on this
  board/config, unrelated to the custom screen. If USB *does* come up,
  the custom-screen crash is severe enough to prevent USB init too - a
  bigger, earlier fault than previously assumed. **Not yet tested.**
  If USB logging turns out to be a dead end entirely, next options are a
  hardware debug probe (SWD/RTT) if one is available, or a UART-to-USB
  adapter wired directly to the nRF52840's hardware UART pins - both
  more hands-on than what's been tried so far.
- Step 1 (WPM + layer text widgets) and beyond are on hold until this
  crash is actually root-caused - no point building more on top of a
  foundation that crashes on one label.
- **CI artifact naming collision found (unrelated to the crash, but
  explains a confusing "left dead" report after reverting)**: the
  reusable workflow (`zmkfirmware/zmk/.github/workflows/build-user-config.yml`)
  names artifacts as `${shield}-${board}-zmk` - it does **not** include
  the snippet or cmake-args. The diagnostic target above and the normal
  `sofle_left` build both use shield `sofle_left` + board `nice_nano//zmk`,
  so they collided under the identical artifact name
  `artifact-sofle_left-nice_nano__zmk-zmk`. Confirmed via the GitHub API
  that a run with both targets only produced *one* artifact under that
  name - meaning a "normal sofle_left" download could silently have
  actually been the diagnostic build with the crashing screen forced on.
  **Fixed**: added an explicit `artifact-name: sofle_left_debug_usb_logging`
  to the diagnostic `build.yaml` entry so the two can never collide again.
  If "wrong/dead firmware after flashing a supposedly-normal build" comes
  up again for any shield combo, check for an artifact name collision
  first via `gh api repos/d53dave/zmk-config-sofle/actions/runs/<id>/artifacts`
  before assuming settings/hardware.

### History: two earlier attempts, both crashed (paused as of this restart)

Original goal was custom text + a small animated "cat" widget. Both
crashed on real hardware the same way:

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

## Tried and abandoned: zmk-nice-oled module

Tried [mctechnology17/zmk-nice-oled](https://github.com/mctechnology17/zmk-nice-oled)
(`nice_oled` shield) instead of hand-rolling the custom status screen
again. Targets the exact display this board has (SSD1306 128x32 I2C), no
devicetree conflict with the base `sofle` shield. Didn't pan out - dead
end, but worth recording why so it isn't retried blindly:

- Against ZMK `main`: fails to build. The module's generated image assets
  use the older pre-LVGL-v9 API (`LV_IMG_CF_INDEXED_1BIT`,
  `lv_draw_img_dsc_t`, `LV_IMG_CF_TRUE_COLOR`) which `main`'s current LVGL
  has renamed/removed. It was built against ZMK `v0.3.0`'s older LVGL.
- Tried pinning `west.yml` to `v0.3.0` to match (confirmed `v0.3.0`,
  tagged 2025-08-01, is actually ZMK's *newest tagged release* - `main` is
  184 unreleased commits ahead - and Studio support already exists at
  that tag, so this wasn't a feature downgrade). **This broke every other
  build too**: `nice_nano//zmk` board-variant syntax needs Zephyr's board
  metadata `qualifiers` field, which didn't exist yet in the Zephyr
  version `v0.3.0` pins (`//zmk` board variants were only added to ZMK in
  Dec 2025 - see https://zmk.dev/blog/2025/12/09/zephyr-4-1#zmk-board-variant,
  four months after `v0.3.0`). Build failed with `KeyError: 'qualifiers'`
  before compiling anything, on *all* targets including the previously
  green sofle_left/sofle_right/settings_reset.
- **Reverted everything**: `west.yml` back to `revision: main`,
  `build.yaml`'s nice_oled targets and `config/nice_oled.conf` removed.
  Confirmed CI green again on `main` before moving on (see run history).
- Bottom line: `nice_oled` needs an LVGL version incompatible with the
  board-variant setup this repo currently needs to build at all. Not
  worth pursuing further unless the module gets updated for LVGL v9, or
  a much older/different board-definition approach is adopted (not
  recommended - would give up the `//zmk` variant this repo's Studio
  support depends on).

## TODO: built-in status screen icon glyphs render as noise (left half)

**Not a hardware issue** - confirmed working on the legacy branch on this
same board (see "Tried and abandoned" above: legacy pins a completely
different ZMK fork, `infused-kim/zmk@sofle`, pre-LVGL-v9 API). This is a
software/version regression to keep chasing via source/config, not
wiring/physical causes.

Symptom: only the *icon* symbol glyphs render as noise (confirmed: layer
number and WPM digits next to them are legible), and only on the left
half (confirmed: right half is fine). RGB on/off doesn't affect it
(tested, ruled out).

Real lead, confirmed from ZMK's own source
(`app/src/display/widgets/Kconfig`):
- `ZMK_WIDGET_OUTPUT_STATUS` (the widget using `LV_SYMBOL_USB`,
  `LV_SYMBOL_WIFI`, `LV_SYMBOL_OK`, `LV_SYMBOL_CLOSE`,
  `LV_SYMBOL_SETTINGS`) only compiles in on the **central** role - that's
  the left half in this build (USB-connected via the hub).
- `ZMK_WIDGET_PERIPHERAL_STATUS` (right half only) uses a *different*,
  smaller glyph set - just `LV_SYMBOL_WIFI` + `LV_SYMBOL_OK`/`LV_SYMBOL_CLOSE`
  (confirmed by reading both widgets' source). No `LV_SYMBOL_USB` or
  `LV_SYMBOL_SETTINGS` on the peripheral side, ever.
- So "only left is broken" doesn't require a per-half hardware
  difference at all - the two halves render genuinely different glyphs
  from the same font, because they run different widgets. The likely
  culprit is one specific glyph unique to `output_status.c`, most likely
  `LV_SYMBOL_USB` (if this keyboard connects to the host over USB rather
  than BLE) or `LV_SYMBOL_SETTINGS`.
- Searched ZMK's and LVGL's GitHub issues for a known report of this -
  no direct hit yet, so likely specific to this project's exact
  font/Kconfig combo rather than a widely-hit upstream bug.

**Next step**: confirm which host transport this keyboard actually uses
(USB vs BLE) to narrow down which specific glyph in `output_status.c` is
rendering, then test that glyph in isolation (e.g. a minimal LVGL label
showing just that one symbol) to confirm/deny it's the corrupted one.

## Still open / next steps

- OLED custom status screen - in progress, see "In progress" section
  above (currently at step 0, not yet hardware-tested). `zmk-nice-oled`
  was tried and doesn't work here - see "Tried and abandoned" above.
- Left-half-only garbled icon glyphs on the built-in status screen - not
  root-caused, see note above (physical/left-half-specific, not RGB).
- Tune the per-layer RGB colors in `config/src/rgb_layer_color.c` further
  to taste if purple/amber/teal isn't quite right.
- Kailh clicky switch swap (the original reason for revisiting this
  repo) hasn't been touched — hardware-only, no firmware config needed.
- Pinning `west.yml` off of `main` isn't currently viable: tried, and
  even the newest tagged ZMK release (`v0.3.0`) predates the `//zmk`
  board-variant support this repo's build depends on. Tracking `main`
  remains the only option unless the board-variant approach changes.

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
