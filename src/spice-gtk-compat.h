#pragma once

/*
 * GTK3/GTK4 compatibility shims.
 *
 * Inline functions and macros that abstract API differences between
 * GTK 3.24 and GTK 4.x so the same source files compile cleanly
 * against either version.  Each numbered section documents the
 * specific API change and the chosen compatibility strategy.
 *
 * Include this header AFTER gtk/gtk.h (or gdk/gdk.h) and config.h.
 */

#include "config.h"
#include <gtk/gtk.h>

/* ------------------------------------------------------------------ */
/* 1. GtkGestureMultiPress → GtkGestureClick                         */
/*                                                                    */
/* In GTK4, GtkGestureMultiPress was renamed to GtkGestureClick.     */
/* The constructor also changed from gtk_gesture_multi_press_new()    */
/* to gtk_gesture_click_new(), and no longer takes a widget argument. */
/* ------------------------------------------------------------------ */
#if GTK_CHECK_VERSION(4, 0, 0)
  typedef GtkGestureClick  SpiceCompat_GestureButton;
  #define SPICE_TYPE_GESTURE_BUTTON  GTK_TYPE_GESTURE_CLICK
#else
  typedef GtkGestureMultiPress  SpiceCompat_GestureButton;
  #define SPICE_TYPE_GESTURE_BUTTON  GTK_TYPE_GESTURE_MULTI_PRESS
#endif

/* ------------------------------------------------------------------ */
/* 2. Event controller constructors                                   */
/*                                                                    */
/* GTK 3.24: constructors take a GtkWidget* and auto-attach.          */
/* GTK 4:    constructors take no arguments; caller must then call     */
/*           gtk_widget_add_controller(widget, controller).           */
/*                                                                    */
/* We provide wrapper macros that always return the controller and    */
/* always leave it attached to the widget.                            */
/* ------------------------------------------------------------------ */
#if GTK_CHECK_VERSION(4, 0, 0)

static inline GtkEventController *
spice_compat_event_controller_key_new(GtkWidget *widget)
{
    GtkEventController *c = gtk_event_controller_key_new();
    gtk_widget_add_controller(widget, c);
    return c;
}

static inline GtkEventController *
spice_compat_event_controller_motion_new(GtkWidget *widget)
{
    GtkEventController *c = gtk_event_controller_motion_new();
    gtk_widget_add_controller(widget, c);
    return c;
}

static inline GtkEventController *
spice_compat_event_controller_scroll_new(GtkWidget *widget,
                                         GtkEventControllerScrollFlags flags)
{
    GtkEventController *c = gtk_event_controller_scroll_new(flags);
    gtk_widget_add_controller(widget, c);
    return c;
}

static inline GtkGesture *
spice_compat_gesture_button_new(GtkWidget *widget)
{
    GtkGesture *g = gtk_gesture_click_new();
    gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(g));
    return g;
}

#else /* GTK 3 */

static inline GtkEventController *
spice_compat_event_controller_key_new(GtkWidget *widget)
{
    return gtk_event_controller_key_new(widget);
}

static inline GtkEventController *
spice_compat_event_controller_motion_new(GtkWidget *widget)
{
    return gtk_event_controller_motion_new(widget);
}

static inline GtkEventController *
spice_compat_event_controller_scroll_new(GtkWidget *widget,
                                         GtkEventControllerScrollFlags flags)
{
    return gtk_event_controller_scroll_new(widget, flags);
}

static inline GtkGesture *
spice_compat_gesture_button_new(GtkWidget *widget)
{
    return GTK_GESTURE(gtk_gesture_multi_press_new(widget));
}

#endif /* GTK version check */

/* ------------------------------------------------------------------ */
/* 3. Focus signals                                                   */
/*                                                                    */
/* GTK 3.24: "focus-in" and "focus-out" are signals on                */
/*           GtkEventControllerKey.                                   */
/* GTK 4:    Focus handling moved to a separate                       */
/*           GtkEventControllerFocus, with "enter" and "leave".       */
/*                                                                    */
/* We provide a helper that connects focus callbacks to the right     */
/* object depending on the GTK version.  The caller passes the key    */
/* controller (which we ignore in GTK4), the widget to attach the     */
/* focus controller to (GTK4 only), and the two callbacks.            */
/*                                                                    */
/* Callback signatures:                                               */
/*   GTK 3: void cb(GtkEventControllerKey *controller, gpointer)     */
/*   GTK 4: void cb(GtkEventControllerFocus *controller, gpointer)   */
/* Both are compatible with GCallback.                                */
/* ------------------------------------------------------------------ */
#if GTK_CHECK_VERSION(4, 0, 0)

static inline void
spice_compat_connect_focus(GtkEventController *key_controller G_GNUC_UNUSED,
                           GtkWidget *widget,
                           GCallback  focus_in_cb,
                           GCallback  focus_out_cb,
                           gpointer   user_data)
{
    GtkEventController *fc = gtk_event_controller_focus_new();
    gtk_widget_add_controller(widget, fc);
    g_signal_connect(fc, "enter",  focus_in_cb,  user_data);
    g_signal_connect(fc, "leave",  focus_out_cb, user_data);
}

#else /* GTK 3 */

static inline void
spice_compat_connect_focus(GtkEventController *key_controller,
                           GtkWidget *widget G_GNUC_UNUSED,
                           GCallback  focus_in_cb,
                           GCallback  focus_out_cb,
                           gpointer   user_data)
{
    g_signal_connect(key_controller, "focus-in",  focus_in_cb,  user_data);
    g_signal_connect(key_controller, "focus-out", focus_out_cb, user_data);
}

#endif /* GTK version check */

/* ------------------------------------------------------------------ */
/* 4. gtk_widget_add_events() — removed in GTK4                      */
/*                                                                    */
/* In GTK4 all events are delivered via controllers; there is no      */
/* event mask.  Make it a no-op.                                      */
/* ------------------------------------------------------------------ */
#if GTK_CHECK_VERSION(4, 0, 0)
  #define spice_compat_widget_add_events(widget, mask)  ((void)0)
#else
  #define spice_compat_widget_add_events(widget, mask)  \
      gtk_widget_add_events(widget, mask)
#endif

/* ------------------------------------------------------------------ */
/* 5. gtk_widget_show_all() — removed in GTK4                        */
/*                                                                    */
/* In GTK4 widgets are visible by default; show_all is unnecessary.  */
/* ------------------------------------------------------------------ */
#if GTK_CHECK_VERSION(4, 0, 0)
  #define spice_compat_widget_show_all(widget)  ((void)0)
  #define spice_compat_widget_show(widget)      ((void)0)
#else
  #define spice_compat_widget_show_all(widget)  gtk_widget_show_all(widget)
  #define spice_compat_widget_show(widget)      gtk_widget_show(widget)
#endif

/* ------------------------------------------------------------------ */
/* 6. gtk_box_pack_start() with fill/expand/padding → gtk_box_append */
/*                                                                    */
/* GTK 3: gtk_box_pack_start(box, child, expand, fill, padding)      */
/* GTK 4: gtk_box_append(box, child)                                 */
/*   (expansion is controlled via child hexpand/vexpand properties)  */
/* ------------------------------------------------------------------ */
#if GTK_CHECK_VERSION(4, 0, 0)

static inline void
spice_compat_box_pack_start(GtkBox *box, GtkWidget *child,
                            gboolean expand, gboolean fill G_GNUC_UNUSED,
                            guint padding G_GNUC_UNUSED)
{
    if (expand) {
        gtk_widget_set_hexpand(child, TRUE);
        gtk_widget_set_vexpand(child, TRUE);
    }
    gtk_box_append(box, child);
}

