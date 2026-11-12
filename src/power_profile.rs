use gtk::{
    gio::DBusConnection,
    glib::{self, g_warning},
    prelude::{ButtonExt, WidgetExt},
    Builder, Button,
};

use crate::{
    clone_as, dbus,
    lib_utils::{
        types::{builder_object::BuilderObject, handler_error::HandlerError},
        ui,
    },
};

pub fn handle_power_profile(builder: &Builder, conn: &DBusConnection) -> Result<(), HandlerError> {
    let performance_btn: Button = builder.get_object("power-profile-button-performance")?;
    let balanced_btn: Button = builder.get_object("power-profile-button-balanced")?;
    let powersave_btn: Button = builder.get_object("power-profile-button-power-save")?;

    performance_btn.connect_clicked(clone_as!(
        #[strong],
        [conn, performance_btn, balanced_btn, powersave_btn],
        move |_| {
            glib::spawn_future_local(handle_click(
                conn.clone(),
                performance_btn.clone(),
                balanced_btn.clone(),
                powersave_btn.clone(),
                PowerProfile::Performance,
            ));
        }
    ));

    balanced_btn.connect_clicked(clone_as!(
        #[strong],
        [conn, performance_btn, balanced_btn, powersave_btn],
        move |_| {
            glib::spawn_future_local(handle_click(
                conn.clone(),
                performance_btn.clone(),
                balanced_btn.clone(),
                powersave_btn.clone(),
                PowerProfile::Balanced,
            ));
        }
    ));

    powersave_btn.connect_clicked(clone_as!(
        #[strong],
        [conn, performance_btn, balanced_btn, powersave_btn],
        move |_| {
            glib::spawn_future_local(handle_click(
                conn.clone(),
                performance_btn.clone(),
                balanced_btn.clone(),
                powersave_btn.clone(),
                PowerProfile::PowerSaver,
            ));
        }
    ));

    performance_btn.set_tooltip_text(Some("Performance(AC) profile"));
    balanced_btn.set_tooltip_text(Some("Balanced(BAT) profile"));
    powersave_btn.set_tooltip_text(Some("Power Saver(SAV) profile"));

    // Get power profile on startup
    glib::spawn_future_local(clone_as!(
        #[strong],
        [conn, performance_btn, balanced_btn, powersave_btn],
        async move {
            let res = dbus::PowerProfile::get(&conn).await;

            match &res {
                Ok(v) if let Some(profile) = v.child_value(0).get::<u32>() => {
                    let Ok(profile) = PowerProfile::try_from(profile) else {
                        g_warning!(None, "PowerProfile conversion error");
                        return;
                    };
                    update_buttons(profile, &performance_btn, &balanced_btn, &powersave_btn);
                }
                e => g_warning!(None, "DBus call error: {e:?}"),
            }
        }
    ));

    Ok(())
}

fn update_buttons(profile: PowerProfile, perf: &Button, balance: &Button, saver: &Button) {
    let result = match profile {
        PowerProfile::Performance => (true, false, false),
        PowerProfile::Balanced => (false, true, false),
        PowerProfile::PowerSaver => (false, false, true),
    };
    ui::update_button_active(perf, result.0);
    ui::update_button_active(balance, result.1);
    ui::update_button_active(saver, result.2);
}

async fn handle_click(
    conn: DBusConnection,
    perf: Button,
    balance: Button,
    saver: Button,
    value: PowerProfile,
) {
    update_buttons(value, &perf, &balance, &saver);
    let dbus_call = dbus::PowerProfile::set(&conn, value.into());
    match dbus_call.await {
        Ok(_) => (),
        e => g_warning!(None, "DBus call error: {e:?}"),
    }
}

// NOTE: matches backend's power profile
#[derive(Debug, Clone, Copy)]
enum PowerProfile {
    Performance = 0,
    Balanced = 1,
    PowerSaver = 2,
}
impl TryFrom<u32> for PowerProfile {
    type Error = HandlerError;
    fn try_from(value: u32) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::Performance),
            1 => Ok(Self::Balanced),
            2 => Ok(Self::PowerSaver),
            _ => Err(HandlerError::Conversion(format!(
                "Failed to convert {value} to PowerProfile"
            ))),
        }
    }
}
impl From<PowerProfile> for u32 {
    fn from(value: PowerProfile) -> Self {
        match value {
            PowerProfile::Performance => 0,
            PowerProfile::Balanced => 1,
            PowerProfile::PowerSaver => 2,
        }
    }
}
