use std::{cell::RefCell, ops::Deref, rc::Rc};

use gtk::{
    gio::DBusConnection,
    glib::{self, g_warning, object::ObjectExt, SignalHandlerId},
    prelude::{RangeExt, WidgetExt},
    Box, Builder, GestureClick, Image, Label, Scale,
};

use crate::{
    dbus,
    lib_utils::transform::LossyU32Cast,
    types::{HandlerError, State},
    ui::get_volume_icon,
};

// TODO: lessen lines on this func
pub fn handle_audio(builder: &Builder, conn: &DBusConnection) -> Result<(), HandlerError> {
    let scale = builder
        .object::<Scale>("volume-scale")
        .ok_or(HandlerError::ObjectError(
            "Failed to get volume-scale".to_string(),
        ))?;
    let label = builder
        .object::<Label>("volume-scale-label")
        .ok_or(HandlerError::ObjectError(
            "Failed to get volume-scale-label".to_string(),
        ))?;
    let label_box =
        builder
            .object::<Box>("volume-scale-label-box")
            .ok_or(HandlerError::ObjectError(
                "Failed to get volume-scale-label-box".to_string(),
            ))?;
    let label_image =
        builder
            .object::<Image>("volume-scale-label-image")
            .ok_or(HandlerError::ObjectError(
                "Failed to get volume-scale-label-image".to_string(),
            ))?;

    let state = Rc::new(RefCell::new(State::default()));

    let pending_timer: Rc<RefCell<Option<glib::SourceId>>> = Rc::new(RefCell::new(None));
    let last_sent_value = Rc::new(std::cell::Cell::new(u32::MAX));

    let signal = scale.connect_value_changed(glib::clone!(
        #[strong]
        label,
        #[strong]
        label_image,
        #[strong]
        conn,
        #[strong]
        state,
        move |scale| {
            let value = scale.value().to_lossy_u32();

            {
                let mut s = state.borrow_mut();
                s.set_volume(value);
            }
            update_volume(&state.borrow(), &label, &label_image, scale, None);

            if value == last_sent_value.get() {
                return;
            }
            if let Some(source_id) = pending_timer.borrow_mut().take() {
                source_id.remove();
            }

            let timer_clone = pending_timer.clone();
            let last_sent_clone = last_sent_value.clone();

            let source_id = glib::timeout_add_local_once(
                std::time::Duration::from_millis(50),
                glib::clone!(
                    #[strong]
                    conn,
                    #[strong]
                    state,
                    move || {
                        timer_clone.borrow_mut().take();
                        last_sent_clone.set(value);

                        glib::spawn_future_local(handle_scale_update_timeout(value, conn, state));
                    }
                ),
            );

            *pending_timer.borrow_mut() = Some(source_id);
        }
    ));

    let signal_rc = Rc::new(signal);

    // Get volume on startup
    glib::spawn_future_local(glib::clone!(
        #[strong]
        label,
        #[strong]
        scale,
        #[strong]
        conn,
        #[strong]
        label_image,
        #[strong]
        signal_rc,
        #[strong]
        state,
        async move {
            let call = dbus::Audio::get(&conn);
            match call.await {
                Ok(v) => {
                    let mut state = state.borrow_mut();
                    state.update(&v);
                    update_volume(&state, &label, &label_image, &scale, Some(&signal_rc));
                }
                Err(e) => g_warning!(None, "DBus call error: {e:?}"),
            }
        }
    ));

    let click_controller = GestureClick::new();
    click_controller.connect_pressed(glib::clone!(
        #[strong]
        conn,
        #[strong]
        label_image,
        #[strong]
        label,
        #[strong]
        scale,
        move |_gesture, _n_press, _x, _y| {
            {
                let mut state = state.borrow_mut();
                state.toggle_muted();
            }
            update_volume(&state.borrow(), &label, &label_image, &scale, None);
            glib::spawn_future_local(handle_mute_click(conn.clone(), label_image.clone()));
        }
    ));
    label_box.add_controller(click_controller);

    Ok(())
}

//

fn update_volume<T>(
    state: &T,
    label: &Label,
    image: &Image,
    scale: &Scale,
    signal: Option<&SignalHandlerId>,
) where
    T: Deref<Target = State>,
{
    label.set_text(&format!("{}", state.volume));
    image.set_icon_name(Some(get_volume_icon(state.volume, state.is_muted)));

    if let Some(signal) = signal {
        scale.block_signal(signal);
        scale.set_value(state.volume.into());
        scale.unblock_signal(signal);
    }
}

async fn handle_scale_update_timeout(value: u32, conn: DBusConnection, state: Rc<RefCell<State>>) {
    let res = dbus::Audio::set(&conn, value).await;
    match res {
        Ok(v) => {
            let mut state = state.borrow_mut();
            state.update(&v);
        }
        Err(e) => g_warning!(None, "DBus call error: {e:?}"),
    }
}

async fn handle_mute_click(conn: DBusConnection, image: Image) {
    let res = dbus::Audio::toggle_muted(&conn).await;
    if let Err(e) = &res {
        g_warning!(None, "DBus call error: {e:?}");
        return;
    }

    let get = dbus::Audio::get(&conn).await;
    match get {
        Ok(v) => {
            let volume = v.child_value(0).get::<u32>();
            let is_muted = v.child_value(1).get::<bool>();
            match (volume, is_muted) {
                (Some(v), Some(m)) => image.set_icon_name(Some(get_volume_icon(v, m))),
                _ => g_warning!(None, "DBus unknown call error"),
            }
        }
        Err(e) => {
            g_warning!(None, "DBus call error: {e:?}");
        }
    }
}