#else /* GTK 3 */

static inline void
spice_compat_box_pack_start(GtkBox *box, GtkWidget *child,
                            gboolean expand, gboolean fill,
                            guint padding)
{
    gtk_box_pack_start(box, child, expand, fill, padding);
}

#endif /* GTK version check */

/* ------------------------------------------------------------------ */
/* 6b. gtk_box_pack_end() with fill/expand/padding → gtk_box_append  */
/*                                                                    */
/* GTK 3: gtk_box_pack_end(box, child, expand, fill, padding)        */
/* GTK 4: gtk_box_append(box, child) — pack_end distinction gone     */
/* ------------------------------------------------------------------ */
#if GTK_CHECK_VERSION(4, 0, 0)

static inline void
spice_compat_box_pack_end(GtkBox *box, GtkWidget *child,
                          gboolean expand, gboolean fill G_GNUC_UNUSED,
                          guint padding G_GNUC_UNUSED)
{
    if (expand) {
        gtk_widget_set_hexpand(child, TRUE);
        gtk_widget_set_vexpand(child, TRUE);
    }
    gtk_box_append(box, child);
}

#else /* GTK 3 */

static inline void
spice_compat_box_pack_end(GtkBox *box, GtkWidget *child,
                          gboolean expand, gboolean fill,
                          guint padding)
{
    gtk_box_pack_end(box, child, expand, fill, padding);
}

#endif /* GTK version check */

/* ------------------------------------------------------------------ */
/* 7. GdkPoint — removed in GTK4                                     */
/*                                                                    */
/* GdkPoint was a simple struct { gint x; gint y; }.  GTK4 dropped   */
/* it.  We provide a typedef under a compat name so the struct field  */
/* in SpiceDisplayPrivate keeps working.                              */
/* ------------------------------------------------------------------ */
#if GTK_CHECK_VERSION(4, 0, 0)
typedef struct { gint x; gint y; } SpiceCompatPoint;
#else
typedef GdkPoint SpiceCompatPoint;
#endif

/* ------------------------------------------------------------------ */
/* 8. GdkWindow → GdkSurface type and cast                           */
/*                                                                    */
/* In GTK4, GdkWindow was renamed to GdkSurface.                     */
/* We provide a typedef and cast macro so the same source code        */
/* compiles under both GTK versions.                                  */
/* ------------------------------------------------------------------ */
#if GTK_CHECK_VERSION(4, 0, 0)
  typedef GdkSurface  SpiceCompatSurface;
  #define SPICE_COMPAT_GDK_SURFACE(obj)  GDK_SURFACE(obj)
#else
  typedef GdkWindow   SpiceCompatSurface;
  #define SPICE_COMPAT_GDK_SURFACE(obj)  GDK_WINDOW(obj)
#endif

/* ------------------------------------------------------------------ */
/* 9. gtk_widget_get_window() → gtk_native_get_surface()              */
/*                                                                    */
/* GTK 3: gtk_widget_get_window(widget) returns GdkWindow*.           */
/* GTK 4: widgets don't own their GdkSurface directly. The nearest   */
/*         GtkNative ancestor holds it.                               */
/* ------------------------------------------------------------------ */
static inline SpiceCompatSurface *
spice_compat_widget_get_surface(GtkWidget *widget)
{
#if GTK_CHECK_VERSION(4, 0, 0)
    GtkNative *native = gtk_widget_get_native(widget);
    if (native == NULL)
        return NULL;
    return gtk_native_get_surface(native);
#else
    return gtk_widget_get_window(widget);
#endif
}

/* ------------------------------------------------------------------ */
/* 10. gdk_window_get_display() → gdk_surface_get_display()          */
/* ------------------------------------------------------------------ */
static inline GdkDisplay *
spice_compat_surface_get_display(SpiceCompatSurface *surface)
{
#if GTK_CHECK_VERSION(4, 0, 0)
    return gdk_surface_get_display(surface);
#else
    return gdk_window_get_display(surface);
#endif
}

/* ------------------------------------------------------------------ */
/* 11. gdk_window_get_width/height() → gdk_surface_get_width/height()*/
/* ------------------------------------------------------------------ */
static inline int
spice_compat_surface_get_width(SpiceCompatSurface *surface)
{
#if GTK_CHECK_VERSION(4, 0, 0)
    return gdk_surface_get_width(surface);
#else
    return gdk_window_get_width(surface);
#endif
}

static inline int
spice_compat_surface_get_height(SpiceCompatSurface *surface)
{
#if GTK_CHECK_VERSION(4, 0, 0)
    return gdk_surface_get_height(surface);
#else
    return gdk_window_get_height(surface);
#endif
}

/* ------------------------------------------------------------------ */
/* 12. gdk_window_set_cursor() / get_cursor() → widget-based API     */
/*                                                                    */
/* GTK 3: cursor is set on GdkWindow.                                */
/* GTK 4: cursor is set on GtkWidget (per-widget cursors).           */
/*                                                                    */
/* We provide widget-based wrappers that take the widget.            */
/* ------------------------------------------------------------------ */
static inline void
spice_compat_widget_set_cursor(GtkWidget *widget,
                               SpiceCompatSurface *surface G_GNUC_UNUSED,
                               GdkCursor *cursor)
{
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_widget_set_cursor(widget, cursor);
#else
    (void)widget;
    gdk_window_set_cursor(surface, cursor);
#endif
}

static inline GdkCursor *
spice_compat_widget_get_cursor(GtkWidget *widget,
                               SpiceCompatSurface *surface G_GNUC_UNUSED)
{
#if GTK_CHECK_VERSION(4, 0, 0)
    return gtk_widget_get_cursor(widget);
#else
    (void)widget;
    return gdk_window_get_cursor(surface);
#endif
}

/* ------------------------------------------------------------------ */
/* 13. gdk_window_ensure_native() → no-op in GTK4                    */
/*                                                                    */
/* In GTK4 all surfaces are native; this call is not needed.          */
/* Returns TRUE always in GTK4 for compatibility.                     */
/* ------------------------------------------------------------------ */
static inline gboolean
spice_compat_surface_ensure_native(SpiceCompatSurface *surface G_GNUC_UNUSED)
{
#if GTK_CHECK_VERSION(4, 0, 0)
    return TRUE;
#else
    return gdk_window_ensure_native(surface);
#endif
}

/* ------------------------------------------------------------------ */
/* 14. gdk_display_get_monitor_at_window() →                         */
/*     gdk_display_get_monitor_at_surface()                           */
/* ------------------------------------------------------------------ */
static inline GdkMonitor *
spice_compat_display_get_monitor_at_surface(GdkDisplay *display,
                                            SpiceCompatSurface *surface)
{
#if GTK_CHECK_VERSION(4, 0, 0)
    return gdk_display_get_monitor_at_surface(display, surface);
#else
    return gdk_display_get_monitor_at_window(display, surface);
#endif
}

/* ------------------------------------------------------------------ */
/* 15. gdk_window_get_device_position() →                            */
/*     gdk_surface_get_device_position()                              */
/*                                                                    */
/* Note: GTK4 version returns double* for x/y instead of int*.      */
/* We provide an int-based wrapper for GTK3 compatibility.           */
/* ------------------------------------------------------------------ */
static inline GdkModifierType
spice_compat_surface_get_device_position(SpiceCompatSurface *surface,
                                         GdkDevice *device)
{
    GdkModifierType mask = 0;
#if GTK_CHECK_VERSION(4, 0, 0)
    double x, y;
    gdk_surface_get_device_position(surface, device, &x, &y, &mask);
#else
    gdk_window_get_device_position(surface, device, NULL, NULL, &mask);
#endif
    return mask;
}

