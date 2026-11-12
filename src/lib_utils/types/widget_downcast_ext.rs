use gtk::{
    glib::object::{CastNone, IsA},
    Widget,
};

use crate::lib_utils::types::handler_error::HandlerError;

pub trait WidgetDowncastExt {
    fn downcast_or_err<T: IsA<Widget>>(self, msg: &str) -> Result<T, HandlerError>;
}

impl WidgetDowncastExt for Option<Widget> {
    fn downcast_or_err<T: IsA<Widget>>(self, msg: &str) -> Result<T, HandlerError> {
        self.and_downcast::<T>()
            .ok_or(HandlerError::Object(msg.to_owned()))
    }
}
