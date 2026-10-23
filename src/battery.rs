use gtk::{
    gio::DBusConnection,
    glib::{self, g_warning},
    Builder, Image, Label,
};

use crate::{
    clone_as, dbus,
    lib_utils::types::{BuilderObject, HandlerError},
};

pub fn handle_battery(builder: &Builder, conn: &DBusConnection) -> Result<(), HandlerError> {
    let label: Label = builder.get_object("battery-button-box-label")?;
    let image: Image = builder.get_object("battery-button-box-image")?;

    glib::spawn_future_local(clone_as!(
        #[strong],
        [conn],
        async move {
            loop {
                if let Err(e) = request_battery(&conn, &label, &image).await {
                    g_warning!(None, "{e:?}");
                }

                glib::timeout_future_seconds(1).await;
            }
        }
    ));

    Ok(())
}

async fn request_battery(
    conn: &DBusConnection,
    label: &Label,
    image: &Image,
) -> Result<(), Box<dyn std::error::Error>> {
    let variant = dbus::Battery::get(conn).await?;

    let icon_name = variant.child_value(0).get::<String>();
    let percentage = variant.child_value(1).get::<u32>();

    match (icon_name, percentage) {
        (Some(icon), Some(percentage)) => {
            label.set_text(&format!("{percentage}%"));
            image.set_icon_name(Some(&icon));
            Ok(())
        }
        _ => Err("GetBattery returned invalid tuple types".into()),
    }
}