/* ------------------------------------------------------------------ */
/* 16. X11 backend surface macros                                     */
/*                                                                    */
/* GDK_WINDOW_XID → GDK_SURFACE_XID (via gdk_x11_surface_get_xid)   */
/* GDK_WINDOW_XDISPLAY → GDK_SURFACE_XDISPLAY                       */
/* GDK_IS_X11_WINDOW → GDK_IS_X11_SURFACE                           */
/* ------------------------------------------------------------------ */
#ifdef GDK_WINDOWING_X11
  #if GTK_CHECK_VERSION(4, 0, 0)
    #define SPICE_COMPAT_SURFACE_XID(s)       GDK_SURFACE_XID(s)
    #define SPICE_COMPAT_SURFACE_XDISPLAY(s)  GDK_SURFACE_XDISPLAY(s)
    #define SPICE_COMPAT_IS_X11_SURFACE(s)    GDK_IS_X11_SURFACE(s)
  #else
    #define SPICE_COMPAT_SURFACE_XID(s)       GDK_WINDOW_XID(s)
    #define SPICE_COMPAT_SURFACE_XDISPLAY(s)  GDK_WINDOW_XDISPLAY(s)
    #define SPICE_COMPAT_IS_X11_SURFACE(s)    GDK_IS_X11_WINDOW(s)
  #endif
#endif

/* ------------------------------------------------------------------ */
/* 17. Wayland backend surface function                               */
/*                                                                    */
/* gdk_wayland_window_get_wl_surface() →                             */
/* gdk_wayland_surface_get_wl_surface()                               */
/* GDK_IS_WAYLAND_WINDOW → GDK_IS_WAYLAND_SURFACE                   */
/* ------------------------------------------------------------------ */
#ifdef GDK_WINDOWING_WAYLAND
  #if GTK_CHECK_VERSION(4, 0, 0)
    #define spice_compat_wayland_surface_get_wl_surface(s) \
        gdk_wayland_surface_get_wl_surface(s)
    #define SPICE_COMPAT_IS_WAYLAND_SURFACE(s)  GDK_IS_WAYLAND_SURFACE(s)
  #else
    #define spice_compat_wayland_surface_get_wl_surface(s) \
        gdk_wayland_window_get_wl_surface(s)
    #define SPICE_COMPAT_IS_WAYLAND_SURFACE(s)  GDK_IS_WAYLAND_WINDOW(s)
  #endif
#endif

/* ------------------------------------------------------------------ */
/* 18. Win32 backend surface macros                                   */
/*                                                                    */
/* GDK_IS_WIN32_WINDOW → GDK_IS_WIN32_SURFACE                       */
/* gdk_win32_window_get_impl_hwnd → gdk_win32_surface_get_impl_hwnd  */
/* ------------------------------------------------------------------ */
#ifdef GDK_WINDOWING_WIN32
  #if GTK_CHECK_VERSION(4, 0, 0)
    #define SPICE_COMPAT_IS_WIN32_SURFACE(s)  GDK_IS_WIN32_SURFACE(s)
    #define spice_compat_win32_get_hwnd(s)    gdk_win32_surface_get_impl_hwnd(s)
  #else
    #define SPICE_COMPAT_IS_WIN32_SURFACE(s)  GDK_IS_WIN32_WINDOW(s)
    #define spice_compat_win32_get_hwnd(s)    gdk_win32_window_get_impl_hwnd(s)
  #endif
#endif

/* ------------------------------------------------------------------ */
/* 19. macOS/Quartz backend surface macros                            */
/*                                                                    */
/* GDK_IS_QUARTZ_WINDOW → GDK_IS_MACOS_SURFACE (renamed in GTK4)    */
/* ------------------------------------------------------------------ */
#ifdef GDK_WINDOWING_QUARTZ
  #define SPICE_COMPAT_IS_MACOS_SURFACE(s)  GDK_IS_QUARTZ_WINDOW(s)
#elif defined(GDK_WINDOWING_MACOS)
  #define SPICE_COMPAT_IS_MACOS_SURFACE(s)  GDK_IS_MACOS_SURFACE(s)
#endif

/* ------------------------------------------------------------------ */
/* 19b. Broadway backend surface macros                               */
/*                                                                    */
/* GDK_IS_BROADWAY_WINDOW → GDK_IS_BROADWAY_SURFACE                  */
/* ------------------------------------------------------------------ */
#ifdef GDK_WINDOWING_BROADWAY
  #if GTK_CHECK_VERSION(4, 0, 0)
    #define SPICE_COMPAT_IS_BROADWAY_SURFACE(s)  GDK_IS_BROADWAY_SURFACE(s)
  #else
    #define SPICE_COMPAT_IS_BROADWAY_SURFACE(s)  GDK_IS_BROADWAY_WINDOW(s)
  #endif
#endif

/* ------------------------------------------------------------------ */
/* 20. gdk_cairo_surface_create_from_pixbuf() window parameter       */
/*                                                                    */
/* GTK 3: gdk_cairo_surface_create_from_pixbuf(pixbuf, scale, window)*/
/*         window is used to determine the actual scale factor.       */
/* GTK 4: gdk_cairo_surface_create_from_pixbuf(pixbuf, scale, NULL)  */
/*         The window parameter was removed; pass NULL.               */
/* Note: This function is NOT removed in GTK4, just the 3rd arg      */
/* meaning changed. We keep the wrapper for clarity.                  */
/* ------------------------------------------------------------------ */
static inline cairo_surface_t *
spice_compat_cairo_surface_from_pixbuf(GdkPixbuf *pixbuf, int scale,
                                       SpiceCompatSurface *surface G_GNUC_UNUSED)
{
#if GTK_CHECK_VERSION(4, 0, 0)
    return gdk_cairo_surface_create_from_pixbuf(pixbuf, scale, NULL);
#else
    return gdk_cairo_surface_create_from_pixbuf(pixbuf, scale, surface);
#endif
}

/* ------------------------------------------------------------------ */
/* 21. gdk_event_free() → gdk_event_unref()                          */
/*                                                                    */
/* GTK 3: gdk_event_free(event)                                      */
/* GTK 4: gdk_event_unref(event) — GdkEvent is now refcounted        */
/* ------------------------------------------------------------------ */
#if GTK_CHECK_VERSION(4, 0, 0)
  #define spice_compat_event_free(event)  gdk_event_unref(event)
#else
  #define spice_compat_event_free(event)  gdk_event_free(event)
#endif

/* ------------------------------------------------------------------ */
/* 22. gtk_get_current_event() → event controller API                 */
/*                                                                    */
/* GTK 3: gtk_get_current_event() returns a copy (must be freed).     */
/* GTK 4: gtk_get_current_event() was removed. Use                    */
/*         gtk_event_controller_get_current_event() instead, which    */
/*         returns a borrowed reference (do NOT free).                 */
/*                                                                    */
/* We provide two approaches:                                         */
/*  a) spice_compat_get_current_event(controller) — returns the       */
/*     event. In GTK3 it's a copy (caller must free), in GTK4 it's   */
/*     borrowed (caller must NOT free).                               */
/*  b) SPICE_COMPAT_CURRENT_EVENT_NEEDS_FREE — TRUE in GTK3, FALSE   */
/*     in GTK4, so callers know whether to free.                      */
/* ------------------------------------------------------------------ */
#if GTK_CHECK_VERSION(4, 0, 0)

