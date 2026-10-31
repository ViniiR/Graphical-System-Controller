use gtk::glib::{self, g_warning};

pub mod builder_object;
pub mod handler_error;
pub mod variant_array;
pub mod widget_downcast_ext;

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
