use gtk::glib::{object::IsA, Object};

use crate::lib_utils::types::handler_error::HandlerError;

pub trait BuilderObject {
    fn get_object<T>(&self, name: &str) -> Result<T, HandlerError>
    where
        T: IsA<Object>;
}
impl BuilderObject for gtk::Builder {
    fn get_object<T>(&self, name: &str) -> Result<T, HandlerError>
    where
        T: IsA<Object>,
    {
        self.object::<T>(name)
            .ok_or(HandlerError::Object(format!("Failed to get {name}")))
    }
}
