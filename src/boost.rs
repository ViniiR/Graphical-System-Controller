use gtk::{
    gio::DBusConnection,
    glib::{self, g_warning},
    prelude::ButtonExt,
    Builder, Button,
};

use crate::{dbus, types::HandlerError, ui};

pub fn handle_boost(builder: &Builder, conn: &DBusConnection) -> Result<(), HandlerError> {
    let boost = builder
        .object::<Button>("boost-mode-button")
        .ok_or(HandlerError::ObjectError(
            "Failed to get boost-mode-button".to_string(),
        ))?;

    boost.connect_clicked(glib::clone!(
        #[strong]
        conn,
        move |btn| {
            glib::spawn_future_local(ui::button_click_handler(
                dbus::Boost::toggle(&conn),
                btn.clone(),
            ));
        }
    ));

    // Get boost on startup
    glib::spawn_future_local(glib::clone!(
        #[strong]
        conn,
        #[strong]
        boost,
        async move {
            let res = dbus::Boost::get(&conn).await;

            match &res {
                Ok(_v) if let Some(b) = _v.child_value(0).get::<bool>() => {
                    ui::update_button_active(&boost, b)
                }
                e => g_warning!(None, "DBus call error: {e:?}"),
            };
        }
    ));

    Ok(())
}