static inline GdkEvent *
spice_compat_get_current_event(GtkEventController *controller)
{
    return (GdkEvent *)gtk_event_controller_get_current_event(controller);
}

  #define SPICE_COMPAT_CURRENT_EVENT_NEEDS_FREE  FALSE

#else /* GTK 3 */

static inline GdkEvent *
spice_compat_get_current_event(GtkEventController *controller G_GNUC_UNUSED)
{
    return gtk_get_current_event();
}

  #define SPICE_COMPAT_CURRENT_EVENT_NEEDS_FREE  TRUE

#endif /* GTK version check */

/* ------------------------------------------------------------------ */
/* 23. gtk_get_current_event_state() replacement                      */
/*                                                                    */
/* GTK 3: gtk_get_current_event_state(&state) — global function.     */
/* GTK 4: Removed. Use the event from the controller instead.         */
/*                                                                    */
/* We provide a controller-based wrapper that works on both versions. */
/* ------------------------------------------------------------------ */
static inline GdkModifierType
spice_compat_get_current_event_state(GtkEventController *controller G_GNUC_UNUSED)
{
    GdkModifierType state = 0;
#if GTK_CHECK_VERSION(4, 0, 0)
    GdkEvent *event = gtk_event_controller_get_current_event(controller);
    if (event)
        state = gdk_event_get_modifier_state(event);
#else
    gtk_get_current_event_state(&state);
#endif
    return state;
}

/* ------------------------------------------------------------------ */
/* 24. GdkEvent field access — key events                             */
/*                                                                    */
/* GTK 3: GdkEventKey struct with ->group, ->is_modifier fields.     */
/* GTK 4: GdkEvent is opaque. Use accessor functions:                 */
/*         gdk_key_event_get_layout() for group,                      */
/*         no direct is_modifier — check keyval against known mods.   */
/* ------------------------------------------------------------------ */
static inline int
spice_compat_key_event_get_group(GdkEvent *event)
{
#if GTK_CHECK_VERSION(4, 0, 0)
    return gdk_key_event_get_layout(event);
#else
    return ((GdkEventKey *)event)->group;
#endif
}

static inline gboolean
spice_compat_key_event_get_is_modifier(GdkEvent *event)
{
#if GTK_CHECK_VERSION(4, 0, 0)
    /* GTK4 removed is_modifier. Check if the keyval is a known modifier. */
    guint keyval = gdk_key_event_get_keyval(event);
    return (keyval == GDK_KEY_Shift_L || keyval == GDK_KEY_Shift_R ||
            keyval == GDK_KEY_Control_L || keyval == GDK_KEY_Control_R ||
            keyval == GDK_KEY_Alt_L || keyval == GDK_KEY_Alt_R ||
            keyval == GDK_KEY_Meta_L || keyval == GDK_KEY_Meta_R ||
            keyval == GDK_KEY_Super_L || keyval == GDK_KEY_Super_R ||
            keyval == GDK_KEY_Hyper_L || keyval == GDK_KEY_Hyper_R ||
            keyval == GDK_KEY_Caps_Lock || keyval == GDK_KEY_Num_Lock ||
            keyval == GDK_KEY_Scroll_Lock ||
            keyval == GDK_KEY_ISO_Lock || keyval == GDK_KEY_ISO_Level3_Shift ||
            keyval == GDK_KEY_ISO_Level5_Shift);
#else
    return ((GdkEventKey *)event)->is_modifier;
#endif
}

/* ------------------------------------------------------------------ */
/* 25. GdkEvent field access — motion events                          */
/*                                                                    */
/* GTK 3: GdkEventMotion struct with ->state, ->x_root, ->y_root.   */
/* GTK 4: GdkEvent is opaque. Use accessor functions. Note that       */
/*         x_root/y_root concept doesn't exist in GTK4 (Wayland has  */
/*         no global coords). We fall back to 0.                      */
/* ------------------------------------------------------------------ */
static inline GdkModifierType
spice_compat_motion_event_get_state(GdkEvent *event)
{
#if GTK_CHECK_VERSION(4, 0, 0)
    return gdk_event_get_modifier_state(event);
#else
    return ((GdkEventMotion *)event)->state;
#endif
}

static inline void
spice_compat_motion_event_get_root_coords(GdkEvent *event G_GNUC_UNUSED,
                                          gdouble *x_root, gdouble *y_root)
{
#if GTK_CHECK_VERSION(4, 0, 0)
    /* GTK4/Wayland has no concept of root/global coordinates.
     * Return 0,0 — callers that need root coords (mouse warping)
     * will need separate handling in Step 2.6. */
    *x_root = 0;
    *y_root = 0;
#else
    *x_root = ((GdkEventMotion *)event)->x_root;
    *y_root = ((GdkEventMotion *)event)->y_root;
#endif
}

/* ------------------------------------------------------------------ */
/* 26. GdkKeymap → GdkDisplay keyval mapping                          */
/*                                                                    */
/* GTK 3: gdk_keymap_get_for_display() + gdk_keymap_get_entries_for_  */
/*         keyval(keymap, keyval, &keys, &n_keys)                     */
/* GTK 4: GdkKeymap removed. Use                                     */
/*         gdk_display_map_keyval(display, keyval, &keys, &n_keys)   */
/*                                                                    */
/* We wrap the keyval→keycode mapping into a single function.         */
/* ------------------------------------------------------------------ */
static inline gboolean
spice_compat_map_keyval(guint keyval, GdkKeymapKey **keys, gint *n_keys)
{
#if GTK_CHECK_VERSION(4, 0, 0)
    return gdk_display_map_keyval(gdk_display_get_default(), keyval, keys, n_keys);
#else
    GdkKeymap *keymap = gdk_keymap_get_for_display(gdk_display_get_default());
    return gdk_keymap_get_entries_for_keyval(keymap, keyval, keys, n_keys);
#endif
}

/* ------------------------------------------------------------------ */
/* 27. Keyboard lock state queries                                    */
/*                                                                    */
/* GTK 3: gdk_keymap_get_caps_lock_state(keymap), etc.               */
/* GTK 4: GdkKeymap removed. Lock state moved to GdkDevice:          */
/*         gdk_device_get_caps_lock_state(device), etc.               */
/*         Use the default seat's keyboard device.                    */
/* ------------------------------------------------------------------ */
static inline gboolean
spice_compat_get_caps_lock_state(void)
{
#if GTK_CHECK_VERSION(4, 0, 0)
    GdkSeat *seat = gdk_display_get_default_seat(gdk_display_get_default());
    GdkDevice *keyboard = gdk_seat_get_keyboard(seat);
    return gdk_device_get_caps_lock_state(keyboard);
#else
    GdkKeymap *keymap = gdk_keymap_get_for_display(gdk_display_get_default());
    return gdk_keymap_get_caps_lock_state(keymap);
#endif
}

static inline gboolean
spice_compat_get_num_lock_state(void)
{
#if GTK_CHECK_VERSION(4, 0, 0)
    GdkSeat *seat = gdk_display_get_default_seat(gdk_display_get_default());
    GdkDevice *keyboard = gdk_seat_get_keyboard(seat);
    return gdk_device_get_num_lock_state(keyboard);
#else
    GdkKeymap *keymap = gdk_keymap_get_for_display(gdk_display_get_default());
    return gdk_keymap_get_num_lock_state(keymap);
#endif
}

