use gtk::{
    glib::{self, g_warning, Variant},
    prelude::WidgetExt,
    Button,
};

pub fn update_button_active(btn: &Button, to_active: bool) {
    const ACTIVE: &str = "button-active";
    const INACTIVE: &str = "button-inactive";

    btn.add_css_class(if to_active { ACTIVE } else { INACTIVE });
    btn.remove_css_class(if to_active { INACTIVE } else { ACTIVE });
}

// TODO:
// MISSING ICONS
// power modes (optional)

//https://gitlab.gnome.org/GNOME/adwaita-icon-theme/-/blob/master/Adwaita/symbolic/status/battery-level-0-symbolic.svg
//https://developer.gnome.org/documentation/tutorials/themed-icons.html
pub fn get_volume_icon(percentage: u32, is_muted: bool) -> String {
    if is_muted {
        return String::from("audio-volume-muted-symbolic");
    }

    match percentage {
        0 => String::from("audio-volume-zero-symbolic"),
        1..=40 => String::from("audio-volume-low-symbolic"),
        41..=60 => String::from("audio-volume-medium-symbolic"),
        _ => String::from("audio-volume-high-symbolic"),
    }
}

pub fn get_brightness_icon(_percentage: u32) -> String {
    String::from("display-brightness-symbolic")
    //match percentage {
    //    0..=40 => String::from("display-brightness-low-symbolic"),
    //    41..=60 => String::from("display-brightness-medium-symbolic"),
    //    _ => String::from("display-brightness-high-symbolic"),
    //}
}

pub fn get_individual_audio_icon(name: &str) -> String {
    if name.contains("YouTube") {
        String::from("firefox-logo")
    } else if name.contains("MPD Pulse Audio Output") {
        String::from("audio-headset-symbolic")
    } else {
        String::from("audio-card-symbolic")
    }
}

pub async fn button_click_handler(
    dbus_call: impl Future<Output = Result<Variant, glib::Error>>,
    button: Button,
) {
    match dbus_call.await {
        Ok(_v) if let Some(b) = _v.child_value(0).get::<bool>() => update_button_active(&button, b),
        e => g_warning!(None, "DBus call error: {e:?}"),
    };
}
