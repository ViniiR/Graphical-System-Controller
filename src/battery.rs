use std::fmt::Display;

use gtk::{
    gio::DBusConnection,
    glib::{self, g_warning},
    prelude::WidgetExt,
    Builder, Button, Image, Label,
};

use crate::{
    clone_as, dbus,
    lib_utils::types::{builder_object::BuilderObject, handler_error::HandlerError},
};

pub fn handle_battery(builder: &Builder, conn: &DBusConnection) -> Result<(), HandlerError> {
    let button: Button = builder.get_object("battery-button")?;
    let label: Label = builder.get_object("battery-button-box-label")?;
    let image: Image = builder.get_object("battery-button-box-image")?;

    glib::spawn_future_local(clone_as!(
        #[strong],
        [conn],
        async move {
            loop {
                if let Err(e) = request_battery(&conn,&button, &label, &image).await {
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
    button: &Button,
    label: &Label,
    image: &Image,
) -> Result<(), Box<dyn std::error::Error>> {
    let variant = dbus::Battery::get(conn).await?;

    let icon_name = variant.child_value(0).get::<String>();
    let percentage = variant.child_value(1).get::<u32>();
    let status = variant.child_value(2).get::<u32>();

    match (icon_name, percentage, status) {
        (Some(icon), Some(percentage), Some(status)) => {
            label.set_text(&format!("{percentage}%"));
            image.set_icon_name(Some(&icon));
            button.set_tooltip_text(Some(&format!(
                "Battery {percentage}% {}",
                ChargingStatus::try_from(status)?
            )));
            Ok(())
        }
        _ => Err("GetBattery returned invalid tuple types".into()),
    }
}

enum ChargingStatus {
    Charging = 0,
    Discharging = 1,
    PluggedIn = 2,
}
impl Display for ChargingStatus {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let value = match self {
            ChargingStatus::Charging => "Charging",
            ChargingStatus::Discharging => "Discharging",
            ChargingStatus::PluggedIn => "Plugged-in",
        };
        write!(f, "{value}")
    }
}
impl TryFrom<u32> for ChargingStatus {
    type Error = HandlerError;
    fn try_from(value: u32) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::Charging),
            1 => Ok(Self::Discharging),
            2 => Ok(Self::PluggedIn),
            _ => Err(HandlerError::ConversionError(format!(
                "Failed to convert {value} to ChargingStatus"
            ))),
        }
    }
}
