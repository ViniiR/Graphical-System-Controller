use std::cell::RefCell;
use std::rc::Rc;

use gtk::gdk::{Display, Key};
use gtk::gio;
use gtk::glib::{self, g_critical};
use gtk::{prelude::*, CssProvider};
use gtk::{Application, ApplicationWindow, EventControllerFocus, EventControllerKey};

use crate::types::Program;

mod audio;
mod audio_individual;
mod battery;
mod boost;
mod brightness;
mod conservation;
mod power;
mod types;
mod ui;

pub struct Filepaths;
impl Filepaths {
    pub const BUILDER: &str = "src/ui/builder.ui";
    pub const CSS: &str = "src/ui/style.css";
    pub const INDIVIDUAL_AUDIO_BUILDER: &str = "src/ui/individual_audio.ui";
}

// TODO: rewrite everything to have a central logging place
// dont log and return error, return error then log(higher up)
// whenever possible, maybe not in closures
fn main() -> glib::ExitCode {
    let app = Application::builder().application_id(Program::NAME).build();

    gio::resources_register_include!("compiled.gresource").expect("Failed to register resources.");

    app.connect_startup(|_| {
        let Some(display) = Display::default() else {
            g_critical!(None, "Failed to get default display");
            return;
        };

        let provider = CssProvider::new();
        provider.load_from_path(Filepaths::CSS);
        gtk::style_context_add_provider_for_display(
            &display,
            &provider,
            gtk::STYLE_PROVIDER_PRIORITY_APPLICATION,
        );

        let theme = gtk::IconTheme::for_display(&display);
        let resource_path = format!("{}/icons", Program::PATH);
        theme.add_resource_path(&resource_path);

        let settings = gtk::Settings::for_display(&display);
        settings.set_gtk_icon_theme_name(Some("vgsc-icons"));
    });
    app.connect_activate(activate);

    app.run()
}

fn activate(app: &Application) {
    let builder = gtk::Builder::from_file(Filepaths::BUILDER);
    let Some(window) = builder.object::<ApplicationWindow>("main-window") else {
        g_critical!(None, "Failed to get builder main-window");
        return;
    };

    glib::spawn_future_local(async move {
        // TODO: return here might be useless
        // NOTE: getting system bus
        let Ok(dbus_connection) = gio::bus_get_future(gio::BusType::System).await else {
            g_critical!(None, "Failed to connect with DBus");
            return;
        };

        if let Err(e) = brightness::handle_brightness(&builder, &dbus_connection) {
            g_critical!(None, "Brightness error: {e:?}");
            return;
        };

        if let Err(e) = power::handle_power(&builder, &dbus_connection) {
            g_critical!(None, "Power error: {e:?}");
            return;
        };

        if let Err(e) = battery::handle_battery(&builder, &dbus_connection) {
            g_critical!(None, "Battery error: {e:?}");
            return;
        };

        if let Err(e) = boost::handle_boost(&builder, &dbus_connection) {
            g_critical!(None, "Boost error: {e:?}");
            return;
        };

        if let Err(e) = conservation::handle_conservation(&builder, &dbus_connection) {
            g_critical!(None, "Conservation error: {e:?}");
            return;
        };

        if let Err(e) = audio::handle_audio(&builder, &dbus_connection) {
            g_critical!(None, "Audio error: {e:?}");
            return;
        };

        if let Err(e) = audio_individual::handle_audio_individual(&builder, &dbus_connection) {
            g_critical!(None, "Audio Individual error: {e:?}");
            return;
        };
    });

    let enable_focus_close = Rc::new(RefCell::new(true));

    let key_controller = EventControllerKey::new();
    key_controller.connect_key_pressed(glib::clone!(
        #[strong]
        window,
        #[strong]
        enable_focus_close,
        move |_, val, _code, _state| {
            match val {
                Key::Escape => {
                    // Leave focus, focus_controller handles closing
                    GtkWindowExt::set_focus(&window, None::<&gtk::Widget>);
                    glib::Propagation::Stop
                }
                Key::F12 => {
                    enable_focus_close.replace_with(|val| !*val);
                    glib::Propagation::Proceed
                }
                _ => glib::Propagation::Proceed,
            }
        }
    ));
    window.add_controller(key_controller);

    let focus_controller = EventControllerFocus::new();
    focus_controller.connect_leave(glib::clone!(
        #[strong]
        window,
        #[strong]
        enable_focus_close,
        move |_| {
            if *enable_focus_close.borrow() {
                window.close();
            }
        }
    ));
    window.add_controller(focus_controller);

    window.set_application(Some(app));

    window.present();
}
