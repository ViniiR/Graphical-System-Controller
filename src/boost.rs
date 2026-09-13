use gtk::{
    gio::{DBusCallFlags, DBusConnection},
    glib::{self, g_warning, VariantTy},
    prelude::ButtonExt,
    Builder, Button,
};

use crate::types::{dbus, Program};
use crate::{types::HandlerError, ui};

async fn click_handler(conn: DBusConnection, button: Button) {
    let res = conn
        .call_future(
            Some(Program::BACKEND_NAME),
            dbus::Controllers::BOOST,
            &dbus::Controllers::to_interface(dbus::Controllers::BOOST),
            dbus::Methods::TOGGLE_BOOST,
            None,
            Some(VariantTy::TUPLE),
            DBusCallFlags::NONE,
            dbus::Timeout::NONE,
        )
        .await;

    match &res {
        Ok(_v) if let Some(b) = _v.child_value(0).get::<bool>() => {
            ui::update_button_active(&button, b)
        }
        e => g_warning!(None, "DBus call error: {e:?}"),
    };
}

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
            glib::spawn_future_local(click_handler(conn.clone(), btn.clone()));
        }
    ));

    // Get boost on startup
    glib::spawn_future_local(glib::clone!(
        #[strong]
        conn,
        #[strong]
        boost,
        async move {
            let res = conn
                .call_future(
                    Some(Program::BACKEND_NAME),
                    dbus::Controllers::BOOST,
                    &dbus::Controllers::to_interface(dbus::Controllers::BOOST),
                    dbus::Methods::GET_BOOST,
                    None,
                    Some(VariantTy::TUPLE),
                    DBusCallFlags::NONE,
                    dbus::Timeout::NONE,
                )
                .await;

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
