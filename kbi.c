/*
Simple keyboard layout indicator for system tray.
Thanks to everyone for their help and code examples.
Andrey Shashanov (2019)
gcc -O2 -s -lX11 `pkg-config --cflags --libs gtk+-3.0 pangocairo` -o kbi kbi.c
*/

#define GDK_VERSION_MIN_REQUIRED (G_ENCODE_VERSION(3, 0))
#define GDK_VERSION_MAX_ALLOWED (G_ENCODE_VERSION(3, 12))

#include <X11/XKBlib.h>
#include <pango/pangocairo.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#define ICON_SIZE 16
#define FONT "Sans 10"
#define COLOR 0xDFDFDF /* 0x2E3436 */
#define X_POS 0
#define Y_POS 1
/* #define CENTER_POS */

typedef struct _applet
{
    Display *dpy;
    GtkStatusIcon *status_icon;
    int xkbEventType;
} applet;

static void activate_cb(applet *data)
{
    XkbDescPtr xkb;
    XkbStateRec state;

    XkbGetState(data->dpy, XkbUseCoreKbd, &state);
    xkb = XkbGetKeyboard(data->dpy, XkbAllComponentsMask, XkbUseCoreKbd);

    if (xkb->names->groups[state.group + 1] != 0)
        XkbLockGroup(data->dpy, XkbUseCoreKbd, state.group + 1U);
    else
        XkbLockGroup(data->dpy, XkbUseCoreKbd, 0);

    XkbFreeKeyboard(xkb, XkbAllComponentsMask, 1);
}

static void status_icon_set(applet *data)
{
    char *name;
    XkbDescPtr xkb;
    XkbStateRec state;
    cairo_surface_t *surface;
    cairo_t *cr;
    PangoLayout *layout;
    PangoFontDescription *desc;
#if defined(CENTER_POS)
    PangoRectangle ink, logical;
#endif
    GdkPixbuf *pixbuf;

    XkbGetState(data->dpy, XkbUseCoreKbd, &state);
    xkb = XkbGetKeyboard(data->dpy, XkbAllComponentsMask, XkbUseCoreKbd);
    name = XGetAtomName(data->dpy, xkb->names->groups[state.group]);

    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                         ICON_SIZE, ICON_SIZE);
    cr = cairo_create(surface);

    layout = pango_cairo_create_layout(cr);
    pango_layout_set_text(layout, name, 2);

    gtk_status_icon_set_tooltip_text(data->status_icon, name);

    XFree(name);
    XkbFreeKeyboard(xkb, XkbAllComponentsMask, 1);

    desc = pango_font_description_from_string(FONT);
    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc);
    cairo_set_source_rgb(cr,
                         (COLOR >> 16) / 255.0,
                         (COLOR >> 8 & ~(0xFF << 8)) / 255.0,
                         (COLOR & ~(0xFFFF << 8)) / 255.0);
#if defined(CENTER_POS)
    pango_layout_get_pixel_extents(layout, &ink, &logical);
    cairo_move_to(cr,
                  (ICON_SIZE - logical.width) / 2,
                  (ICON_SIZE - logical.height) / 2);
#else
    cairo_move_to(cr, X_POS, Y_POS);
#endif
    pango_cairo_show_layout(cr, layout);

    pixbuf = gdk_pixbuf_get_from_surface(surface, 0, 0, ICON_SIZE, ICON_SIZE);

    gtk_status_icon_set_from_pixbuf(data->status_icon, pixbuf);

    g_object_unref(pixbuf);
    g_object_unref(layout);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

static GdkFilterReturn filter(XEvent *xev, GdkEvent *event, applet *data)
{
    (void)event;

    if (xev->type == data->xkbEventType &&
        ((XkbEvent *)xev)->any.xkb_type == XkbStateNotify)
    {
        status_icon_set(data);
        return GDK_FILTER_REMOVE;
    }
    return GDK_FILTER_CONTINUE;
}

int main(int argc, char *argv[])
{
    (void)argc;

    applet data;
    int dummy;

    gtk_init(NULL, NULL);

    if ((data.dpy = gdk_x11_get_default_xdisplay()) == NULL)
    {
        fprintf(stderr, "%s: Error getting default DISPLAY\n", argv[0]);
        return 1;
    }

    if (!XkbQueryExtension(data.dpy, &dummy, &data.xkbEventType,
                           &dummy, &dummy, &dummy))
    {
        fprintf(stderr, "%s: XKB extension is not present\n", argv[0]);
        return 1;
    }

    data.status_icon = gtk_status_icon_new();

    g_signal_connect_swapped(data.status_icon, "activate",
                             G_CALLBACK(activate_cb), &data);

    status_icon_set(&data);

    XkbSelectEventDetails(data.dpy, XkbUseCoreKbd, XkbStateNotify,
                          XkbAllStateComponentsMask, XkbGroupStateMask);

    gdk_window_add_filter(NULL, (GdkFilterFunc)filter, &data);

    gtk_main();

    /* code will never be executed */
    XkbSelectEventDetails(data.dpy, XkbUseCoreKbd, XkbStateNotify,
                          XkbAllStateComponentsMask, 0);
    return 0;
}
