# Session notes (2026-08-09)

Context dump for picking this back up after a reboot.

## Where things stand

`master` builds green ([run 31334571549](https://github.com/d53dave/zmk-config-sofle/actions/runs/31334571549))
with **RGB underglow disabled again** (commit `cfd6ada`). This is a
recovery build to get a bootable keyboard back — flash it to both
halves.

This is a **wired split, no batteries** — left half is USB-powered
(through the powered hub), right half draws its power over the TRRS
cable from the left half's regulator, not from its own USB connection.
That TRRS-fed path has much less current headroom than direct USB.

Three separate things were going on, conflated as one "brownout" during
earlier diagnosis:

1. **Hardware defects** (42keebs v3 board): one LED's solder pad had
   lifted and lost power connection (bridged), and the board's
   indicator-bypass jumper was unclear in the docs — with the bypass
   active and the front indicator LED soldered, RGB wouldn't work at
   all. Not bypassing it fixed that, but the indicator LED is extremely
   bright — bright enough to leave a visual afterimage — and can't be
   dimmed in firmware on its own, so it's now covered with a thick dot
   of Posca paint pen to physically dim it. Works well. Because the
   indicator is no longer bypassed, it's part of the same WS2812 data
   chain as the underglow LEDs: `chain-length` went from `6` to `7`
   (6 underglow + 1 indicator) in `sofle.keymap`.
2. **The toggle-specific crash** (screens blank, keys drop/double-fire,
   only a power cycle recovers): `sofle.conf` had
   `CONFIG_ZMK_RGB_UNDERGLOW_EXT_POWER=y`, which makes `RGB_TOG` also
   toggle `ext_power` off — and `ext_power` apparently feeds the
   displays (and glitches the matrix on the transition). This explains
   why brightness/color changes were always fine and only the toggle
   broke things. The old `infused-kim` fork config already had this set
   to `n` with a comment describing this exact symptom — missed when
   reconstructing the RGB config from scratch for the mainline
   migration. Fixed in commit `57f24b2` — **this fix is real and should
   stay**, but wasn't sufficient on its own (see #3).
3. **Boot-time brownout on the right half** (fixed #1 and #2 applied,
   but underglow was persisted "on" from prior testing): the right half
   went completely dead at boot, before it could even receive a toggle
   command. Root cause: with RGB persisted on, the LED strip
   (chain-length 7) initializes immediately at boot — and the right
   half's TRRS-fed power path can't supply that current draw the way
   the left half's direct USB connection can. Mitigated for now by
   disabling RGB again (commit `cfd6ada`), which stops strip init at
   boot regardless of persisted state.

## What happened today

1. Migrated `config/west.yml` off the stale `infused-kim/zmk@sofle` fork
   (2023-era, dead) onto mainline `zmkfirmware/zmk@main`, to get ZMK Studio
   support. This is the right direction, but the mainline `sofle` shield has
   no built-in RGB wiring (the fork did), so underglow had to be
   reconstructed by hand as a devicetree overlay in `sofle.keymap`.
2. Fixed a stack of build breakage from that migration, in order encountered:
   - `frame-format` enum value invalid (fixed: `0`)
   - `CONFIG_WS2812_STRIP` — Kconfig symbol renamed/auto-selected on current
     Zephyr, removed from `sofle.conf`
   - `board: nice_nano` needs the Zephyr 4.1 `//zmk` variant suffix →
     `nice_nano//zmk` in `build.yaml`
   - `color-mapping = <2 1 0>` was wrong (raw indices vs. the actual
     `LED_COLOR_ID_*` scheme), silently mapping the blue channel to WHITE.
     Fixed to `<LED_COLOR_ID_GREEN LED_COLOR_ID_RED LED_COLOR_ID_BLUE>`
     (standard WS2812 GRB order) with `#include <dt-bindings/led/led.h>`.
   - `chain-length` set to `6`, matching confirmed hardware: underglow-only
     (42keebs v3 board), no per-key backlight LEDs populated.
3. **Problem surfaced after the above was green**: toggling underglow on
   caused both OLEDs to blank and the key matrix to drop/double-fire keys,
   recoverable only by a full power cycle (not just a reset). ZMK persists
   underglow on/off state across reboots, so once toggled on it started
   happening on every boot. At the time this looked like an LED
   current-draw brownout, so RGB was disabled in firmware
   (`CONFIG_ZMK_RGB_UNDERGLOW` commented out, `&spi3 status = "disabled"`)
   just to get back to a stable keyboard.
4. **Hardware root cause found and fixed**: a lifted LED solder pad
   (bridged) and an indicator-bypass jumper issue (see "Where things
   stand" above). RGB underglow re-enabled in `sofle.conf`/`sofle.keymap`
   (commit `57f6ed3`), `chain-length` bumped `6` → `7` for the now-inline
   indicator LED. Brightness/color confirmed working, but RGB_TOG itself
   still crashed the same way.
5. **Toggle-specific root cause found**: `CONFIG_ZMK_RGB_UNDERGLOW_EXT_POWER=y`
   coupling `RGB_TOG` to `ext_power` (see "Where things stand" above).
   Set to `n` in commit `57f24b2`.
6. **Right half went dead on boot** after flashing the above (underglow
   was persisted "on" from earlier testing). Traced to the TRRS-fed
   power path on the right half not having enough current headroom for
   boot-time strip init (see "Where things stand" #3). RGB disabled
   again in commit `cfd6ada` to recover a bootable keyboard.

## Still open / next steps

- **Re-enabling RGB again needs a strategy that survives boot-time
  init on the TRRS-fed right half**, not just avoiding the ext_power
  coupling. Options to consider: a much lower `CONFIG_ZMK_RGB_UNDERGLOW_BRT_STEP`
  / initial brightness so first-boot current draw is small even if
  state was persisted "on"; forcing underglow off by default on every
  boot instead of persisting state; or physically improving the
  right-half power delivery (thicker TRRS power conductor / added
  decoupling capacitor on that half specifically, right at the strip's
  VCC/GND). Should probably verify current draw with a meter before
  guessing again.
- Kailh clicky switch swap (the original reason for revisiting this repo)
  hasn't been touched — it's a hardware-only change, no firmware config
  needed for a switch swap.
- Consider pinning `west.yml`'s `revision: main` to a tagged ZMK release
  instead of tracking `main`, so future CI runs don't silently break on
  upstream changes the way the `//zmk` board-variant migration did today.

## Fallback branch: not usable

`legacy-pre-studio` branch (pushed today, points at the last
pre-migration commit, old `infused-kim` fork, `nice_nano_v2`, no Studio)
**does not build**: `ModuleNotFoundError: No module named 'distutils'`.
That old toolchain predates Python removing `distutils` (gone since
3.12); GitHub's current runner image doesn't have it. Unrelated to
anything changed today — pure bit rot from 2-3 years of dormancy. Not
worth fixing; `master` is the way forward.

Also: GitHub had already purged all workflow run history/artifacts from
before today (only 8 runs exist total, all from 2026-08-09) — there's no
old pre-built firmware to fall back to from past CI runs either.

## Key combos (current keymap, raise_layer)

- **RGB underglow toggle**: hold RAISE (right thumb key next to spacebar)
  + tap the key in the same column as `6`/`Y`/`H`/`N` (top row, right
  half, first column).
- **ZMK Studio unlock**: hold RAISE + tap the key in the same column as
  `-`/`BKSPC` (top row, right half, last column).