static inline gboolean
spice_compat_get_scroll_lock_state(void)
{
#if GTK_CHECK_VERSION(4, 0, 0)
    GdkSeat *seat = gdk_display_get_default_seat(gdk_display_get_default());
    GdkDevice *keyboard = gdk_seat_get_keyboard(seat);
    return gdk_device_get_scroll_lock_state(keyboard);
#else
    GdkKeymap *keymap = gdk_keymap_get_for_display(gdk_display_get_default());
    return gdk_keymap_get_scroll_lock_state(keymap);
#endif
}

/* ------------------------------------------------------------------ */
/* 28. GdkKeymap "state-changed" signal → GdkDevice "changed"        */
/*                                                                    */
/* GTK 3: Connect to GdkKeymap's "state-changed" signal.             */
/* GTK 4: GdkKeymap removed. Connect to the seat keyboard's          */
/*         "changed" signal instead, which fires on modifier changes. */
/*                                                                    */
/* We provide helpers to get the object and signal name so the caller */
/* can use spice_g_signal_connect_object() directly.                  */
/*                                                                    */
/* Note: The callback signature differs:                              */
/*   GTK 3: void cb(GdkKeymap *keymap, gpointer data)               */
/*   GTK 4: void cb(GdkDevice *device, gpointer data)               */
/* Both are compatible with a generic (GObject*, gpointer) pattern.  */
/* ------------------------------------------------------------------ */
static inline gpointer
spice_compat_get_modifier_state_source(void)
{
#if GTK_CHECK_VERSION(4, 0, 0)
    GdkSeat *seat = gdk_display_get_default_seat(gdk_display_get_default());
    return gdk_seat_get_keyboard(seat);
#else
    return gdk_keymap_get_for_display(gdk_display_get_default());
#endif
}

#if GTK_CHECK_VERSION(4, 0, 0)
  #define SPICE_COMPAT_MODIFIER_STATE_SIGNAL  "changed"
#else
  #define SPICE_COMPAT_MODIFIER_STATE_SIGNAL  "state-changed"
#endif

/* ------------------------------------------------------------------ */
/* 29. gdk_cursor_new_from_surface() → gdk_cursor_new_from_texture() */
/*                                                                    */
/* GTK 3: gdk_cursor_new_from_surface(display, surface, x, y)        */
/* GTK 4: gdk_cursor_new_from_surface() removed. Use:                */
/*         gdk_cursor_new_from_texture(texture, hotspot_x, hotspot_y,*/
/*                                     fallback)                      */
/*         Must convert cairo_surface_t to GdkTexture first.          */
/* ------------------------------------------------------------------ */
static inline GdkCursor *
spice_compat_cursor_new_from_surface(GdkDisplay *display G_GNUC_UNUSED,
                                     cairo_surface_t *surface,
                                     double hotspot_x, double hotspot_y)
{
#if GTK_CHECK_VERSION(4, 0, 0)
    GdkTexture *texture;
    GdkCursor *cursor;
    int width, height;
    int stride;
    unsigned char *data;
    GBytes *bytes;

    width = cairo_image_surface_get_width(surface);
    height = cairo_image_surface_get_height(surface);
    stride = cairo_image_surface_get_stride(surface);

    cairo_surface_flush(surface);
    data = cairo_image_surface_get_data(surface);
    bytes = g_bytes_new(data, stride * height);

    texture = gdk_memory_texture_new(width, height,
                                     GDK_MEMORY_B8G8R8A8_PREMULTIPLIED,
                                     bytes, stride);
    g_bytes_unref(bytes);

    cursor = gdk_cursor_new_from_texture(texture,
                                         (int)hotspot_x, (int)hotspot_y,
                                         NULL);
    g_object_unref(texture);
    return cursor;
#else
    return gdk_cursor_new_from_surface(display, surface, hotspot_x, hotspot_y);
#endif
}

/* ------------------------------------------------------------------ */
/* 30. gdk_display_get_primary_monitor() — removed in GTK4            */
/*                                                                    */
/* GTK 3: gdk_display_get_primary_monitor(display)                    */
/* GTK 4: No concept of "primary" monitor. Return the first monitor  */
/*         from the monitor list as a fallback, or NULL.              */
/* ------------------------------------------------------------------ */
static inline GdkMonitor *
spice_compat_display_get_primary_monitor(GdkDisplay *display)
{
#if GTK_CHECK_VERSION(4, 0, 0)
    GListModel *monitors = gdk_display_get_monitors(display);
    if (g_list_model_get_n_items(monitors) > 0)
        return GDK_MONITOR(g_list_model_get_item(monitors, 0));
    return NULL;
#else
    return gdk_display_get_primary_monitor(display);
#endif
}

/* ------------------------------------------------------------------ */
/* 31. gdk_display_get_monitor_at_point() — removed in GTK4           */
/*                                                                    */
/* GTK 3: gdk_display_get_monitor_at_point(display, x, y)            */
/* GTK 4: Removed. Iterate the GListModel of monitors and check      */
/*         which monitor's geometry contains the point.               */
/* ------------------------------------------------------------------ */
static inline GdkMonitor *
spice_compat_display_get_monitor_at_point(GdkDisplay *display, int x, int y)
{
#if GTK_CHECK_VERSION(4, 0, 0)
    GListModel *monitors = gdk_display_get_monitors(display);
    guint n = g_list_model_get_n_items(monitors);
    for (guint i = 0; i < n; i++) {
        GdkMonitor *mon = GDK_MONITOR(g_list_model_get_item(monitors, i));
        GdkRectangle geom;
        gdk_monitor_get_geometry(mon, &geom);
        if (x >= geom.x && x < geom.x + geom.width &&
            y >= geom.y && y < geom.y + geom.height) {
            g_object_unref(mon);
            return GDK_MONITOR(g_list_model_get_item(monitors, i));
        }
        g_object_unref(mon);
    }
    /* Fallback: return the first monitor */
    if (n > 0)
        return GDK_MONITOR(g_list_model_get_item(monitors, 0));
    return NULL;
#else
    return gdk_display_get_monitor_at_point(display, x, y);
#endif
}

/* ------------------------------------------------------------------ */
/* 32. gtk_grab_remove() — removed in GTK4                            */
/*                                                                    */
/* GTK 3: gtk_grab_remove(widget) releases a GTK grab.               */
/* GTK 4: The GTK grab concept is gone; input is managed by event     */
/*         controllers. Make this a no-op.                            */
/* ------------------------------------------------------------------ */
#if GTK_CHECK_VERSION(4, 0, 0)
  #define spice_compat_gtk_grab_remove(widget)  ((void)0)
#else
  #define spice_compat_gtk_grab_remove(widget)  gtk_grab_remove(widget)
#endif

/* ------------------------------------------------------------------ */
/* 33. gtk_widget_get_has_window() — removed in GTK4                  */
/*                                                                    */
/* GTK 3: Returns whether the widget has its own GdkWindow.          */
/* GTK 4: All widgets are backed by a GdkSurface (conceptually), so  */
/*         this always returns TRUE. The function was removed.        */
/* ------------------------------------------------------------------ */
#if GTK_CHECK_VERSION(4, 0, 0)
  #define spice_compat_widget_get_has_window(widget)  (TRUE)
#else
  #define spice_compat_widget_get_has_window(widget)  \
      gtk_widget_get_has_window(widget)
#endif

/* ------------------------------------------------------------------ */
/* 34. gdk_cairo_region() — removed in GTK4                           */
/*                                                                    */
/* GTK 3: gdk_cairo_region(cr, region) adds region to cairo clip.    */
/* GTK 4: Removed. Manually iterate rectangles and add them to the   */
/*         cairo path.                                                */
/* ------------------------------------------------------------------ */
static inline void
spice_compat_cairo_region(cairo_t *cr, const cairo_region_t *region)
{
#if GTK_CHECK_VERSION(4, 0, 0)
    int n = cairo_region_num_rectangles(region);
    for (int i = 0; i < n; i++) {
        cairo_rectangle_int_t rect;
        cairo_region_get_rectangle(region, i, &rect);
        cairo_rectangle(cr, rect.x, rect.y, rect.width, rect.height);
    }
#else
    gdk_cairo_region(cr, region);
#endif
}

