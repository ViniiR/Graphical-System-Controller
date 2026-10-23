use gtk::{
    gio::DBusConnection,
    glib::{self, g_warning},
    prelude::ButtonExt,
    Builder, Button,
};

use crate::{
    clone_as, dbus,
    lib_utils::{
        types::{BuilderObject, HandlerError},
        ui,
    },
};

pub fn handle_keyboard(builder: &Builder, conn: &DBusConnection) -> Result<(), HandlerError> {
    let dev_btn: Button = builder.get_object("keyboard-button-dev")?;
    let std_btn: Button = builder.get_object("keyboard-button-standard")?;

    dev_btn.connect_clicked(clone_as!(
        #[strong],
        [conn, std_btn],
        move |btn| {
            glib::spawn_future_local(handle_click(conn.clone(), std_btn.clone(), btn.clone()));
        }
    ));
    std_btn.connect_clicked(clone_as!(
        #[strong],
        [conn, dev_btn],
        move |btn| {
            glib::spawn_future_local(handle_click(conn.clone(), dev_btn.clone(), btn.clone()));
        }
    ));

    // Get keyboard on startup
    glib::spawn_future_local(clone_as!(
        #[strong],
        [conn, dev_btn, std_btn],
        async move {
            let res = dbus::Keyboard::get(&conn).await;

            match &res {
                Ok(v) if let Some(b) = v.child_value(0).get::<bool>() => {
                    ui::update_button_active(&std_btn, b);
                    ui::update_button_active(&dev_btn, !b);
                }
                e => g_warning!(None, "DBus call error: {e:?}"),
            }
        }
    ));

    Ok(())
}

async fn handle_click(conn: DBusConnection, std_btn: Button, dev_btn: Button) {
    let dbus_call = dbus::Keyboard::toggle(&conn);
    match dbus_call.await {
        Ok(_v) => {
            ui::toggle_button_active(&std_btn);
            ui::toggle_button_active(&dev_btn);
        }
        e => g_warning!(None, "DBus call error: {e:?}"),
    }
}
