use gtk::glib::{variant::FromVariant, Variant};

pub trait VariantArray {
    fn get_array<T, TupleT>(&self) -> Vec<T>
    where
        T: From<TupleT>,
        TupleT: From<T> + FromVariant;
}
impl VariantArray for Variant {
    fn get_array<T, TupleT>(&self) -> Vec<T>
    where
        T: From<TupleT>,
        TupleT: From<T> + FromVariant,
    {
        match self.get::<(Vec<TupleT>,)>() {
            Some(v) => v.0.into_iter().map(|tuple: TupleT| tuple.into()).collect(),
            None => vec![],
        }
    }
}
