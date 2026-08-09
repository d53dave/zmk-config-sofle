# Session notes (2026-08-09)

Context dump for picking this back up. Rewritten once already because
earlier entries in this file contained wrong theories that got
disproven — see "Dead ends" at the bottom instead of trusting old
commit messages that reference them.

## Where things stand

`master` builds green ([run 31337327533](https://github.com/d53dave/zmk-config-sofle/actions/runs/31337327533),
commit `559b5ac`).

**RGB is confirmed working on hardware**, including toggle - the
`EXT_POWER=n` fix (see below) resolved the toggle-triggered crash. The
earlier "boot-time brownout on the right half" turned out to be a
stale persisted-on state from before the fix, cleared via the
`settings_reset` build target (see `build.yaml`) - not a real hardware
power issue. RGB devicetree now comes from mainline ZMK's own `sofle`
shield overlay, not a hand-rolled one.

**Custom OLED status screen added** (commit `559b5ac`, in
`config/src`, `config/include`, `config/Kconfig`, `config/CMakeLists.txt`,
`config/zephyr/module.yml`): the default status screen's output/battery
icons were rendering as garbled noise on both halves (photographed and
confirmed - see conversation), and there's no battery on this wired
build anyway. Replaced entirely with a from-scratch custom status
screen built as a proper Zephyr module living in `config/`:

- A small animated "cat" widget (plain LVGL shapes - circle + two
  rectangles - not bitmap images, so it can't suffer the same
  corruption). Central half: paws bounce faster with typing speed
  (WPM). Peripheral half: gentle idle bounce while linked to central,
  frozen when not (no WPM/layer data exists on the peripheral side).
- Central half also shows: active layer name, WPM number, USB/BLE
  output status (font-symbol glyphs, which were never corrupted - only
  the bitmap icons were).
- Peripheral half also shows: "linked"/"no link" text.

**Not yet confirmed on hardware** - flash `master` and check both
screens render correctly and the cat animates.

This is a **wired split, no batteries**. Left (central) half is
USB-powered (through a powered hub); right (peripheral) half draws
power over the TRRS cable from the left half. Split data transport is
still BLE (TRRS only carries power) - confirmed via
`ZMK_WIDGET_PERIPHERAL_STATUS depends on ZMK_SPLIT_BLE` in ZMK's own
Kconfig.

## What's actually enabled right now

- `config/sofle.conf`: `CONFIG_ZMK_RGB_UNDERGLOW=y`,
  `CONFIG_ZMK_RGB_UNDERGLOW_EXT_POWER=n` (keeps `RGB_TOG` from also
  cutting `ext_power`, which feeds the displays — confirmed via both
  the old `infused-kim` fork's config comment and mainline ZMK's own
  `sofle.conf` template, which carries the identical comment/default).
- `config/sofle.keymap`: **no hand-rolled RGB devicetree anymore.**
  Removed it entirely (commit `d66df2a`). Reason: mainline ZMK's
  `sofle` shield ships its own board-specific overlay at
  `app/boards/shields/sofle/boards/nice_nano_nrf52840_zmk.overlay`
  (fetched and inspected via `gh api` from `zmkfirmware/zmk@main`),
  which already defines `&spi3`, the `led_strip: ws2812@0` node
  (MOSI on P0.06, GRB color-mapping, chain-length 36 arbitrary/
  overprovisioned - extra phantom LEDs are harmless per both this file
  and the old fork's identical comment), and the
  `chosen { zmk,underglow = &led_strip; }` — for exactly the
  `nice_nano//zmk` board variant this repo builds. Our hand-written
  copy was redundant and had drifted (different chain-length). Verified
  in the actual CI-compiled devicetree log (`gh run view --job <id>
  --log`) that there's exactly one `led_strip` node, no duplication.

## How we got here (chronological, including dead ends)

1. Migrated `config/west.yml` off the stale `infused-kim/zmk@sofle`
   fork (2023-era, dead) onto mainline `zmkfirmware/zmk@main`, for ZMK
   Studio support. Fixed a stack of resulting build breakage: invalid
   `frame-format`, renamed `CONFIG_WS2812_STRIP` Kconfig symbol, missing
   `//zmk` board-variant suffix, wrong `color-mapping` values. At the
   time it looked like mainline's `sofle` shield had no RGB wiring at
   all, so RGB devicetree was hand-rolled from scratch into
   `sofle.keymap`. **This assumption was wrong** — see below.
2. Toggling RGB on caused both OLEDs to blank and the matrix to
   drop/double-fire keys, recoverable only by a full power cycle.
   Underglow disabled entirely as a stopgap.
3. Found and fixed two real hardware defects unrelated to firmware: a
   lifted LED solder pad (bridged), and the front-indicator bypass
   jumper (left un-bypassed, indicator physically dimmed with a Posca
   marker dot instead). RGB re-enabled; brightness/color confirmed
   working; **toggle still crashed the same way.**
4. Found the real cause of the toggle crash:
   `CONFIG_ZMK_RGB_UNDERGLOW_EXT_POWER=y` makes `RGB_TOG` also toggle
   `ext_power`, which feeds the displays. Set to `n`. This fix is
   correct and confirmed by two independent sources (old fork's config
   comment, mainline shield's own template) — **keep it.**
5. After flashing that fix, the right half went completely dead at
   boot (underglow was persisted "on" from earlier testing, so the
   strip tried to init immediately). **Wrong theory floated here:**
   blamed a TRRS-power-headroom brownout. User correctly called this
   out as unfounded speculation — the legacy fork's firmware runs the
   same physical hardware/wiring with RGB on with no such issue, which
   directly contradicts a hardware/power explanation.
6. Went and read the actual devicetree source instead of guessing:
   fetched the working `infused-kim` fork's shield files via
   `gh api repos/infused-kim/zmk/contents/...` and mainline ZMK's
   current `sofle` shield files the same way. Mainline `sofle` **does**
   ship a working RGB overlay (added at some point after the original
   `west.yml` migration research) - the "no built-in RGB wiring"
   assumption from step 1 was stale/wrong. Our hand-rolled overlay in
   `sofle.keymap` duplicated it with different chain-length values.
   Deleted ours, rely on the shield's. This is a real, source-verified
   fix, not a guess - but still needs hardware confirmation.

## Dead ends (for context, don't repeat these)

- "LED current draw browns out the nice!nano regulator" — never
  verified, no meter reading ever taken.
- "TRRS-fed right half lacks current headroom vs USB-fed left half" —
  contradicted by the legacy fork working fine on identical hardware.
- Both of the above were plausible-sounding electrical theories
  invented without checking the one thing that actually mattered: what
  devicetree the known-working legacy firmware used, and what mainline
  ZMK's shield currently provides. If RGB still misbehaves after this
  fix, go back to primary sources (actual devicetree/Kconfig, CI's
  compiled output, or a meter) before theorizing again.

## Still open / next steps

- **Confirm on hardware**: flash `master` (commit `d66df2a`), check
  boot is clean and `RGB_TOG` doesn't blank displays / drop keys.
- Kailh clicky switch swap (the original reason for revisiting this
  repo) hasn't been touched — hardware-only, no firmware config needed.
- Consider pinning `west.yml`'s `revision: main` to a tagged ZMK
  release instead of tracking `main`, so future CI runs don't silently
  break on upstream changes the way the `//zmk` board-variant migration
  did.

## Fallback branch: not usable (as CI)

`legacy-pre-studio` branch (points at the last pre-migration commit,
old `infused-kim` fork, `nice_nano_v2`, no Studio) **does not build in
GitHub Actions**: `ModuleNotFoundError: No module named 'distutils'` -
that old toolchain predates Python removing `distutils`; the current
runner image doesn't have it. Pure bit rot, unrelated to anything
changed this session. Its *source* is still useful as a reference for
diffing against the old working config (as done above) even though CI
can't build it. The user has since found and manually flashed a
pre-built legacy firmware image directly (outside of CI).

Also: GitHub had already purged all workflow run history/artifacts
from before this session (only a handful of runs exist, all from
2026-08-09) — no old pre-built firmware to recover from past CI runs.

## Key combos (current keymap, raise_layer)

- **RGB underglow toggle**: hold RAISE (right thumb key next to
  spacebar) + tap the key in the same column as `6`/`Y`/`H`/`N` (top
  row, right half, first column).
- **ZMK Studio unlock**: hold RAISE + tap the key in the same column as
  `-`/`BKSPC` (top row, right half, last column).
