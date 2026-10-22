use gtk::{
    gio::DBusConnection,
    glib::{self, g_warning},
    Builder, Image, Label,
};

use crate::{dbus, lib_utils::types::HandlerError};

pub fn handle_battery(builder: &Builder, conn: &DBusConnection) -> Result<(), HandlerError> {
    let label =
        builder
            .object::<Label>("battery-button-box-label")
            .ok_or(HandlerError::ObjectError(
                "Failed to get battery label".to_string(),
            ))?;
    let image =
        builder
            .object::<Image>("battery-button-box-image")
            .ok_or(HandlerError::ObjectError(
                "Failed to get battery image".to_string(),
            ))?;

    glib::spawn_future_local(glib::clone!(
        #[strong]
        conn,
        async move {
            loop {
                let res = dbus::Battery::get(&conn).await;

                match res {
                    Ok(v) => {
                        let icon_name = v.child_value(0).get::<String>();
                        let percentage = v.child_value(1).get::<u32>();

                        match (icon_name, percentage) {
                            (Some(icon), Some(percentage)) => {
                                label.set_text(&format!("{percentage}%"));
                                image.set_icon_name(Some(&icon));
                            }
                            _ => {
                                g_warning!(None, "GetBattery returned invalid tuple types");
                            }
                        }
                    }
                    Err(e) => {
                        g_warning!(None, "DBus call error: {e:?}");
                    }
                }
                glib::timeout_future_seconds(1).await;
            }
        }
    ));

    Ok(())
}
