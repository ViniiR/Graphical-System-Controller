pub trait LossyU32Cast {
    fn to_lossy_u32(self) -> u32;
}

impl LossyU32Cast for f64 {
    #[allow(
        clippy::as_conversions,
        clippy::cast_possible_truncation,
        clippy::cast_sign_loss
    )]
    fn to_lossy_u32(self) -> u32 {
        if self <= 0.0 {
            0
        } else if self >= Self::from(u32::MAX) {
            u32::MAX
        } else {
            self.round() as u32
        }
    }
}
