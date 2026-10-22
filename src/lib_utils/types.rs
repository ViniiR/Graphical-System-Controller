use gtk::glib::{self, g_warning, object::IsA, Object};

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

#[derive(Debug, Clone)]
pub struct AudioStream {
    pub sink_id: u32,
    pub name: String,
    pub volume: u32,
    pub is_muted: bool,
}
pub type AudioStreamTuple = (u32, String, u32, bool);
impl From<AudioStream> for AudioStreamTuple {
    fn from(value: AudioStream) -> Self {
        (value.sink_id, value.name, value.volume, value.is_muted)
    }
}
impl From<AudioStreamTuple> for AudioStream {
    fn from(value: AudioStreamTuple) -> Self {
        Self {
            sink_id: value.0,
            name: value.1,
            volume: value.2,
            is_muted: value.3,
        }
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
