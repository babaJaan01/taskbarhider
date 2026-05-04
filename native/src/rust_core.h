#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ThCore ThCore;

typedef enum ThAction : uint32_t {
    TH_ACTION_NONE = 0,
    TH_ACTION_SHOW_TASKBAR = 1,
    TH_ACTION_HIDE_TASKBAR = 2,
} ThAction;

typedef struct ThCoreConfig {
    uint8_t show_on_app_attention;
    uint8_t _reserved[7];
} ThCoreConfig;

typedef struct ThDecision {
    uint32_t action;
    uint8_t start_hide_delay;
    uint8_t cancel_hide_delay;
    uint8_t _reserved[2];
} ThDecision;

ThCore* th_core_new(ThCoreConfig config);
void th_core_free(ThCore* core);
void th_core_set_config(ThCore* core, ThCoreConfig config);
void th_core_force_shown(ThCore* core);

ThDecision th_core_on_visibility_update(ThCore* core, uint8_t has_visible_windows);
ThDecision th_core_on_main_tick(ThCore* core, uint8_t has_visible_windows, uint8_t mouse_in_taskbar_zone);
ThDecision th_core_on_hide_delay(ThCore* core, uint8_t has_visible_windows, uint8_t mouse_in_taskbar_zone);
ThDecision th_core_on_attention_event(ThCore* core);

#ifdef __cplusplus
}
#endif
