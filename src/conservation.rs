use gtk::{
    gio::DBusConnection,
    glib::{self, g_warning},
    prelude::ButtonExt,
    Builder, Button,
};

use crate::{dbus, types::HandlerError, ui};

pub fn handle_conservation(builder: &Builder, conn: &DBusConnection) -> Result<(), HandlerError> {
    let conservation =
        builder
            .object::<Button>("conservation-mode-button")
            .ok_or(HandlerError::ObjectError(
                "Failed to get conservation-mode-button".to_string(),
            ))?;

    conservation.connect_clicked(glib::clone!(
        #[strong]
        conn,
        move |btn| {
            glib::spawn_future_local(ui::button_click_handler(
                dbus::Conservation::toggle(&conn),
                btn.clone(),
            ));
        }
    ));

    // Get boost on startup
    glib::spawn_future_local(glib::clone!(
        #[strong]
        conn,
        #[strong]
        conservation,
        async move {
            let res = dbus::Conservation::get(&conn).await;

            match &res {
                Ok(v) if let Some(b) = v.child_value(0).get::<bool>() => {
                    ui::update_button_active(&conservation, b);
                }
                e => g_warning!(None, "DBus call error: {e:?}"),
            }
        }
    ));
    Ok(())
}
