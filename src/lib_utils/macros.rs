#[macro_export]
macro_rules! clone_as {
    ( #[$lvl:ident], [$($var:ident),+], $($blk:tt)+ ) => {
        glib::clone!(
            $(#[$lvl] $var),+ , $($blk)+
        )
    };
}
