use gtk::gio::DBusConnection;
use gtk::gio::{DBusCallFlags, ListStore};
use gtk::glib::object::{Cast, CastNone, IsA};
use gtk::glib::{g_warning, BoxedAnyObject, Variant, VariantTy};
use gtk::prelude::{GridExt, ListItemExt, RangeExt, WidgetExt};
use gtk::{
    glib, Builder, Expander, Grid, Image, Label, ListItem, ListView, NoSelection, Scale,
    SignalListItemFactory, Widget,
};

use crate::types::{dbus, AudioStream, AudioStreamTuple, HandlerError, Program};
use crate::{ui, Filepaths};

pub fn handle_audio_individual(
    builder: &Builder,
    conn: &DBusConnection,
) -> Result<(), HandlerError> {
    let expander: Expander =
        builder
            .object("individual-audio-expander")
            .ok_or(HandlerError::ObjectError(
                "Failed to get individual-audio-expander".to_string(),
            ))?;
    let list: ListView =
        builder
            .object("individual-audio-list")
            .ok_or(HandlerError::ObjectError(
                "Failed to get individual-audio-list".to_string(),
            ))?;

    let store = ListStore::new::<glib::BoxedAnyObject>();

    let selection_model = NoSelection::new(Some(store.clone()));
    list.set_model(Some(&selection_model));

    let factory = SignalListItemFactory::new();
    list.set_factory(Some(&factory));
    factory.connect_setup(|_, list_item| {
        let Some(list_item) = list_item.downcast_ref::<ListItem>() else {
            g_warning!(None, "ListView ListItem does not exist");
            return;
        };

        let individual_builder = Builder::from_resource(Filepaths::INDIVIDUAL_AUDIO_BUILDER);

        list_item.set_child(create_list_item(&individual_builder).as_ref());
    });
    factory.connect_bind(|_, list_item| {
        let Some(list_item) = list_item.downcast_ref::<ListItem>() else {
            g_warning!(None, "ListView ListItem does not exist");
            return;
        };
        let Some(item) = list_item.item().and_downcast::<BoxedAnyObject>() else {
            g_warning!(None, "ListView ListItem Data does not exist");
            return;
        };

        let stream = item.borrow::<AudioStream>().clone();

        if let Err(e) = populate_list_item(&stream, list_item) {
            g_warning!(None, "{e:?}");
        }
    });

    // TODO: handle input: mute, volume

    expander.connect_activate(glib::clone!(
        #[strong]
        conn,
        #[strong]
        store,
        move |_expander| {
            glib::spawn_future_local(glib::clone!(
                #[strong]
                conn,
                #[strong]
                store,
                async move {
                    let call = conn.call_future(
                        Some(Program::BACKEND_NAME),
                        dbus::Controllers::AUDIO,
                        &dbus::Controllers::to_interface(dbus::Controllers::AUDIO),
                        dbus::Methods::GET_ALL_AUDIO_INDIVIDUAL,
                        None,
                        Some(VariantTy::TUPLE),
                        DBusCallFlags::NONE,
                        dbus::Timeout::NONE,
                    );
                    match call.await {
                        Ok(v) => {
                            let children = get_stream_array(&v);

                            store.remove_all();
                            for item in children {
                                store.append(&glib::BoxedAnyObject::new(item));
                            }
                        }
                        Err(e) => g_warning!(None, "DBus call error: {e:?}"),
                    }
                }
            ));
        }
    ));

    Ok(())
}

/// Variant is expected to be (a(usub))
fn get_stream_array(variant: &Variant) -> Vec<AudioStream> {
    if let Some((array,)) = variant.get::<(Vec<AudioStreamTuple>,)>() {
        array
            .into_iter()
            .map(|tuple: AudioStreamTuple| tuple.into())
            .collect()
    } else {
        vec![]
    }
}

fn create_list_item(builder: &Builder) -> Option<impl IsA<Widget>> {
    let Some(ret) = builder.object::<Grid>("main-grid") else {
        g_warning!(None, "Failed to get individual_audio main-grid");
        return None;
    };

    Some(ret)
}

fn populate_list_item(stream: &AudioStream, list_item: &ListItem) -> Result<(), HandlerError> {
    let grid = list_item
        .child()
        .downcast_or_err::<Grid>("Failed to get main-grid on individual_audio.ui")?;

    let icon = grid
        .child_at(0, 0)
        .downcast_or_err::<Image>("Failed to get image on individual_audio.ui")?;
    icon.set_icon_name(Some(&ui::get_individual_audio_icon(&stream.name)));
    let label = grid
        .child_at(1, 0)
        .downcast_or_err::<Label>("Failed to get label on individual_audio.ui")?;
    label.set_label(&stream.name);

    let r#box = grid
        .child_at(0, 1)
        .downcast_or_err::<gtk::Box>("Failed to get box on individual_audio.ui")?;
    let icon = r#box
        .first_child()
        .downcast_or_err::<Image>("Failed to get box image on individual_audio.ui")?;
    icon.set_icon_name(Some(&ui::get_volume_icon(stream.volume, stream.is_muted)));
    let label = r#box
        .last_child()
        .downcast_or_err::<Label>("Failed to get box label on individual_audio.ui")?;
    label.set_label(&format!("{}", stream.volume));
    let scale = grid
        .child_at(1, 1)
        .downcast_or_err::<Scale>("Failed to get scale on individual_audio.ui")?;
    scale.set_value(stream.volume as f64);

    Ok(())
}

trait WidgetDowncastExt {
    fn downcast_or_err<T: IsA<Widget>>(self, msg: &str) -> Result<T, HandlerError>;
}

impl WidgetDowncastExt for Option<Widget> {
    fn downcast_or_err<T: IsA<Widget>>(self, msg: &str) -> Result<T, HandlerError> {
        self.and_downcast::<T>()
            .ok_or(HandlerError::ObjectError(msg.to_owned()))
    }
}
