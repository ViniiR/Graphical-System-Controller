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