/* ------------------------------------------------------------------ */
/* 35. gtk_widget_destroy() — removed in GTK4                         */
/*                                                                    */
/* GTK 3: gtk_widget_destroy(widget) destroys a widget.              */
/* GTK 4: Use gtk_widget_unparent() to remove from parent (which     */
/*         triggers destruction if the parent held the last ref).     */
/*                                                                    */
/* For g_clear_pointer(&ptr, gtk_widget_destroy) patterns, callers   */
/* should use spice_compat_clear_widget(&ptr) instead.               */
/* ------------------------------------------------------------------ */
static inline void
spice_compat_widget_destroy(GtkWidget *widget)
{
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_widget_unparent(widget);
#else
    gtk_widget_destroy(widget);
#endif
}

static inline void
spice_compat_clear_widget(GtkWidget **widget_ptr)
{
    if (*widget_ptr != NULL) {
        spice_compat_widget_destroy(*widget_ptr);
        *widget_ptr = NULL;
    }
}

/* ------------------------------------------------------------------ */
/* 36. GTK_ICON_SIZE_SMALL_TOOLBAR — removed in GTK4                  */
/*                                                                    */
/* GTK 3: GtkIconSize enum with GTK_ICON_SIZE_SMALL_TOOLBAR, etc.    */
/* GTK 4: Icon size enums removed. gtk_image_new_from_icon_name()    */
/*         takes only the icon name (no size parameter).              */
/*                                                                    */
/* We define the constant as 0 for GTK4 so existing call sites still */
/* compile, and provide a wrapper that ignores the size in GTK4.     */
/* ------------------------------------------------------------------ */
#if GTK_CHECK_VERSION(4, 0, 0)
  #define GTK_ICON_SIZE_SMALL_TOOLBAR  0
#endif

static inline GtkWidget *
spice_compat_image_new_from_icon_name(const gchar *icon_name,
                                      gint icon_size G_GNUC_UNUSED)
{
#if GTK_CHECK_VERSION(4, 0, 0)
    return gtk_image_new_from_icon_name(icon_name);
#else
    return gtk_image_new_from_icon_name(icon_name, icon_size);
#endif
}

/* ------------------------------------------------------------------ */
/* 37. gtk_container_add() for non-Box containers — removed in GTK4   */
/*                                                                    */
/* GTK 3: gtk_container_add(GTK_CONTAINER(parent), child)            */
/* GTK 4: Container-specific methods:                                 */
/*         GtkCheckButton → gtk_check_button_set_child()             */
/*         Generic fallback → gtk_widget_set_parent() (but usually   */
/*         each widget has its own add method in GTK4).               */
/*                                                                    */
/* For the GtkCheckButton case used in usb-device-widget.c:          */
/* ------------------------------------------------------------------ */
static inline void
spice_compat_check_button_set_child(GtkCheckButton *button, GtkWidget *child)
{
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_check_button_set_child(button, child);
#else
    gtk_container_add(GTK_CONTAINER(button), child);
#endif
}

/* ------------------------------------------------------------------ */
/* 38. gtk_container_foreach() — removed in GTK4                      */
/*                                                                    */
/* GTK 3: gtk_container_foreach(container, callback, data)           */
/* GTK 4: Iterate with gtk_widget_get_first_child() /                */
/*         gtk_widget_get_next_sibling().                              */
/*                                                                    */
/* Note: We collect children into a list first to allow the callback  */
/* to safely remove/destroy widgets during iteration.                 */
/* ------------------------------------------------------------------ */
static inline void
spice_compat_container_foreach(GtkWidget *container,
                               GtkCallback callback,
                               gpointer callback_data)
{
#if GTK_CHECK_VERSION(4, 0, 0)
    GList *children = NULL;
    GtkWidget *child;

    /* Collect children first so callback can safely modify the tree */
    for (child = gtk_widget_get_first_child(container);
         child != NULL;
         child = gtk_widget_get_next_sibling(child)) {
        children = g_list_prepend(children, child);
    }
    children = g_list_reverse(children);

    for (GList *l = children; l != NULL; l = l->next) {
        callback(GTK_WIDGET(l->data), callback_data);
    }
    g_list_free(children);
#else
    gtk_container_foreach(GTK_CONTAINER(container), callback, callback_data);
#endif
}

/* ------------------------------------------------------------------ */
/* 39. gtk_box_reorder_child() — removed in GTK4                      */
/*                                                                    */
/* GTK 3: gtk_box_reorder_child(box, child, position)                */
/*         position -1 means "move to end".                           */
/* GTK 4: Use gtk_box_reorder_child_after(box, child, sibling).      */
/*         sibling=NULL means "move to start". To move to end, pass  */
/*         the last child as sibling.                                 */
/*                                                                    */
/* We only need the "move to end" case (position == -1).             */
/* ------------------------------------------------------------------ */
static inline void
spice_compat_box_reorder_child_to_end(GtkBox *box, GtkWidget *child)
{
#if GTK_CHECK_VERSION(4, 0, 0)
    /* Find the last child of the box */
    GtkWidget *last = NULL;
    GtkWidget *iter;
    for (iter = gtk_widget_get_first_child(GTK_WIDGET(box));
         iter != NULL;
         iter = gtk_widget_get_next_sibling(iter)) {
        last = iter;
    }
    /* If child is already last, nothing to do */
    if (last != child) {
        gtk_box_reorder_child_after(box, child, last);
    }
#else
    gtk_box_reorder_child(box, child, -1);
#endif
}

/* ------------------------------------------------------------------ */
/* 40. gtk_dialog_run() + gtk_file_chooser_get_filename() — GTK4      */
/*                                                                    */
/* GTK 3: Synchronous gtk_dialog_run() + get_filename().             */
/* GTK 4: Dialogs are async-only, GtkFileChooserDialog removed.      */
/*         Use GtkFileDialog with async API.                          */
/*                                                                    */
/* These changes are too structural for a simple shim — callers use   */
/* #if GTK_CHECK_VERSION(4,0,0) blocks directly in the source.       */
/* No shim provided here; documented for completeness.               */
/* ------------------------------------------------------------------ */

/* ================================================================== */
/* Clipboard-related compat shims                                     */
/* ================================================================== */

/* ------------------------------------------------------------------ */
/* 41. GdkAtom → const char* (MIME type string)                       */
/*                                                                    */
/* GTK 3: GdkAtom is an opaque pointer (interned string pointer).    */
/*         GDK_NONE is ((GdkAtom)NULL).                               */
/*         gdk_atom_intern_static_string(s) returns a GdkAtom.       */
/*         gdk_atom_name(atom) returns a newly-allocated string.      */
/*         Atom comparison uses ==.                                    */
/* GTK 4: GdkAtom was removed entirely. MIME types are plain          */
/*         const char* strings. We typedef GdkAtom and provide shims  */
/*         so existing code compiles under both versions.             */
/*                                                                    */
/* Note: In GTK4, gdk_atom_name() shim returns g_strdup() because    */
/* all existing callers g_free() the result.                          */
/* ------------------------------------------------------------------ */
#if GTK_CHECK_VERSION(4, 0, 0)

