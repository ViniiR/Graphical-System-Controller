use gtk::{
    gio::{DBusCallFlags, DBusConnection},
    glib::{self, variant::ToVariant, Variant, VariantTy},
};
use macros::BuildCall;

use crate::types::Program;

pub type DbusCallFuture = std::pin::Pin<Box<dyn Future<Output = Result<Variant, glib::Error>>>>;

trait ToInterface {
    fn to_interface(&self) -> String;
}
impl ToInterface for str {
    fn to_interface(&self) -> String {
        self.chars().skip(1).collect::<String>().replace('/', ".")
    }
}

trait BuildCall {
    const NAME: &str;

    fn build(
        conn: &DBusConnection,
        method: &str,
        params: Option<&Variant>,
        reply: Option<&VariantTy>,
    ) -> DbusCallFuture {
        conn.call_future(
            Some(Program::BACKEND_NAME),
            &format!("{}/{}", Program::PATH, Self::NAME),
            &format!("{}/{}", Program::PATH, Self::NAME).to_interface(),
            method,
            params,
            reply,
            DBusCallFlags::NONE,
            Timeout::NONE,
        )
    }
}

#[derive(BuildCall)]
pub struct Brightness;
impl Brightness {
    pub const SET: &str = "SetBrightness";
    pub const GET: &str = "GetBrightness";

    pub fn get(conn: &DBusConnection) -> DbusCallFuture {
        Self::build(conn, Self::GET, None, Some(VariantTy::TUPLE))
    }
    pub fn set(conn: &DBusConnection, value: u32) -> DbusCallFuture {
        Self::build(conn, Self::SET, Some(&(value,).to_variant()), None)
    }
}

#[derive(BuildCall)]
pub struct Power;
impl Power {
    pub const SHUTDOWN: &str = "Shutdown";
    pub const REBOOT: &str = "Reboot";
    pub const LOGOUT: &str = "Logout";

    pub fn shutdown(conn: &DBusConnection) -> DbusCallFuture {
        Self::build(conn, Self::SHUTDOWN, None, None)
    }
    pub fn reboot(conn: &DBusConnection) -> DbusCallFuture {
        Self::build(conn, Self::REBOOT, None, None)
    }
    pub fn logout(conn: &DBusConnection) -> DbusCallFuture {
        Self::build(conn, Self::LOGOUT, None, None)
    }
}

#[derive(BuildCall)]
pub struct Battery;
impl Battery {
    pub const GET: &str = "GetBattery";

    pub fn get(conn: &DBusConnection) -> DbusCallFuture {
        Self::build(conn, Self::GET, None, Some(VariantTy::TUPLE))
    }
}

#[derive(BuildCall)]
pub struct Boost;
impl Boost {
    pub const GET: &str = "GetBoost";
    pub const TOGGLE: &str = "ToggleBoost";

    pub fn get(conn: &DBusConnection) -> DbusCallFuture {
        Self::build(conn, Self::GET, None, Some(VariantTy::TUPLE))
    }
    pub fn toggle(conn: &DBusConnection) -> DbusCallFuture {
        Self::build(conn, Self::TOGGLE, None, Some(VariantTy::TUPLE))
    }
}

#[derive(BuildCall)]
pub struct Conservation;
impl Conservation {
    pub const GET: &str = "GetConservation";
    pub const TOGGLE: &str = "ToggleConservation";

    pub fn get(conn: &DBusConnection) -> DbusCallFuture {
        Self::build(conn, Self::GET, None, Some(VariantTy::TUPLE))
    }
    pub fn toggle(conn: &DBusConnection) -> DbusCallFuture {
        Self::build(conn, Self::TOGGLE, None, Some(VariantTy::TUPLE))
    }
}

#[derive(BuildCall)]
pub struct Audio;
impl Audio {
    pub const GET: &str = "GetAudio";
    pub const SET: &str = "SetAudio";
    pub const TOGGLE_MUTED: &str = "ToggleAudioMuted";

    pub fn get(conn: &DBusConnection) -> DbusCallFuture {
        Self::build(conn, Self::GET, None, Some(VariantTy::TUPLE))
    }
    pub fn set(conn: &DBusConnection, value: u32) -> DbusCallFuture {
        Self::build(
            conn,
            Self::SET,
            Some(&(value,).to_variant()),
            Some(VariantTy::TUPLE),
        )
    }
    pub fn toggle_muted(conn: &DBusConnection) -> DbusCallFuture {
        Self::build(conn, Self::TOGGLE_MUTED, None, None)
    }

    pub const GET_ALL_INDIVIDUAL: &str = "GetAllAudioIndividual";
    pub const SET_INDIVIDUAL: &str = "SetAudioIndividual";
    pub const TOGGLE_MUTED_INDIVIDUAL: &str = "ToggleAudioMutedIndividual";

    pub fn get_all_individual(conn: &DBusConnection) -> DbusCallFuture {
        Self::build(conn, Self::GET_ALL_INDIVIDUAL, None, Some(VariantTy::TUPLE))
    }
    // TODO: implement this functionality
    //pub fn set_individual(conn: &DBusConnection, value: u32) -> DbusCallFuture {
    //    Self::build(
    //        conn,
    //        Self::SET_INDIVIDUAL,
    //        Some(&(value,).to_variant()),
    //        Some(VariantTy::TUPLE),
    //    )
    //}
    //pub fn toggle_muted_individual(conn: &DBusConnection) -> DbusCallFuture {
    //    Self::build(conn, Self::TOGGLE_MUTED_INDIVIDUAL, None, None)
    //}
}

#[derive(BuildCall)]
pub struct Keyboard;
impl Keyboard {
    pub const GET: &str = "GetKeyboard";
    pub const TOGGLE: &str = "ToggleKeyboard";

    pub fn get(conn: &DBusConnection) -> DbusCallFuture {
        Self::build(conn, Self::GET, None, Some(VariantTy::TUPLE))
    }
    pub fn toggle(conn: &DBusConnection) -> DbusCallFuture {
        Self::build(conn, Self::TOGGLE, None, None)
    }
}

pub struct Timeout;
impl Timeout {
    pub const NONE: i32 = -1;
}
