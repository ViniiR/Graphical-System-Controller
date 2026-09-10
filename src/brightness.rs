use std::{cell::RefCell, rc::Rc};

use gtk::{
    gio::{DBusCallFlags, DBusConnection},
    glib::{self, g_warning, object::ObjectExt, variant::ToVariant, SignalHandlerId, VariantTy},
    prelude::RangeExt,
    Builder, Image, Label, Scale,
};

use crate::types::HandlerError;
use crate::types::{dbus, Program};
use crate::ui::get_brightness_icon;

pub fn handle_brightness<'a>(
    builder: &'a Builder,
    conn: &'a DBusConnection,
) -> Result<(), HandlerError<'a>> {
    let scale = builder
        .object::<Scale>("brightness-scale")
        .ok_or(HandlerError::ObjectError("Failed to get brightness-scale"))?;
    let label =
        builder
            .object::<Label>("brightness-scale-label")
            .ok_or(HandlerError::ObjectError(
                "Failed to get brightness-scale-label",
            ))?;
    let image = builder
        .object::<Image>("brightness-scale-label-image")
        .ok_or(HandlerError::ObjectError(
            "Failed to get brightness-scale-label-image",
        ))?;

    let pending_timer: Rc<RefCell<Option<glib::SourceId>>> = Rc::new(RefCell::new(None));
    let last_sent_value = Rc::new(std::cell::Cell::new(u32::MAX));

    let signal = scale.connect_value_changed(glib::clone!(
        #[strong]
        label,
        #[strong]
        image,
        #[strong]
        conn,
        move |scale| {
            let value = scale.value() as u32;

            update_brightness(value, &label, &image, None, None);

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
                    move || {
                        timer_clone.borrow_mut().take();
                        last_sent_clone.set(value);

                        let res = conn.call_future(
                            Some(Program::BACKEND_NAME),
                            dbus::Controllers::BRIGHTNESS,
                            &dbus::Controllers::to_interface(dbus::Controllers::BRIGHTNESS),
                            dbus::Methods::SET_BRIGHTNESS,
                            Some(&(value,).to_variant()),
                            None,
                            DBusCallFlags::NONE,
                            dbus::Timeout::NONE,
                        );

                        glib::spawn_future_local(async move {
                            let res = res.await;

                            if let Err(e) = &res {
                                g_warning!(None, "DBus call error: {e:?}");
                            }
                        });
                    }
                ),
            );

            *pending_timer.borrow_mut() = Some(source_id);
        }
    ));

    // Get brightness on startup
    glib::spawn_future_local(glib::clone!(
        #[strong]
        conn,
        #[strong]
        label,
        #[strong]
        image,
        #[strong]
        scale,
        async move {
            let call = conn.call_future(
                Some(Program::BACKEND_NAME),
                dbus::Controllers::BRIGHTNESS,
                &dbus::Controllers::to_interface(dbus::Controllers::BRIGHTNESS),
                dbus::Methods::GET_BRIGHTNESS,
                None,
                Some(VariantTy::TUPLE),
                DBusCallFlags::NONE,
                dbus::Timeout::NONE,
            );

            match call.await {
                Ok(r) => {
                    let Some(p) = r.child_value(0).get::<u32>() else {
                        g_warning!(None, "GetBrightness callback returned incorrect value");
                        return;
                    };

                    update_brightness(p, &label, &image, Some(&scale), Some(&signal));
                }
                Err(e) => {
                    g_warning!(None, "DBus call error: {e:?}");
                }
            }
        }
    ));

    Ok(())
}

fn update_brightness(
    percentage: u32,
    label: &Label,
    image: &Image,
    scale: Option<&Scale>,
    signal: Option<&SignalHandlerId>,
) {
    label.set_text(&format!("{}", percentage));
    image.set_icon_name(Some(&get_brightness_icon(percentage)));

    if let (Some(s), Some(scl)) = (signal, scale) {
        scl.block_signal(s);
        scl.set_value(percentage as f64);
        scl.unblock_signal(s);
    }
}