typedef const char *GdkAtom;
#define GDK_NONE  ((GdkAtom)NULL)

static inline GdkAtom
spice_compat_atom_intern(const char *atom_name)
{
    /* In GTK4, MIME types are just strings. The "interning" is a no-op.
     * We cast away const to match the return type expectation, but the
     * pointer is still to the original static string. */
    return atom_name;
}
#define gdk_atom_intern_static_string(s)  spice_compat_atom_intern(s)

static inline gchar *
spice_compat_atom_name(GdkAtom atom)
{
    /* Callers g_free() the result, so we must return a copy */
    return g_strdup(atom);
}
#define gdk_atom_name(a)  spice_compat_atom_name(a)

/* Atom comparison: GTK3 uses pointer ==, GTK4 needs string compare */
#define SPICE_COMPAT_ATOM_EQ(a, b)  (g_strcmp0((a), (b)) == 0)

#else /* GTK 3 */

#define SPICE_COMPAT_ATOM_EQ(a, b)  ((a) == (b))

#endif /* GTK version check */

/* ------------------------------------------------------------------ */
/* 42. GtkClipboard → GdkClipboard                                    */
/*                                                                    */
/* GTK 3: GtkClipboard* obtained via gtk_clipboard_get(GDK_SELECTION_*)*/
/*         Signals: "owner-change" with GdkEventOwnerChange* param.   */
/*         Methods: set_with_owner, clear, request_targets, etc.      */
/* GTK 4: GdkClipboard* obtained via gdk_display_get_clipboard().     */
/*         Signal: "changed" (no event parameter).                    */
/*         Methods: set_content, get_formats, read_text_async, etc.   */
/*                                                                    */
/* The APIs are fundamentally different. We provide a typedef so that */
/* struct fields and helper function signatures compile, but the      */
/* actual clipboard operations use #if blocks in the source.          */
/* ------------------------------------------------------------------ */
#if GTK_CHECK_VERSION(4, 0, 0)
  typedef GdkClipboard  SpiceCompatClipboard;
#else
  typedef GtkClipboard  SpiceCompatClipboard;
#endif

/* ------------------------------------------------------------------ */
/* 43. gtk_clipboard_clear() → gdk_clipboard_set_content(cb, NULL)    */
/*                                                                    */
/* GTK 3: gtk_clipboard_clear(clipboard)                              */
/* GTK 4: gdk_clipboard_set_content(clipboard, NULL)                  */
/* ------------------------------------------------------------------ */
static inline void
spice_compat_clipboard_clear(SpiceCompatClipboard *clipboard)
{
#if GTK_CHECK_VERSION(4, 0, 0)
    gdk_clipboard_set_content(clipboard, NULL);
#else
    gtk_clipboard_clear(clipboard);
#endif
}

/* ------------------------------------------------------------------ */
/* 44. gtk_clipboard_get_owner() → gdk_clipboard_is_local()           */
/*                                                                    */
/* GTK 3: gtk_clipboard_get_owner(cb) returns GObject* owner or NULL. */
/*         Used to check if we own the clipboard.                     */
/* GTK 4: gdk_clipboard_is_local(cb) returns TRUE if local.           */
/*         gdk_clipboard_get_content(cb) returns the content provider.*/
/*                                                                    */
/* There's no direct equivalent; callers need #if blocks. But we     */
/* provide a helper for the common "is this owned by us?" check.      */
/* ------------------------------------------------------------------ */
static inline gboolean
spice_compat_clipboard_is_owned_by(SpiceCompatClipboard *clipboard,
                                   gpointer owner)
{
#if GTK_CHECK_VERSION(4, 0, 0)
    (void)owner;
    /* In GTK4 we check if the clipboard is local (owned by this process).
     * The content provider check is done separately where needed. */
    return gdk_clipboard_is_local(clipboard);
#else
    return (gtk_clipboard_get_owner(clipboard) == G_OBJECT(owner));
#endif
}

/* ------------------------------------------------------------------ */
/* 45. gdk_device_warp() — removed in GTK4                            */
/*                                                                    */
/* GTK 3: gdk_device_warp(device, screen, x, y)                      */
/* GTK 4: removed entirely. Use platform-specific pointer warping:    */
/*   - X11: XWarpPointer() via GDK X11 backend APIs                  */
/*   - Wayland: not possible (pointer lock handles relative motion)   */
/*   - Win32: callers already use SetCursorPos() directly             */
/*                                                                    */
/* This shim wraps the GTK3 call on GTK3, and provides an X11-only   */
/* implementation on GTK4.  On GTK4/Wayland the call is a no-op.     */
/* ------------------------------------------------------------------ */
static inline void
spice_compat_device_warp(GdkDisplay *gdk_display, gint x, gint y)
{
#if GTK_CHECK_VERSION(4, 0, 0)
  #ifdef GDK_WINDOWING_X11
    if (GDK_IS_X11_DISPLAY(gdk_display)) {
        Display *xdisplay = gdk_x11_display_get_xdisplay(gdk_display);
        XWarpPointer(xdisplay, None, DefaultRootWindow(xdisplay),
                     0, 0, 0, 0, x, y);
        XFlush(xdisplay);
        return;
    }
  #endif
    /* Wayland / other backends: no-op (pointer lock handles relative motion) */
    (void)gdk_display; (void)x; (void)y;
#else
    GdkDevice *pointer = gdk_seat_get_pointer(
        gdk_display_get_default_seat(gdk_display));
    GdkScreen *screen = gdk_display_get_default_screen(gdk_display);
    gdk_device_warp(pointer, screen, x, y);
#endif
}

/* ------------------------------------------------------------------ */
/* 46. gdk_seat_grab() for pointer — removed in GTK4                  */
/*                                                                    */
/* GTK 3: gdk_seat_grab(seat, surface, CAPABILITY_ALL_POINTING, ...)  */
/* GTK 4: removed. Use platform-specific grabs:                       */
/*   - X11: XGrabPointer()                                            */
/*   - Wayland: pointer lock via zwp_pointer_constraints_v1           */
/*              (already handled separately in wayland-extensions.c)   */
/*   - Win32: ClipCursor() (already handled separately)               */
/*                                                                    */
/* Returns GDK_GRAB_SUCCESS on success. On GTK4/Wayland, always       */
/* returns GDK_GRAB_SUCCESS since actual locking is done via          */
/* wayland-extensions.  On GTK4/X11, maps X11 grab status.           */
/* ------------------------------------------------------------------ */
static inline GdkGrabStatus
spice_compat_grab_pointer(GdkDisplay *gdk_display,
                          SpiceCompatSurface *surface,
                          GdkCursor *cursor)
{
#if GTK_CHECK_VERSION(4, 0, 0)
  #ifdef GDK_WINDOWING_X11
    if (GDK_IS_X11_DISPLAY(gdk_display)) {
        Display *xdisplay = gdk_x11_display_get_xdisplay(gdk_display);
        Window xwindow = GDK_SURFACE_XID(surface);
        Cursor xcursor = None;
        int result;

        if (cursor) {
            /* Get the X cursor from GdkCursor name */
            const char *name = gdk_cursor_get_name(cursor);
            if (name && g_strcmp0(name, "none") == 0) {
                /* Create an invisible cursor */
                static char empty_data[] = { 0 };
                Pixmap pixmap = XCreatePixmapFromBitmapData(
                    xdisplay, xwindow, empty_data, 1, 1, 0, 0, 1);
                XColor color = { 0 };
                xcursor = XCreatePixmapCursor(
                    xdisplay, pixmap, pixmap, &color, &color, 0, 0);
                XFreePixmap(xdisplay, pixmap);
            }
        }

        result = XGrabPointer(xdisplay, xwindow, True,
                              ButtonPressMask | ButtonReleaseMask |
                              PointerMotionMask | EnterWindowMask |
                              LeaveWindowMask,
                              GrabModeAsync, GrabModeAsync,
                              xwindow, xcursor, CurrentTime);

        if (xcursor != None)
            XFreeCursor(xdisplay, xcursor);

        return (result == GrabSuccess) ? GDK_GRAB_SUCCESS : GDK_GRAB_FAILED;
    }
  #endif
    /* Wayland / other: pointer lock is done via zwp_pointer_constraints */
    (void)gdk_display; (void)surface; (void)cursor;
    return GDK_GRAB_SUCCESS;
#else
    GdkSeat *seat = gdk_display_get_default_seat(gdk_display);
    return gdk_seat_grab(seat, surface,
                         GDK_SEAT_CAPABILITY_ALL_POINTING,
                         TRUE, cursor, NULL, NULL, NULL);
#endif
}

