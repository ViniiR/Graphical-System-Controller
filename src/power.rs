use gtk::{
    gio::DBusConnection,
    glib::{self, g_warning},
    prelude::ButtonExt,
    Builder, Button,
};

use crate::{
    dbus,
    lib_utils::types::{BuilderObject, HandlerError},
};

pub fn handle_power(builder: &Builder, conn: &DBusConnection) -> Result<(), HandlerError> {
    let shutdown: Button = builder.get_object("shutdown-button")?;
    let reboot: Button = builder.get_object("reboot-button")?;
    let logout: Button = builder.get_object("logout-button")?;

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
