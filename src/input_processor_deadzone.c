/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_input_processor_deadzone

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <drivers/input_processor.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct deadzone_config {
    uint16_t threshold;
    uint16_t timeout_ms;
};

struct deadzone_data {
    uint16_t accumulator;
    bool threshold_exceeded;
    struct k_work_delayable timeout_work;
    const struct device *dev;
};

static void deadzone_timeout_callback(struct k_work *work) {
    struct k_work_delayable *d_work = k_work_delayable_from_work(work);
    struct deadzone_data *data = CONTAINER_OF(d_work, struct deadzone_data, timeout_work);

    LOG_DBG("Deadzone timeout, resetting accumulator (was %d)", data->accumulator);
    data->accumulator = 0;
    data->threshold_exceeded = false;
}

static int deadzone_handle_event(const struct device *dev, struct input_event *event,
                                 uint32_t param1, uint32_t param2,
                                 struct zmk_input_processor_state *state) {
    const struct deadzone_config *cfg = dev->config;
    struct deadzone_data *data = dev->data;

    /* Only filter REL_X/REL_Y events; pass everything else through */
    if (event->type != INPUT_EV_REL) {
        return ZMK_INPUT_PROC_CONTINUE;
    }
    if (event->code != INPUT_REL_X && event->code != INPUT_REL_Y) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    /* Reschedule idle timeout */
    k_work_reschedule(&data->timeout_work, K_MSEC(cfg->timeout_ms));

    /* Already past threshold — pass through */
    if (data->threshold_exceeded) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    /* Accumulate movement magnitude */
    int16_t abs_val = (event->value < 0) ? -event->value : event->value;
    uint32_t new_acc = (uint32_t)data->accumulator + abs_val;
    if (new_acc > UINT16_MAX) {
        new_acc = UINT16_MAX;
    }
    data->accumulator = (uint16_t)new_acc;

    LOG_DBG("Deadzone accumulator: %d / %d", data->accumulator, cfg->threshold);

    if (data->accumulator >= cfg->threshold) {
        data->threshold_exceeded = true;
        LOG_DBG("Deadzone threshold exceeded, passing events through");
        return ZMK_INPUT_PROC_CONTINUE;
    }

    /* Below threshold — suppress this event */
    return ZMK_INPUT_PROC_STOP;
}

static int deadzone_init(const struct device *dev) {
    struct deadzone_data *data = dev->data;
    data->dev = dev;
    k_work_init_delayable(&data->timeout_work, deadzone_timeout_callback);
    return 0;
}

static struct zmk_input_processor_driver_api deadzone_driver_api = {
    .handle_event = deadzone_handle_event,
};

#define DEADZONE_INST(n)                                                                           \
    static struct deadzone_data deadzone_data_##n = {};                                            \
    static const struct deadzone_config deadzone_config_##n = {                                    \
        .threshold = DT_INST_PROP(n, threshold),                                                   \
        .timeout_ms = DT_INST_PROP(n, timeout_ms),                                                 \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, deadzone_init, NULL, &deadzone_data_##n, &deadzone_config_##n,        \
                          POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &deadzone_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEADZONE_INST)