/* ------------------------------------------------------------------ */
/* 47. gdk_seat_grab() for keyboard — removed in GTK4                 */
/*                                                                    */
/* GTK 3: gdk_seat_grab(seat, surface, CAPABILITY_KEYBOARD, ...)      */
/* GTK 4: removed. Use platform-specific grabs:                       */
/*   - X11: XGrabKeyboard()                                           */
/*   - Wayland: zwp_keyboard_shortcuts_inhibit_manager_v1             */
/*              (handled in wayland-extensions.c)                      */
/*   - Win32: keyboard hook (already handled separately)              */
/* ------------------------------------------------------------------ */
static inline GdkGrabStatus
spice_compat_grab_keyboard(GdkDisplay *gdk_display,
                           SpiceCompatSurface *surface)
{
#if GTK_CHECK_VERSION(4, 0, 0)
  #ifdef GDK_WINDOWING_X11
    if (GDK_IS_X11_DISPLAY(gdk_display)) {
        Display *xdisplay = gdk_x11_display_get_xdisplay(gdk_display);
        Window xwindow = GDK_SURFACE_XID(surface);
        int result = XGrabKeyboard(xdisplay, xwindow, False,
                                   GrabModeAsync, GrabModeAsync, CurrentTime);
        return (result == GrabSuccess) ? GDK_GRAB_SUCCESS : GDK_GRAB_FAILED;
    }
  #endif
    /* Wayland: keyboard shortcuts inhibit handled via wayland-extensions */
    (void)gdk_display; (void)surface;
    return GDK_GRAB_SUCCESS;
#else
    GdkSeat *seat = gdk_display_get_default_seat(gdk_display);
    return gdk_seat_grab(seat, surface,
                         GDK_SEAT_CAPABILITY_KEYBOARD,
                         FALSE, NULL, NULL, NULL, NULL);
#endif
}

/* ------------------------------------------------------------------ */
/* 48. gdk_seat_ungrab() / gdk_device_ungrab() — removed in GTK4      */
/*                                                                    */
/* GTK 3: gdk_seat_ungrab() releases all grabs;                       */
/*         gdk_device_ungrab() releases a single device grab          */
/* GTK 4: both removed. Use platform-specific ungrab:                 */
/*   - X11: XUngrabPointer() / XUngrabKeyboard()                     */
/*   - Wayland: destroy pointer constraint / keyboard inhibitor       */
/*   - Win32: ClipCursor(NULL) / unhook (handled separately)          */
/* ------------------------------------------------------------------ */
static inline void
spice_compat_ungrab_pointer(GdkDisplay *gdk_display)
{
#if GTK_CHECK_VERSION(4, 0, 0)
  #ifdef GDK_WINDOWING_X11
    if (GDK_IS_X11_DISPLAY(gdk_display)) {
        Display *xdisplay = gdk_x11_display_get_xdisplay(gdk_display);
        XUngrabPointer(xdisplay, CurrentTime);
        XFlush(xdisplay);
        return;
    }
  #endif
    /* Wayland: pointer unlock done via wayland-extensions */
    (void)gdk_display;
#else
    GdkDevice *pointer = gdk_seat_get_pointer(
        gdk_display_get_default_seat(gdk_display));
    G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    gdk_device_ungrab(pointer, GDK_CURRENT_TIME);
    G_GNUC_END_IGNORE_DEPRECATIONS
#endif
}

static inline void
spice_compat_ungrab_keyboard(GdkDisplay *gdk_display)
{
#if GTK_CHECK_VERSION(4, 0, 0)
  #ifdef GDK_WINDOWING_X11
    if (GDK_IS_X11_DISPLAY(gdk_display)) {
        Display *xdisplay = gdk_x11_display_get_xdisplay(gdk_display);
        XUngrabKeyboard(xdisplay, CurrentTime);
        XFlush(xdisplay);
        return;
    }
  #endif
    /* Wayland: keyboard shortcuts inhibit release via wayland-extensions */
    (void)gdk_display;
#else
    GdkDevice *keyboard = gdk_seat_get_keyboard(
        gdk_display_get_default_seat(gdk_display));
    G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    gdk_device_ungrab(keyboard, GDK_CURRENT_TIME);
    G_GNUC_END_IGNORE_DEPRECATIONS
#endif
}

static inline void
spice_compat_seat_ungrab(GdkDisplay *gdk_display)
{
#if GTK_CHECK_VERSION(4, 0, 0)
    /* In GTK4, ungrab both pointer and keyboard */
    spice_compat_ungrab_pointer(gdk_display);
    spice_compat_ungrab_keyboard(gdk_display);
#else
    GdkSeat *seat = gdk_display_get_default_seat(gdk_display);
    gdk_seat_ungrab(seat);
#endif
}

/* ------------------------------------------------------------------ */
/* 49. gdk_window_get_root_coords() — removed in GTK4                 */
/*                                                                    */
/* GTK 3: gdk_window_get_root_coords(window, wx, wy, &rx, &ry)       */
/*         converts window-relative coords to root (screen) coords.   */
/* GTK 4: removed. Use platform-specific translation:                 */
/*   - X11: XTranslateCoordinates() to root window                    */
/*   - Wayland: not meaningful (no absolute positioning),             */
/*              but since warp is a no-op there, output (0,0).        */
/*   - Win32: callers already use Win32 APIs directly                 */
/* ------------------------------------------------------------------ */
static inline void
spice_compat_surface_get_root_coords(SpiceCompatSurface *surface,
                                     gint wx, gint wy,
                                     gint *root_x, gint *root_y)
{
#if GTK_CHECK_VERSION(4, 0, 0)
  #ifdef GDK_WINDOWING_X11
    GdkDisplay *gdk_display = spice_compat_surface_get_display(surface);
    if (GDK_IS_X11_DISPLAY(gdk_display)) {
        Display *xdisplay = gdk_x11_display_get_xdisplay(gdk_display);
        Window xwindow = GDK_SURFACE_XID(surface);
        Window root_ret;
        int rx = 0, ry = 0;
        XTranslateCoordinates(xdisplay, xwindow,
                              XDefaultRootWindow(xdisplay),
                              wx, wy, &rx, &ry, &root_ret);
        *root_x = rx;
        *root_y = ry;
        return;
    }
  #endif
    /* Wayland / other: absolute screen coordinates not available;
     * pointer warp is a no-op anyway, so output zeros. */
    *root_x = 0;
    *root_y = 0;
    (void)surface; (void)wx; (void)wy;
#else
    gdk_window_get_root_coords(surface, wx, wy, root_x, root_y);
#endif
}
