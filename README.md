# ZMK Input Processor: Deadzone

A [ZMK](https://zmk.dev) input processor module that filters out trackball micro-vibrations.

Trackball sensors can report small movements from vibrations even when not being touched. This module suppresses those events by requiring a minimum cumulative movement before passing events through.

## How It Works

1. Accumulates `abs(dx) + abs(dy)` from each `REL_X` / `REL_Y` event
2. Suppresses events while the accumulator is below `threshold`
3. Passes events through once the accumulator reaches `threshold`
4. Resets the accumulator after `timeout-ms` of no movement

Other event types (buttons, sync, etc.) always pass through.

## Installation

Add to your `west.yml`:

```yaml
manifest:
  remotes:
    - name: t-ogura
      url-base: https://github.com/t-ogura
  projects:
    - name: zmk-input-processor-deadzone
      remote: t-ogura
```

Then run `west update`.

## Usage

Add to your overlay file. Place it **before** other input processors in the chain.

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
        input-processors = <&deadzone_processor
                            &your_other_processor>;
    };
};
```

## Properties

| Property | Type | Required | Description |
|----------|------|----------|-------------|
| `threshold` | int | yes | Cumulative movement magnitude before events pass through |
| `timeout-ms` | int | yes | Idle time (ms) after which the accumulator resets |

## License

MIT
