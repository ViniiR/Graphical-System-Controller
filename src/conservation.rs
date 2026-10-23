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

pub fn handle_conservation(builder: &Builder, conn: &DBusConnection) -> Result<(), HandlerError> {
    let conservation: Button = builder.get_object("conservation-mode-button")?;

    conservation.connect_clicked(clone_as!(
        #[strong],
        [conn],
        move |btn| {
            glib::spawn_future_local(ui::button_click_handler(
                dbus::Conservation::toggle(&conn),
                btn.clone(),
            ));
        }
    ));

    // Get boost on startup
    glib::spawn_future_local(clone_as!(
        #[strong],
        [conn, conservation],
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
