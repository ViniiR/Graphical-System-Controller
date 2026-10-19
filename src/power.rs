use gtk::{
    gio::DBusConnection,
    glib::{self, g_warning},
    prelude::ButtonExt,
    Builder, Button,
};

use crate::{dbus, types::HandlerError};

pub fn handle_power(builder: &Builder, conn: &DBusConnection) -> Result<(), HandlerError> {
    let shutdown = builder
        .object::<Button>("shutdown-button")
        .ok_or(HandlerError::ObjectError(
            "Failed to get shutdown-button".to_string(),
        ))?;
    let reboot = builder
        .object::<Button>("reboot-button")
        .ok_or(HandlerError::ObjectError(
            "Failed to get reboot-button".to_string(),
        ))?;
    let logout = builder
        .object::<Button>("logout-button")
        .ok_or(HandlerError::ObjectError(
            "Failed to get logout-button".to_string(),
        ))?;

    shutdown.connect_clicked(glib::clone!(
        #[strong]
        conn,
        move |_button| {
            let conn = conn.clone();
            glib::spawn_future_local(async move {
                let res = dbus::Power::shutdown(&conn).await;

                if let Err(e) = &res {
                    g_warning!(None, "DBus call error: {e:?}");
                }
            });
        }
    ));
    reboot.connect_clicked(glib::clone!(
        #[strong]
        conn,
        move |_button| {
            let conn = conn.clone();
            glib::spawn_future_local(async move {
                let res = dbus::Power::reboot(&conn).await;

                if let Err(e) = &res {
                    g_warning!(None, "DBus call error: {e:?}");
                }
            });
        }
    ));
    logout.connect_clicked(glib::clone!(
        #[strong]
        conn,
        move |_button| {
            let conn = conn.clone();
            glib::spawn_future_local(async move {
                let res = dbus::Power::logout(&conn).await;

                if let Err(e) = &res {
                    g_warning!(None, "DBus call error: {e:?}");
                }
            });
        }
    ));

    Ok(())
}
