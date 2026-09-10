use gtk::{
    gio::{DBusCallFlags, DBusConnection},
    glib::{self, g_warning, VariantTy},
    prelude::ButtonExt,
    Builder, Button,
};

use crate::types::{dbus, Program};
use crate::{types::HandlerError, ui};

// TODO: unify function with boost click_handler
async fn click_handler(conn: DBusConnection, button: Button) {
    let res = conn
        .call_future(
            Some(Program::BACKEND_NAME),
            dbus::Controllers::CONSERVATION,
            &dbus::Controllers::to_interface(dbus::Controllers::CONSERVATION),
            dbus::Methods::TOGGLE_CONSERVATION,
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

pub fn handle_conservation<'a>(
    builder: &'a Builder,
    conn: &'a DBusConnection,
) -> Result<(), HandlerError<'a>> {
    let conservation =
        builder
            .object::<Button>("conservation-mode-button")
            .ok_or(HandlerError::ObjectError(
                "Failed to get conservation-mode-button",
            ))?;

    conservation.connect_clicked(glib::clone!(
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
        conservation,
        async move {
            let res = conn
                .call_future(
                    Some(Program::BACKEND_NAME),
                    dbus::Controllers::CONSERVATION,
                    &dbus::Controllers::to_interface(dbus::Controllers::CONSERVATION),
                    dbus::Methods::GET_CONSERVATION,
                    None,
                    Some(VariantTy::TUPLE),
                    DBusCallFlags::NONE,
                    dbus::Timeout::NONE,
                )
                .await;

            match &res {
                Ok(_v) if let Some(b) = _v.child_value(0).get::<bool>() => {
                    ui::update_button_active(&conservation, b)
                }
                e => g_warning!(None, "DBus call error: {e:?}"),
            };
        }
    ));
    Ok(())
}
