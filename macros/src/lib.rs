use proc_macro::TokenStream;
use quote::quote;

#[proc_macro_derive(BuildCall)]
pub fn derive_build_call(item: TokenStream) -> TokenStream {
    let input = syn::parse_macro_input!(item as syn::DeriveInput);

    let struct_name = &input.ident;
    let struct_name_str = struct_name.to_string();

    quote! {
        impl BuildCall for #struct_name {
            const NAME: &str = #struct_name_str;
        }
    }
    .into()
}
