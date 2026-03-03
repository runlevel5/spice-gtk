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
