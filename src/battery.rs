use gtk::{
    gio::{DBusCallFlags, DBusConnection},
    glib::{self, g_warning, VariantTy},
    Builder, Image, Label,
};

use crate::types::{dbus, HandlerError, Program};

pub fn handle_battery<'a>(
    builder: &'a Builder,
    conn: &'a DBusConnection,
) -> Result<(), HandlerError<'a>> {
    let label = builder
        .object::<Label>("battery-button-box-label")
        .ok_or(HandlerError::ObjectError("Failed to get battery label"))?;
    let image = builder
        .object::<Image>("battery-button-box-image")
        .ok_or(HandlerError::ObjectError("Failed to get battery image"))?;

    glib::spawn_future_local(glib::clone!(
        #[strong]
        conn,
        async move {
            #[allow(clippy::never_loop)]
            loop {
                let res = conn
                    .call_future(
                        Some(Program::BACKEND_NAME),
                        dbus::Controllers::BATTERY,
                        &dbus::Controllers::to_interface(dbus::Controllers::BATTERY),
                        dbus::Methods::GET_BATTERY,
                        None,
                        Some(VariantTy::TUPLE), // "su"
                        DBusCallFlags::NONE,
                        dbus::Timeout::NONE,
                    )
                    .await;

                if let Err(e) = res {
                    g_warning!(None, "DBus call error: {e:?}");
                    return;
                }
                let res = res.unwrap();

                let icon_name = res.child_value(0).get::<String>();
                let percentage = res.child_value(1).get::<u32>();

                match (icon_name, percentage) {
                    (Some(icon), Some(percentage)) => {
                        label.set_text(&format!("{percentage}%"));
                        image.set_icon_name(Some(&icon));
                    }
                    _ => {
                        g_warning!(None, "GetBattery returned invalid tuple types");
                    }
                }

                glib::timeout_future_seconds(1).await;
            }
        }
    ));

    Ok(())
}
