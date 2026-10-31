use gtk::{
    glib::{
        self, g_warning,
        object::{CastNone, IsA},
        Object,
    },
    Widget,
};

#[allow(dead_code)]
#[derive(Debug, Clone)]
pub enum HandlerError {
    DBusError(String),
    ObjectError(String),
}

impl From<glib::Error> for HandlerError {
    fn from(value: glib::Error) -> Self {
        Self::DBusError(value.message().to_owned())
    }
}

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
            .ok_or(HandlerError::ObjectError(format!("Failed to get {name}")))
    }
}

pub trait WidgetDowncastExt {
    fn downcast_or_err<T: IsA<Widget>>(self, msg: &str) -> Result<T, HandlerError>;
}

impl WidgetDowncastExt for Option<Widget> {
    fn downcast_or_err<T: IsA<Widget>>(self, msg: &str) -> Result<T, HandlerError> {
        self.and_downcast::<T>()
            .ok_or(HandlerError::ObjectError(msg.to_owned()))
    }
}

#[derive(Default)]
pub struct State {
    pub volume: u32,
    pub is_muted: bool,
}
impl State {
    pub fn set_volume(&mut self, value: u32) {
        self.volume = value;
    }
    pub fn set_muted(&mut self, value: bool) {
        self.is_muted = value;
    }

    pub fn toggle_muted(&mut self) {
        self.is_muted = !self.is_muted;
    }

    pub fn update(&mut self, variant: &glib::Variant) {
        let percentage = variant.child_value(0).get::<u32>();
        let is_muted = variant.child_value(1).get::<bool>();

        match (percentage, is_muted) {
            (Some(percentage), Some(is_muted)) => {
                self.set_volume(percentage);
                self.set_muted(is_muted);
            }
            _ => {
                g_warning!(None, "GetAudio callback returned invalid tuple types");
            }
        }
    }
}
