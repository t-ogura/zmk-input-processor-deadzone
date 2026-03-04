# ZMK Input Processor: Deadzone

A [ZMK](https://zmk.dev) input processor module that filters out trackball micro-vibrations.

Trackball sensors (e.g. PAW3222) can report tiny movements from table bumps, temperature changes, or sensor noise — even when nobody is touching the trackball. If you use ZMK's `temp-layer` (automouse) feature, these phantom movements will keep activating the mouse layer unexpectedly.

This module solves that by requiring a minimum amount of cumulative movement before any events are passed through to the rest of the input processing chain.

## How It Works

```
Trackball sensor
     |
     v
 [ Deadzone Processor ]  <-- This module
     |
     |  Suppresses events until cumulative movement >= threshold
     |  Resets after timeout-ms of no movement
     v
 [ Other processors (temp-layer, scaler, etc.) ]
     |
     v
  ZMK input subsystem
```

1. Each incoming `REL_X` / `REL_Y` event adds `abs(value)` to an internal accumulator
2. While the accumulator is **below** the threshold, events are **suppressed** (not forwarded)
3. Once the accumulator **reaches** the threshold, all subsequent events **pass through** normally
4. After no trackball movement for `timeout-ms`, the accumulator resets to zero — ready to filter the next burst of noise

Non-movement events (buttons, sync, etc.) always pass through unaffected.

## Installation

### Step 1: Add to your `west.yml`

```yaml
manifest:
  remotes:
    - name: aerogu34
      url-base: https://github.com/aerogu34
  projects:
    - name: zmk-input-processor-deadzone
      remote: aerogu34
```

Then run:

```bash
west update
```

### Step 2: Add to your overlay / devicetree

Define the processor node and add it to your input-listener's `input-processors` list. Place it **before** any processors that react to movement (like `temp-layer` or `runtime-input-processor`).

```dts
/ {
    deadzone_processor: deadzone_processor {
        compatible = "zmk,input-processor-deadzone";
        #input-processor-cells = <0>;
        threshold = <10>;
        timeout-ms = <300>;
    };

    trackball_listener: trackball_listener {
        compatible = "zmk,input-listener";
        device = <&trackball>;
        input-processors = <&deadzone_processor    /* <-- first in chain */
                            &your_other_processor>;
    };
};
```

That's it. Rebuild your firmware and flash.

## Configuration

### `threshold`

Minimum cumulative movement magnitude (`abs(dx) + abs(dy)`) before events pass through.

| Value | Behavior |
|-------|----------|
| 5     | Very sensitive — filters only the smallest vibrations |
| **10**| **Recommended starting point.** Blocks typical sensor noise (deltas of 1-3) while keeping response snappy |
| 20    | Requires more deliberate movement to activate |

**If vibrations still trigger your mouse layer** → increase this value.
**If the trackball feels sluggish to start** → decrease this value.

### `timeout-ms`

How long (in milliseconds) after the last trackball event before the accumulator resets to zero.

| Value | Behavior |
|-------|----------|
| 100   | Resets quickly. Brief pauses during movement will require re-exceeding the threshold |
| **300** | **Recommended starting point.** Bridges natural pauses during continuous trackball use |
| 500   | Very forgiving. Good for slow, deliberate trackball movement styles |

**If movement feels choppy when you pause briefly** → increase this value.
**If you want the deadzone to re-arm faster after stopping** → decrease this value.

### Tuning guide

Start with `threshold = <10>` and `timeout-ms = <300>`, then adjust:

1. **First, tune `threshold`** — find the smallest value where vibrations no longer trigger your mouse layer
2. **Then, tune `timeout-ms`** — find the shortest value where continuous trackball use still feels smooth

## Compatibility

- ZMK v0.3.0+
- Any trackball or pointing device that emits `REL_X` / `REL_Y` events
- Works with any number of other input processors in the chain

## License

MIT — see [ZMK's license](https://github.com/zmkfirmware/zmk/blob/main/LICENSE).
