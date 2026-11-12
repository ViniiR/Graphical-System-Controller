use std::{error::Error, fmt::Display};

use gtk::glib;

#[allow(dead_code)]
#[derive(Debug, Clone)]
pub enum HandlerError {
    DBusError(String),
    ObjectError(String),
    IoError(String),
    ConversionError(String),
    OperationError(String),
}

impl From<glib::Error> for HandlerError {
    fn from(value: glib::Error) -> Self {
        Self::DBusError(value.message().to_owned())
    }
}

impl Display for HandlerError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let val = match self {
            Self::DBusError(v)
            | Self::ObjectError(v)
            | Self::IoError(v)
            | Self::ConversionError(v)
            | Self::OperationError(v) => v,
        };
        write!(f, "{val}")
    }
}

impl Error for HandlerError {}
