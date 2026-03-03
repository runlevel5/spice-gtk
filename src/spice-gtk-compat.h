/*
   Copyright (C) 2024 Red Hat, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, see <http://www.gnu.org/licenses/>.
*/
#pragma once

/*
 * GTK3/GTK4 compatibility macros.
 *
 * This header provides shims so that Phase 1 (GTK 3.24) code can compile
 * against both GTK 3 and GTK 4.  Each section documents the API change
 * and the chosen compatibility strategy.
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
