use std::{error::Error, path::PathBuf, process::Command};

use gtk::{
    glib::{self, g_warning, Variant},
    prelude::WidgetExt,
    Button,
};

use crate::{lib_utils::types::handler_error::HandlerError, notification::Notification};

const ACTIVE_CLASS: &str = "button-active";
const INACTIVE_CLASS: &str = "button-inactive";

pub fn update_button_active(btn: &Button, to_active: bool) {
    btn.add_css_class(if to_active {
        ACTIVE_CLASS
    } else {
        INACTIVE_CLASS
    });
    btn.remove_css_class(if to_active {
        INACTIVE_CLASS
    } else {
        ACTIVE_CLASS
    });
}

pub fn toggle_button_active(btn: &Button) {
    update_button_active(btn, !btn.has_css_class(ACTIVE_CLASS));
}

pub fn get_volume_icon(percentage: u32, is_muted: bool) -> &'static str {
    if is_muted {
        return "audio-volume-muted-symbolic";
    }

    match percentage {
        0 => "audio-volume-zero-symbolic",
        1..=40 => "audio-volume-low-symbolic",
        41..=60 => "audio-volume-medium-symbolic",
        _ => "audio-volume-high-symbolic",
    }
}

pub fn get_brightness_icon(_percentage: u32) -> &'static str {
    "display-brightness-symbolic"
    //match percentage {
    //    0..=40 => String::from("display-brightness-low-symbolic"),
    //    41..=60 => String::from("display-brightness-medium-symbolic"),
    //    _ => String::from("display-brightness-high-symbolic"),
    //}
}

pub fn get_individual_audio_icon(name: &str) -> &'static str {
    if name.contains("YouTube") {
        "firefox-logo"
    } else if name.contains("MPD Pulse Audio Output") {
        "audio-headset-symbolic"
    } else {
        "audio-card-symbolic"
    }
}

pub async fn button_click_handler(
    dbus_call: impl Future<Output = Result<Variant, glib::Error>>,
    button: Button,
) {
    match dbus_call.await {
        Ok(v) if let Some(b) = v.child_value(0).get::<bool>() => update_button_active(&button, b),
        e => g_warning!(None, "DBus call error: {e:?}"),
    }
}

pub fn parse_notification_icon(notification: Notification) -> Result<String, Box<dyn Error>> {
    match notification.app.as_str() {
        "discord" => {
            let discord_path = Command::new("which").arg("discord").output()?;
            let output = Command::new("readlink")
                .args(["-f", &*String::from_utf8_lossy(&discord_path.stdout)])
                .output()?;
            let dc_root: String = String::from_utf8_lossy(&output.stdout).into();
            let mut icon_path = PathBuf::from(dc_root);
            icon_path.pop();
            icon_path.pop();
            icon_path.push("share/icons/hicolor/256x256/apps/discord.png");
            Ok(icon_path
                .into_os_string()
                .into_string()
                .map_err(|e| format!("OSString error: {}", e.display()))?)
        }
        _ => Ok(notification.icon),
    }
}

pub fn parse_notitication_timestamp(timestamp: i64) -> Result<String, HandlerError> {
    let output = Command::new("awk")
        .args(["{print int($1)}", "/proc/uptime"])
        .output()
        .map_err(|e| HandlerError::IoError(format!("Failed to read /proc/uptime, error: {e:?}")))?;
    let current_timestamp: String = String::from_utf8_lossy(&output.stdout).into();
    let current_timestamp = current_timestamp.trim().parse::<i64>().map_err(|e| {
        HandlerError::ConversionError(format!("Failed to convert String to i64, error: {e:?}"))
    })?;

    let timestamp =
        current_timestamp
            .checked_sub(timestamp)
            .ok_or(HandlerError::OperationError(String::from(
                "Failed to subtract i64",
            )))?;

    let suffix: (i64, &str) = match timestamp {
        0..60 => (1, "s"),
        60..3_600 => (60, "min"),
        3_600..86_400 => (3_600, "h"),
        86_400..2_592_000 => (86_400, "d"),
        _ => (2_592_000, "mo"), // practically unreachable (really)
    };

    let time_ago = timestamp
        .checked_div(suffix.0)
        .ok_or(HandlerError::OperationError(String::from(
            "Failed to divide i64",
        )))?;

    Ok(format!("{}{} ago", time_ago, suffix.1))
}
