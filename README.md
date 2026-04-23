# PWM Fade LED — nRF54L15

A minimal Zephyr application that demonstrates the Zephyr PWM API on the **nRF54L15 DK** by fading LED1 in and out using hardware PWM.

## What it does

1. **Fade in** — increases the PWM duty cycle from 0 % to 100 % in 50 steps, updating every 10 ms (500 ms total ramp).
2. **Pause** — holds the LED fully on for 1 second.
3. **Fade out** — decreases the duty cycle from 100 % to 0 % in 50 steps (500 ms total ramp).
4. **Pause** — holds the LED fully off for 1 second.
5. **Repeat** — forever.

Progress is logged to the serial console at every fade direction change.

## Platform

| Item | Value |
|---|---|
| Board | `nrf54l15dk/nrf54l15/cpuapp` |
| SDK | nRF Connect SDK v3.3.0-rc2 |
| Zephyr | v4.3.99-ncs |
| Toolchain | Zephyr SDK 0.17.0 (ARM GCC 12.2.0) |

## Hardware

**No external connections required.** LED1 is on-board.

| Signal | DK label | GPIO | PWM peripheral |
|---|---|---|---|
| PWM output | LED1 | P1.10 | PWM20 channel 0 |

### nRF54L15 PWM domain constraint

On nRF54L15, the PWM peripherals (PWM20/21/22) share a power domain exclusively with **GPIO Port P1**. Hardware PWM can only be routed to Port P1 pins. The other on-board LEDs (LED0, LED2) are on Port P2 and **cannot** use hardware PWM. Software PWM (`nordic,nrf-sw-pwm`) is **not** supported on the nRF54L-series.

## Building and flashing

```bash
# Set ZEPHYR_BASE if not already in your environment
export ZEPHYR_BASE=/path/to/ncs/v3.3.0-rc2/zephyr   # Linux/macOS
$env:ZEPHYR_BASE = "D:\work\ncs\v3.3.0-rc2\zephyr"   # Windows PowerShell

west build -b nrf54l15dk/nrf54l15/cpuapp --pristine
west flash
```

## Serial output

Connect to **VCOM1** (the second JLink CDC UART port) at **115200 baud**.

> **Note:** COM port numbers are assigned by Windows and vary from computer to computer. On the development machine used for this example the ports appeared as COM80 and COM81, but yours will likely be different. Open *Device Manager → Ports (COM & LPT)* and look for two entries named **JLink CDC UART Port** — VCOM1 is the one with the higher COM number.

Expected output after reset:

```
[00:00:00.000,000] <inf> pwm_fade: === PWM Fade LED Example (nRF54L15 DK) ===
[00:00:00.000,000] <inf> pwm_fade: Device : pwm@d0000
[00:00:00.000,000] <inf> pwm_fade: Channel: 0
[00:00:00.000,000] <inf> pwm_fade: Period : 20000000 ns (20 ms)
[00:00:00.000,000] <inf> pwm_fade: PWM device ready.
[00:00:00.000,000] <inf> pwm_fade: Fade step: 50 steps x 10 ms = 500 ms per ramp
[00:00:00.000,000] <inf> pwm_fade: Fade in...
[00:00:00.510,000] <inf> pwm_fade: Pause 1000 ms (LED fully on)
[00:00:01.516,000] <inf> pwm_fade: Fade out...
[00:00:02.026,000] <inf> pwm_fade: Pause 1000 ms (LED fully off)
[00:00:03.032,000] <inf> pwm_fade: Fade in...
...
```

With `CONFIG_PWM_LOG_LEVEL_DBG=y` each 10 ms PWM update is also printed:

```
[00:00:00.510,000] <dbg> pwm_nrfx: pwm_nrfx_set_cycles: channel 0, pulse 320000, period 320000, prescaler: 4.
```

The hardware converts the 20 ms period to 320 000 timer counts at 16 MHz with prescaler 4.

Serial settings:

| Parameter | Value |
|---|---|
| Baud rate | 115200 |
| Data bits | 8 |
| Stop bits | 1 |
| Parity | None |

## Project structure

```
pwm_fade_led/
├── CMakeLists.txt
├── prj.conf
├── README.md
├── .gitignore
├── boards/
│   └── nrf54l15dk_nrf54l15_cpuapp.overlay
└── src/
    └── main.c
```

