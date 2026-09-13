use gtk::{
    gio::{DBusCallFlags, DBusConnection},
    glib::{self, g_warning, SignalHandlerId},
    prelude::ButtonExt,
    Builder, Button,
};

use crate::types::HandlerError;
use crate::types::{dbus, Program};

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

    fn make_power_connection(
        button: &Button,
        method: &'static str,
        conn: &DBusConnection,
    ) -> SignalHandlerId {
        button.connect_clicked(glib::clone!(
            #[strong]
            conn,
            move |_button| {
                let res = conn.call_future(
                    Some(Program::BACKEND_NAME),
                    dbus::Controllers::POWER,
                    &dbus::Controllers::to_interface(dbus::Controllers::POWER),
                    method,
                    None,
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
        ))
    }

    make_power_connection(&shutdown, dbus::Methods::SHUTDOWN, conn);
    make_power_connection(&reboot, dbus::Methods::REBOOT, conn);
    make_power_connection(&logout, dbus::Methods::LOGOUT, conn);

    Ok(())
}
