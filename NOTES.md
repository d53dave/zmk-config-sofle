# Session notes (2026-08-09)

Context dump for picking this back up after a reboot.

## Where things stand

`master` builds green ([run 31314355229](https://github.com/d53dave/zmk-config-sofle/actions/runs/31314355229))
but **RGB underglow is temporarily disabled**. Flash this firmware to get a
stable keyboard back.

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
3. **New problem surfaced after the above was green**: toggling underglow on
   caused both OLEDs to blank and the key matrix to drop/double-fire keys,
   recoverable only by a full power cycle (not just a reset). ZMK persists
   underglow on/off state across reboots, so once toggled on it started
   happening on every boot. Likely cause: LED strip current draw/inrush
   browning out the nice!nano's onboard 3.3V regulator (confirmed a
   high-quality powered hub is in use, so it's not a weak host supply).
   **Fix applied**: RGB underglow disabled again — `CONFIG_ZMK_RGB_UNDERGLOW`
   commented out in `sofle.conf`, `&spi3` set to `status = "disabled"` in
   `sofle.keymap` — to guarantee zero current draw regardless of persisted
   state, and get back to a working keyboard.

## Still open / next steps

- **Re-enable RGB underglow safely.** Likely needs a lower default
  brightness and/or a bulk decoupling capacitor across the LED strip's
  VCC/GND near the first LED (common real fix for this exact brownout
  symptom on nice!nano RGB builds). Don't just uncomment — verify current
  draw first.
- The devicetree/Kconfig for RGB is left in place (just disabled) in
  `config/sofle.conf` and `config/sofle.keymap` — search for "temporarily
  disabled" comments there to find both spots to flip back on.
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
  half, first column) — currently a no-op since RGB is disabled.
- **ZMK Studio unlock**: hold RAISE + tap the key in the same column as
  `-`/`BKSPC` (top row, right half, last column).
