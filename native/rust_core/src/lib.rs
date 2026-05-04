use std::ptr::NonNull;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct ThCoreConfig {
    pub show_on_app_attention: u8,
    pub _reserved: [u8; 7],
}

#[repr(u32)]
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum ThAction {
    None = 0,
    ShowTaskbar = 1,
    HideTaskbar = 2,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct ThDecision {
    pub action: u32,
    pub start_hide_delay: u8,
    pub cancel_hide_delay: u8,
    pub _reserved: [u8; 2],
}

impl ThDecision {
    const fn noop() -> Self {
        Self {
            action: ThAction::None as u32,
            start_hide_delay: 0,
            cancel_hide_delay: 0,
            _reserved: [0; 2],
        }
    }

    const fn show(cancel_hide_delay: bool) -> Self {
        Self {
            action: ThAction::ShowTaskbar as u32,
            start_hide_delay: 0,
            cancel_hide_delay: cancel_hide_delay as u8,
            _reserved: [0; 2],
        }
    }

    const fn hide() -> Self {
        Self {
            action: ThAction::HideTaskbar as u32,
            start_hide_delay: 0,
            cancel_hide_delay: 0,
            _reserved: [0; 2],
        }
    }

    const fn start_hide_delay() -> Self {
        Self {
            action: ThAction::None as u32,
            start_hide_delay: 1,
            cancel_hide_delay: 0,
            _reserved: [0; 2],
        }
    }
}

pub struct ThCore {
    config: ThCoreConfig,
    taskbar_hidden: bool,
    last_window_state: bool,
    hover_revealed: bool,
    hide_pending: bool,
}

impl ThCore {
    fn new(config: ThCoreConfig) -> Self {
        Self {
            config,
            taskbar_hidden: false,
            last_window_state: true,
            hover_revealed: false,
            hide_pending: false,
        }
    }

    fn schedule_hide(&mut self) -> ThDecision {
        if self.taskbar_hidden || self.hide_pending {
            return ThDecision::noop();
        }

        self.hide_pending = true;
        ThDecision::start_hide_delay()
    }

    fn on_visibility_update(&mut self, has_visible_windows: bool) -> ThDecision {
        self.last_window_state = has_visible_windows;

        if has_visible_windows {
            self.hide_pending = false;
            self.taskbar_hidden = false;
            self.hover_revealed = false;
            ThDecision::show(true)
        } else {
            self.schedule_hide()
        }
    }

    fn on_main_tick(&mut self, has_visible_windows: bool, mouse_in_taskbar_zone: bool) -> ThDecision {
        if has_visible_windows != self.last_window_state {
            return self.on_visibility_update(has_visible_windows);
        }

        if !self.taskbar_hidden {
            return ThDecision::noop();
        }

        if mouse_in_taskbar_zone && !self.hover_revealed {
            self.hover_revealed = true;
            ThDecision::show(false)
        } else if !mouse_in_taskbar_zone && self.hover_revealed {
            self.hover_revealed = false;
            ThDecision::hide()
        } else {
            ThDecision::noop()
        }
    }

    fn on_hide_delay(&mut self, has_visible_windows: bool, mouse_in_taskbar_zone: bool) -> ThDecision {
        self.hide_pending = false;

        if has_visible_windows {
            self.taskbar_hidden = false;
            self.hover_revealed = false;
            return ThDecision::show(false);
        }

        self.taskbar_hidden = true;

        if mouse_in_taskbar_zone {
            self.hover_revealed = true;
            ThDecision::show(false)
        } else {
            self.hover_revealed = false;
            ThDecision::hide()
        }
    }

    fn on_attention_event(&mut self) -> ThDecision {
        if self.config.show_on_app_attention == 0 {
            return ThDecision::noop();
        }

        self.hide_pending = false;
        self.taskbar_hidden = false;
        self.hover_revealed = false;
        ThDecision::show(true)
    }

    fn force_shown(&mut self) {
        self.hide_pending = false;
        self.taskbar_hidden = false;
        self.hover_revealed = false;
        self.last_window_state = true;
    }
}

fn with_core_mut(core: *mut ThCore, f: impl FnOnce(&mut ThCore) -> ThDecision) -> ThDecision {
    let Some(mut core) = NonNull::new(core) else {
        return ThDecision::noop();
    };
    f(unsafe { core.as_mut() })
}

#[no_mangle]
pub extern "C" fn th_core_new(config: ThCoreConfig) -> *mut ThCore {
    Box::into_raw(Box::new(ThCore::new(config)))
}

#[no_mangle]
pub extern "C" fn th_core_free(core: *mut ThCore) {
    if let Some(core) = NonNull::new(core) {
        unsafe {
            drop(Box::from_raw(core.as_ptr()));
        }
    }
}

#[no_mangle]
pub extern "C" fn th_core_set_config(core: *mut ThCore, config: ThCoreConfig) {
    if let Some(mut core) = NonNull::new(core) {
        unsafe {
            core.as_mut().config = config;
        }
    }
}

#[no_mangle]
pub extern "C" fn th_core_force_shown(core: *mut ThCore) {
    if let Some(mut core) = NonNull::new(core) {
        unsafe {
            core.as_mut().force_shown();
        }
    }
}

#[no_mangle]
pub extern "C" fn th_core_on_visibility_update(
    core: *mut ThCore,
    has_visible_windows: u8,
) -> ThDecision {
    with_core_mut(core, |core| core.on_visibility_update(has_visible_windows != 0))
}

#[no_mangle]
pub extern "C" fn th_core_on_main_tick(
    core: *mut ThCore,
    has_visible_windows: u8,
    mouse_in_taskbar_zone: u8,
) -> ThDecision {
    with_core_mut(core, |core| {
        core.on_main_tick(has_visible_windows != 0, mouse_in_taskbar_zone != 0)
    })
}

#[no_mangle]
pub extern "C" fn th_core_on_hide_delay(
    core: *mut ThCore,
    has_visible_windows: u8,
    mouse_in_taskbar_zone: u8,
) -> ThDecision {
    with_core_mut(core, |core| {
        core.on_hide_delay(has_visible_windows != 0, mouse_in_taskbar_zone != 0)
    })
}

#[no_mangle]
pub extern "C" fn th_core_on_attention_event(core: *mut ThCore) -> ThDecision {
    with_core_mut(core, |core| core.on_attention_event())
}

#[cfg(test)]
mod tests {
    use super::*;

    fn config(show_on_app_attention: bool) -> ThCoreConfig {
        ThCoreConfig {
            show_on_app_attention: show_on_app_attention as u8,
            _reserved: [0; 7],
        }
    }

    #[test]
    fn schedules_hide_when_desktop_becomes_empty() {
        let mut core = ThCore::new(config(false));
        let decision = core.on_visibility_update(false);
        assert_eq!(decision.start_hide_delay, 1);
        assert!(core.hide_pending);
    }

    #[test]
    fn hover_reveals_and_leaving_hides_after_delayed_hide() {
        let mut core = ThCore::new(config(false));
        core.on_visibility_update(false);
        assert_eq!(
            core.on_hide_delay(false, false).action,
            ThAction::HideTaskbar as u32
        );
        assert_eq!(
            core.on_main_tick(false, true).action,
            ThAction::ShowTaskbar as u32
        );
        assert_eq!(
            core.on_main_tick(false, false).action,
            ThAction::HideTaskbar as u32
        );
    }

    #[test]
    fn visible_window_cancels_pending_hide() {
        let mut core = ThCore::new(config(false));
        core.on_visibility_update(false);
        let decision = core.on_visibility_update(true);
        assert_eq!(decision.action, ThAction::ShowTaskbar as u32);
        assert_eq!(decision.cancel_hide_delay, 1);
        assert!(!core.hide_pending);
    }

    #[test]
    fn attention_event_respects_config() {
        let mut disabled = ThCore::new(config(false));
        assert_eq!(disabled.on_attention_event(), ThDecision::noop());

        let mut enabled = ThCore::new(config(true));
        enabled.on_visibility_update(false);
        let decision = enabled.on_attention_event();
        assert_eq!(decision.action, ThAction::ShowTaskbar as u32);
        assert_eq!(decision.cancel_hide_delay, 1);
    }
}
