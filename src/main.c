/*
 * Copyright (c) 2026 Example Author
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file main.c
 * @brief PWM Fade LED example for nRF54L15 DK.
 *
 * Demonstrates the Zephyr PWM API by fading LED1 (P1.10) in and out:
 *   - Fade-in  : 0 → 100 % duty cycle in NUM_STEPS × STEP_SLEEP_MS = 500 ms
 *   - Pause    : PAUSE_MS (1 s)
 *   - Fade-out : 100 % → 0 duty cycle in NUM_STEPS × STEP_SLEEP_MS = 500 ms
 *   - Pause    : PAUSE_MS (1 s)
 *   - Repeat   : forever
 *
 * The PWM period and pin are defined in the devicetree overlay:
 *   boards/nrf54l15dk_nrf54l15_cpuapp.overlay
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_fade, LOG_LEVEL_INF);

/* -------------------------------------------------------------------------
 * Devicetree binding
 * The alias "pwm-led0" is defined in the overlay and resolves to:
 *   pwm20, channel 0, period PWM_MSEC(20), polarity normal
 * ------------------------------------------------------------------------- */
#define PWM_LED_NODE DT_ALIAS(pwm_led0)

static const struct pwm_dt_spec pwm_led = PWM_DT_SPEC_GET(PWM_LED_NODE);

/* -------------------------------------------------------------------------
 * Fade parameters
 * NUM_STEPS  : number of duty-cycle increments per ramp direction
 * STEP_SLEEP_MS : milliseconds between each PWM update (10 ms → 50 Hz update)
 * PAUSE_MS   : pause between fade-in and fade-out (1 s)
 * ------------------------------------------------------------------------- */
#define NUM_STEPS       50U
#define STEP_SLEEP_MS   10U
#define PAUSE_MS        1000U

int main(void)
{
	int ret;
	uint32_t step;
	uint32_t pulse;
	int32_t i;

	LOG_INF("=== PWM Fade LED Example (nRF54L15 DK) ===");
	LOG_INF("Device : %s", pwm_led.dev->name);
	LOG_INF("Channel: %u", pwm_led.channel);
	LOG_INF("Period : %u ns (%u ms)", pwm_led.period,
		pwm_led.period / 1000000U);

	/* ------------------------------------------------------------------
	 * Step 1 — Verify the PWM device is ready before use.
	 * pwm_is_ready_dt() checks that the device was successfully
	 * initialised by the kernel during boot.
	 * ------------------------------------------------------------------ */
	if (!pwm_is_ready_dt(&pwm_led)) {
		LOG_ERR("PWM device %s is not ready", pwm_led.dev->name);
		return -ENODEV;
	}

	LOG_INF("PWM device ready.");
	LOG_INF("Fade step: %u steps x %u ms = %u ms per ramp",
		NUM_STEPS, STEP_SLEEP_MS, NUM_STEPS * STEP_SLEEP_MS);

	/* Compute the pulse increment for a single step.
	 * step = period / NUM_STEPS  →  NUM_STEPS steps cover 0..100 % */
	step = pwm_led.period / NUM_STEPS;

	while (1) {
		/* ----------------------------------------------------------------
		 * Step 2 — Fade in: increase pulse width from 0 to full period.
		 * pwm_set_pulse_dt() keeps the period unchanged and only sets
		 * the on-time (pulse width).
		 * Updates are called every STEP_SLEEP_MS = 10 ms.
		 * ---------------------------------------------------------------- */
		LOG_INF("Fade in...");

		for (i = 0; i <= (int32_t)NUM_STEPS; i++) {
			pulse = (uint32_t)i * step;

			ret = pwm_set_pulse_dt(&pwm_led, pulse);
			if (ret != 0) {
				LOG_ERR("pwm_set_pulse_dt() failed (step %d): %d",
					i, ret);
			}

			k_sleep(K_MSEC(STEP_SLEEP_MS));
		}

		/* Ensure we end exactly at 100 % to avoid rounding artefacts */
		ret = pwm_set_pulse_dt(&pwm_led, pwm_led.period);
		if (ret != 0) {
			LOG_ERR("pwm_set_pulse_dt() 100%% failed: %d", ret);
		}

		LOG_INF("Pause %u ms (LED fully on)", PAUSE_MS);
		k_sleep(K_MSEC(PAUSE_MS));

		/* ----------------------------------------------------------------
		 * Step 3 — Fade out: decrease pulse width from full period to 0.
		 * ---------------------------------------------------------------- */
		LOG_INF("Fade out...");

		for (i = (int32_t)NUM_STEPS; i >= 0; i--) {
			pulse = (uint32_t)i * step;

			ret = pwm_set_pulse_dt(&pwm_led, pulse);
			if (ret != 0) {
				LOG_ERR("pwm_set_pulse_dt() failed (step %d): %d",
					i, ret);
			}

			k_sleep(K_MSEC(STEP_SLEEP_MS));
		}

		/* Ensure we end exactly at 0 % */
		ret = pwm_set_pulse_dt(&pwm_led, 0U);
		if (ret != 0) {
			LOG_ERR("pwm_set_pulse_dt() 0%% failed: %d", ret);
		}

		LOG_INF("Pause %u ms (LED fully off)", PAUSE_MS);
		k_sleep(K_MSEC(PAUSE_MS));
	}

	return 0;
}