## How it works

### Devicetree overlay

The overlay (`boards/nrf54l15dk_nrf54l15_cpuapp.overlay`) explicitly configures the full PWM hardware chain:

1. **`&pinctrl`** — defines `pwm20_default_custom` / `pwm20_sleep_custom` states that route PWM20 output 0 to P1.10.
2. **`&pwm20`** — assigns the custom pinctrl states and enables the peripheral.
3. **`/ { pwm_leds_custom }`** — declares a `pwm-leds` node with one channel (`&pwm20`, channel 0, 20 ms period).
4. **`aliases { pwm-led0 }`** — points to the node above so the application can use `DT_ALIAS(pwm_led0)`.

### Application (`src/main.c`)

```c
static const struct pwm_dt_spec pwm_led = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));
```

`PWM_DT_SPEC_GET` resolves at compile time to the device pointer, channel number, period, and polarity from the devicetree. At runtime:

- `pwm_is_ready_dt()` — verifies the driver initialised successfully.
- `pwm_set_pulse_dt()` — updates only the on-time (pulse width) each 10 ms step, keeping the period fixed.

The fade is linear: `pulse = step_index × (period / NUM_STEPS)`.

## Troubleshooting

### `west: unknown command "build"; do you need to run this inside a workspace?`

`ZEPHYR_BASE` is not set or not pointing at the NCS workspace. In each PowerShell session, run:

```powershell
$env:ZEPHYR_BASE = "D:\work\ncs\v3.3.0-rc2\zephyr"
```

west must also be able to locate `west.yml` — run `west build` from inside the NCS workspace checkout (`D:\work\ncs\v3.3.0-rc2`) or set the environment before `cd`-ing into the project.

### `Error: PWM device ... is not ready`

The PWM peripheral failed to initialise. Check:

1. The overlay is being picked up: the build log should show `Found devicetree overlay: .../boards/nrf54l15dk_nrf54l15_cpuapp.overlay`.
2. `CONFIG_PWM=y` is set in `prj.conf`.
3. Build with `--pristine` after any overlay change.

### `pwm_set_pulse_dt() failed: -22` (EINVAL)

The requested pulse width exceeds the period. This should not happen with the fade logic in this example, but if you modify `NUM_STEPS` to be 0 or change the period, verify `pulse <= period` at all times.

### LED1 does not light up at all

- Confirm you are observing **LED1** (second from left on the DK, labelled `LED1`). LED0, LED2, and LED3 are not driven by this example.
- Check the serial console for the startup banner — if it is missing, the application has not started. Try pressing the reset button.
- Verify the DK is enumerated with `nrfutil device list`.

### LED1 is always fully on or always fully off

The fade loop may be executing but `pwm_set_pulse_dt()` is failing silently. Enable `CONFIG_PWM_LOG_LEVEL_DBG=y` (already set in `prj.conf`) and watch the serial console for debug messages from the PWM driver.

### Two COM ports appear (COM80, COM81) — which one is the application UART?

The nRF54L15 DK exposes two JLink CDC UART ports:

| Port | Maps to | Use |
|---|---|---|
| VCOM0 | UART30 | Secondary / unused |
| VCOM1 | UART20 | Application console (this example) |

On the nRF54L15 DK the application UART (`uart20`) maps to **VCOM1**, which is the **second** JLink CDC port. This is the opposite convention from some other Nordic DKs.

The actual COM port numbers (e.g. COM80/COM81 on the development machine used here) are assigned by Windows and **will differ on every computer**. To find the correct ports, open *Device Manager → Ports (COM & LPT)* and identify the two **JLink CDC UART Port** entries while the DK is connected.

If no output appears on COM80, try COM81. The assignment can vary between host systems.

### Partition Manager deprecation warning during build

```
WARNING: SB_CONFIG_PARTITION_MANAGER is enabled, partition manager has been deprecated...
```

This is a harmless warning from NCS v3.3.0-rc2 sysbuild. The Partition Manager is still functional; boards will migrate to DTS-based partitioning in a future release. The warning does not affect this application.

### `west flash` fails with "No DK found"

Ensure the DK USB cable is connected to the **debug** USB port (marked `nRF USB` or with the debug label on the DK silkscreen). Run `nrfutil device list` to confirm the board is detected.
