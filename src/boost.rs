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

pub fn handle_boost(builder: &Builder, conn: &DBusConnection) -> Result<(), HandlerError> {
    let boost: Button = builder.get_object("boost-mode-button")?;

    boost.connect_clicked(clone_as!(
        #[strong],
        [conn],
        move |btn| {
            glib::spawn_future_local(ui::button_click_handler(
                dbus::Boost::toggle(&conn),
                btn.clone(),
            ));
        }
    ));

    // Get boost on startup
    glib::spawn_future_local(clone_as!(
        #[strong],
        [conn, boost],
        async move {
            let res = dbus::Boost::get(&conn).await;

            match &res {
                Ok(v) if let Some(b) = v.child_value(0).get::<bool>() => {
                    ui::update_button_active(&boost, b);
                }
                e => g_warning!(None, "DBus call error: {e:?}"),
            }
        }
    ));

    Ok(())
}
