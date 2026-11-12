use std::{error::Error, fmt::Display};

use gtk::glib;

#[allow(dead_code)]
#[derive(Debug, Clone)]
pub enum HandlerError {
    Dbus(String),
    Object(String),
    Io(String),
    Conversion(String),
    Operation(String),
}

impl From<glib::Error> for HandlerError {
    fn from(value: glib::Error) -> Self {
        Self::Dbus(value.message().to_owned())
    }
}

impl Display for HandlerError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let val = match self {
            Self::Dbus(v)
            | Self::Object(v)
            | Self::Io(v)
            | Self::Conversion(v)
            | Self::Operation(v) => v,
        };
        write!(f, "{val}")
    }
}

impl Error for HandlerError {}
