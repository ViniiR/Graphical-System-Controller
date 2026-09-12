use std::{cell::RefCell, ops::Deref, rc::Rc};

use gtk::{
    gio::{DBusCallFlags, DBusConnection},
    glib::{self, g_warning, object::ObjectExt, variant::ToVariant, SignalHandlerId, VariantTy},
    prelude::{RangeExt, WidgetExt},
    Box, Builder, GestureClick, Image, Label, Scale,
};

use crate::types::{dbus, Program, State};
use crate::{types::HandlerError, ui::get_volume_icon};

pub fn handle_audio<'a>(
    builder: &'a Builder,
    conn: &'a DBusConnection,
) -> Result<(), HandlerError<'a>> {
    let scale = builder
        .object::<Scale>("volume-scale")
        .ok_or(HandlerError::ObjectError("Failed to get volume-scale"))?;
    let label = builder
        .object::<Label>("volume-scale-label")
        .ok_or(HandlerError::ObjectError(
            "Failed to get volume-scale-label",
        ))?;
    let label_box =
        builder
            .object::<Box>("volume-scale-label-box")
            .ok_or(HandlerError::ObjectError(
                "Failed to get volume-scale-label-box",
            ))?;
    let label_image =
        builder
            .object::<Image>("volume-scale-label-image")
            .ok_or(HandlerError::ObjectError(
                "Failed to get volume-scale-label-image",
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
            let value = scale.value() as u32;

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
            let call = conn.call_future(
                Some(Program::BACKEND_NAME),
                dbus::Controllers::AUDIO,
                &dbus::Controllers::to_interface(dbus::Controllers::AUDIO),
                dbus::Methods::GET_AUDIO,
                None,
                Some(VariantTy::TUPLE),
                DBusCallFlags::NONE,
                dbus::Timeout::NONE,
            );
            match call.await {
                Ok(v) => {
                    let mut state = state.borrow_mut();
                    state.update(v);
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

//macro_rules! update_volume {
//    (
//        $volume:expr,
//        $label:ident,
//        $image:ident,
//        $scale:ident,
//        $signal:ident
//    ) => {{
//        let vol: u32 = $volume;
//        let lbl: Label = $label;
//        let img: Image = $image;
//        let scl: Scale = $scale;
//        let sig: Scale = $scale;
//
//        lbl.set_text(&format!("{}", vol));
//        //img.set_icon_name(Some(&get_volume_icon(vol, state.is_muted)));
//
//        scl.block_signal(sig);
//        scl.set_value(vol.into());
//        scl.unblock_signal(sig);
//    }};
//}

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
    image.set_icon_name(Some(&get_volume_icon(state.volume, state.is_muted)));

    if let Some(signal) = signal {
        scale.block_signal(signal);
        scale.set_value(state.volume.into());
        scale.unblock_signal(signal);
    }
}

async fn handle_scale_update_timeout(value: u32, conn: DBusConnection, state: Rc<RefCell<State>>) {
    let res = conn
        .call_future(
            Some(Program::BACKEND_NAME),
            dbus::Controllers::AUDIO,
            &dbus::Controllers::to_interface(dbus::Controllers::AUDIO),
            dbus::Methods::SET_AUDIO,
            Some(&(value,).to_variant()),
            Some(VariantTy::TUPLE),
            DBusCallFlags::NONE,
            dbus::Timeout::NONE,
        )
        .await;
    match res {
        Ok(v) => {
            let mut state = state.borrow_mut();
            state.update(v);
        }
        Err(e) => g_warning!(None, "DBus call error: {e:?}"),
    }
}

async fn handle_mute_click(conn: DBusConnection, image: Image) {
    let res = conn
        .call_future(
            Some(Program::BACKEND_NAME),
            dbus::Controllers::AUDIO,
            &dbus::Controllers::to_interface(dbus::Controllers::AUDIO),
            dbus::Methods::TOGGLE_AUDIO_MUTED,
            None,
            None,
            DBusCallFlags::NONE,
            dbus::Timeout::NONE,
        )
        .await;
    if let Err(e) = &res {
        g_warning!(None, "DBus call error: {e:?}");
        return;
    }

    let get = conn
        .call_future(
            Some(Program::BACKEND_NAME),
            dbus::Controllers::AUDIO,
            &dbus::Controllers::to_interface(dbus::Controllers::AUDIO),
            dbus::Methods::GET_AUDIO,
            None,
            Some(VariantTy::TUPLE),
            DBusCallFlags::NONE,
            dbus::Timeout::NONE,
        )
        .await;
    if let Err(e) = &get {
        g_warning!(None, "DBus call error: {e:?}");
        return;
    }
    let get = get.unwrap();

    let volume = get.child_value(0).get::<u32>();
    let is_muted = get.child_value(1).get::<bool>();
    match (volume, is_muted) {
        (Some(v), Some(m)) => image.set_icon_name(Some(&get_volume_icon(v, m))),
        _ => g_warning!(None, "DBus unknown call error"),
    }
}
