use std::{cell::RefCell, rc::Rc};

use gtk::{
    gio::DBusConnection,
    glib::{self, g_warning, object::ObjectExt, SignalHandlerId},
    prelude::RangeExt,
    Builder, Image, Label, Scale,
};

use crate::{
    clone_as, dbus,
    lib_utils::{
        transform::LossyU32Cast,
        types::{BuilderObject, HandlerError},
        ui::get_brightness_icon,
    },
};

pub fn handle_brightness(builder: &Builder, conn: &DBusConnection) -> Result<(), HandlerError> {
    let scale: Scale = builder.get_object("brightness-scale")?;
    let label: Label = builder.get_object("brightness-scale-label")?;
    let image: Image = builder.get_object("brightness-scale-label-image")?;

    let pending_timer: Rc<RefCell<Option<glib::SourceId>>> = Rc::new(RefCell::new(None));
    let last_sent_value = Rc::new(std::cell::Cell::new(u32::MAX));

    let signal = scale.connect_value_changed(clone_as!(
        #[strong],
        [label, image, conn],
        move |scale| {
            let value = scale.value().to_lossy_u32();

            update_brightness(value, &label, &image, None, None);

            if value == last_sent_value.get() {
                return;
            }
            if let Some(source_id) = pending_timer.borrow_mut().take() {
                source_id.remove();
            }

            let timer_clone = pending_timer.clone();
            let last_sent_clone = last_sent_value.clone();

            let source_id = glib::timeout_add_local_once(
                std::time::Duration::from_millis(50),
                clone_as!(
                    #[strong],
                    [conn],
                    move || {
                        timer_clone.borrow_mut().take();
                        last_sent_clone.set(value);

                        let res = dbus::Brightness::set(&conn, value);

                        glib::spawn_future_local(async move {
                            let res = res.await;

                            if let Err(e) = &res {
                                g_warning!(None, "DBus call error: {e:?}");
                            }
                        });
                    }
                ),
            );

            *pending_timer.borrow_mut() = Some(source_id);
        }
    ));

    // Get brightness on startup
    glib::spawn_future_local(clone_as!(
        #[strong],
        [conn, label, image, scale],
        async move {
            let call = dbus::Brightness::get(&conn);

            match call.await {
                Ok(r) => {
                    let Some(p) = r.child_value(0).get::<u32>() else {
                        g_warning!(None, "GetBrightness callback returned incorrect value");
                        return;
                    };

                    update_brightness(p, &label, &image, Some(&scale), Some(&signal));
                }
                Err(e) => {
                    g_warning!(None, "DBus call error: {e:?}");
                }
            }
        }
    ));

    Ok(())
}

fn update_brightness(
    percentage: u32,
    label: &Label,
    image: &Image,
    scale: Option<&Scale>,
    signal: Option<&SignalHandlerId>,
) {
    label.set_text(&format!("{percentage}"));
    image.set_icon_name(Some(get_brightness_icon(percentage)));

    if let (Some(s), Some(scl)) = (signal, scale) {
        scl.block_signal(s);
        scl.set_value(percentage.into());
        scl.unblock_signal(s);
    }
}
