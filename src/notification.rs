use gtk::{
    gio::{DBusConnection, ListStore},
    glib::{
        self, g_warning,
        object::{Cast, CastNone, IsA},
        BoxedAnyObject,
    },
    prelude::{GridExt, ListItemExt, WidgetExt},
    Builder, Grid, Image, Label, ListItem, ListView, NoSelection, Picture, SignalListItemFactory,
    Widget,
};

use crate::{
    clone_as, dbus,
    lib_utils::{
        types::{
            builder_object::BuilderObject, handler_error::HandlerError,
            variant_array::VariantArray, widget_downcast_ext::WidgetDowncastExt,
        },
        ui,
    },
    Program,
};

pub fn handle_notification(builder: &Builder, conn: &DBusConnection) -> Result<(), HandlerError> {
    let list: ListView = builder.get_object("notification-list")?;
    let image_btn: Image = builder.get_object("notification-clear-button")?;

    let store = ListStore::new::<glib::BoxedAnyObject>();

    let selection_model = NoSelection::new(Some(store.clone()));
    list.set_model(Some(&selection_model));

    let factory = SignalListItemFactory::new();
    list.set_factory(Some(&factory));
    factory.connect_setup(move |_, list_item| {
        let Some(list_item) = list_item.downcast_ref::<ListItem>() else {
            g_warning!(None, "ListView ListItem does not exist");
            return;
        };

        let notification_builder = Builder::from_resource(&Program::at_runtime_path(
            Program::FILES.notification_builder,
        ));

        list_item.set_child(create_list_item(&notification_builder).as_ref());
    });
    factory.connect_bind(move |_, list_item| {
        let Some(list_item) = list_item.downcast_ref::<ListItem>() else {
            g_warning!(None, "ListView ListItem does not exist");
            return;
        };
        let Some(item) = list_item.item().and_downcast::<BoxedAnyObject>() else {
            g_warning!(None, "ListView ListItem Data does not exist");
            return;
        };

        let notification = item.borrow::<Notification>().clone();

        if let Err(e) = populate_list_item(&notification, list_item) {
            g_warning!(None, "{e:?}");
        }
    });

    glib::spawn_future_local(clone_as!(
        #[strong],
        [conn, store],
        async move {
            let call = dbus::Notification::get_recent(&conn);
            match call.await {
                Ok(v) => {
                    let children = v.get_array::<Notification, NotificationTuple>();

                    store.remove_all();
                    for item in children {
                        store.append(&glib::BoxedAnyObject::new(item));
                    }
                }
                Err(e) => g_warning!(None, "DBus call error: {e:?}"),
            }
        }
    ));

    image_btn.set_tooltip_text(Some("Clear notifications"));

    let click_controller = gtk::GestureClick::new();
    click_controller.connect_pressed(clone_as!(
        #[strong],
        [conn, store],
        move |_, _, _, _| {
            glib::spawn_future_local(clone_as!(
                #[strong],
                [conn, store],
                async move {
                    if let Err(e) = dbus::Notification::clear_notifications(&conn).await{
                        g_warning!(None, "Dbus call error: {e:?}");
                    }
                    store.remove_all();
                }
            ));
        }
    ));
    image_btn.add_controller(click_controller);

    Ok(())
}

#[derive(Debug, Clone)]
pub struct Notification {
    pub body: String,
    pub app: String,
    pub icon: String,
    pub summary: String,
    pub timestamp: i64,
}
pub type NotificationTuple = (String, String, String, String, i64);
impl From<Notification> for NotificationTuple {
    fn from(value: Notification) -> Self {
        (
            value.body,
            value.app,
            value.icon,
            value.summary,
            value.timestamp,
        )
    }
}
impl From<NotificationTuple> for Notification {
    fn from(value: NotificationTuple) -> Self {
        Self {
            body: value.0,
            app: value.1,
            icon: value.2,
            summary: value.3,
            timestamp: value.4,
        }
    }
}

fn create_list_item(builder: &Builder) -> Option<impl IsA<Widget>> {
    let Some(ret) = builder.object::<Grid>("main-grid") else {
        g_warning!(None, "Failed to get notification main-grid");
        return None;
    };

    Some(ret)
}

fn populate_list_item(notif: &Notification, list_item: &ListItem) -> Result<(), HandlerError> {
    let grid = list_item
        .child()
        .downcast_or_err::<Grid>("Failed to get main-grid on notification.ui")?;

    let icon = grid
        .child_at(0, 0)
        .downcast_or_err::<Picture>("Failed to get image on notification.ui")?;
    let icon_name = ui::parse_notification_icon(notif.clone())
        .map_err(|e| HandlerError::IoError(format!("{e:?}")))?;
    icon.set_filename(Some(icon_name));

    let title = grid
        .child_at(1, 0)
        .downcast_or_err::<Label>("Failed to get title on notification.ui")?;
    title.set_label(&notif.summary);

    let timestamp = grid
        .child_at(3, 0)
        .downcast_or_err::<Label>("Failed to get timestamp on notification.ui")?;
    timestamp.set_label(&ui::parse_notitication_timestamp(notif.timestamp)?);

    let label = grid
        .child_at(1, 1)
        .downcast_or_err::<Label>("Failed to get label on notification.ui")?;
    label.set_label(&notif.body);

    Ok(())
}
