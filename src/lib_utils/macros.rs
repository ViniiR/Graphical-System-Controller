#[macro_export]
macro_rules! clone_as {
    //( #[$lvl:ident], [$($var:ident),+], #[$lvl2:ident], [$($var2:ident),+], $($blk:tt)+ ) => {
    //    glib::clone!(
    //        $(#[$lvl] $var),+ , $(#[$lvl2] $var2),+ , $($blk)+
    //    )
    //};
    ( #[$lvl:ident], [$($var:ident),+], $($blk:tt)+ ) => {
        glib::clone!(
            $(#[$lvl] $var),+ , $($blk)+
        )
    };
}
