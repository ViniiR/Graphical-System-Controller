fn main() {
    glib_build_tools::compile_resources(
        &["src/ui"],
        "src/ui/resources.gresource.xml",
        "ui.gresource",
    );
    glib_build_tools::compile_resources(
        &["src/assets"],
        "src/assets/resources.gresource.xml",
        "assets.gresource",
    );
}
