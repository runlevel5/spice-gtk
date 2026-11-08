/*
   Copyright (C) 2014-2016 Red Hat, Inc.

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
#include "config.h"

#include <math.h>
#include <gdk/gdk.h>

#include "spice-widget.h"
#include "spice-widget-priv.h"
#include "spice-gtk-session-priv.h"

/*
 * GTK4 EGL implementation: uses GdkDmabufTextureBuilder to import DMA-BUF
 * scanouts as GdkTexture objects. GTK4 handles all GL compositing internally;
 * we just paint the texture in the draw callback via Cairo download.
 */

G_GNUC_INTERNAL
gboolean spice_egl_init(SpiceDisplay *display, GError **err)
{
    SpiceDisplayPrivate *d = display->priv;

    DISPLAY_DEBUG(display, "GTK4 dmabuf texture backend init");

    d->egl.context_ready = TRUE;

    if (spice_display_channel_get_gl_scanout2(d->display) != NULL) {
        DISPLAY_DEBUG(display, "scanout present during egl init, updating widget");
        spice_display_widget_gl_scanout(display);
        spice_display_widget_update_monitor_area(display);
    }

    return TRUE;
}

G_GNUC_INTERNAL
gboolean spice_egl_realize_display(SpiceDisplay *display, GdkSurface *win,
                                   GError **err)
{
    /* GTK4: no manual EGL surface needed, texture pipeline is display-managed */
    DISPLAY_DEBUG(display, "egl realize (GTK4 dmabuf path, no-op)");
    return TRUE;
}

G_GNUC_INTERNAL
void spice_egl_unrealize_display(SpiceDisplay *display)
{
    SpiceDisplayPrivate *d = display->priv;

    DISPLAY_DEBUG(display, "egl unrealize (GTK4)");

    g_clear_object(&d->egl.scanout_texture);
    d->egl.context_ready = FALSE;
}

G_GNUC_INTERNAL
void spice_egl_resize_display(SpiceDisplay *display, int w, int h)
{
    /* GTK4: the draw callback handles scaling via Cairo; no GL viewport needed */
}

G_GNUC_INTERNAL
void spice_egl_update_display(SpiceDisplay *display)
{
    /* GTK4: rendering is done in draw_event_impl via scanout_texture;
     * this function only exists to satisfy callers that may still invoke it */
}

G_GNUC_INTERNAL
void spice_egl_cursor_set(SpiceDisplay *display)
{
    /* GTK4: cursor compositing over GL scanout is handled via the normal
     * GDK cursor path since GTK4 composites everything through the render
     * node tree. No manual GL cursor overlay needed. */
}

G_GNUC_INTERNAL
gboolean spice_egl_update_scanout(SpiceDisplay *display,
                                  const SpiceGlScanout2 *scanout,
                                  GError **err)
{
    SpiceDisplayPrivate *d = display->priv;
    GdkDmabufTextureBuilder *builder;
    GdkDisplay *gdk_dpy;
    guint j;

    g_return_val_if_fail(scanout != NULL, FALSE);
    g_return_val_if_fail(scanout->num_planes <= 4, FALSE);

    DISPLAY_DEBUG(display, "GTK4 dmabuf update: fd[0]:%d stride[0]:%u y0:%d %ux%u "
                  "format:0x%x (%c%c%c%c) modifier:0x%" G_GINT64_MODIFIER "x",
                  scanout->fd[0], scanout->stride[0], scanout->y0top,
                  scanout->width, scanout->height, scanout->format,
                  (int)scanout->format & 0xff,
                  (int)(scanout->format >> 8) & 0xff,
                  (int)(scanout->format >> 16) & 0xff,
                  (int)(scanout->format >> 24) & 0xff,
                  scanout->modifier);

    /* Release previous texture */
    g_clear_object(&d->egl.scanout_texture);

    d->egl.scanout = *scanout;

    if (scanout->fd[0] == -1)
        return TRUE;

    gdk_dpy = gtk_widget_get_display(GTK_WIDGET(display));
    builder = gdk_dmabuf_texture_builder_new();

    gdk_dmabuf_texture_builder_set_display(builder, gdk_dpy);
    gdk_dmabuf_texture_builder_set_width(builder, scanout->width);
    gdk_dmabuf_texture_builder_set_height(builder, scanout->height);
    gdk_dmabuf_texture_builder_set_fourcc(builder, scanout->format);
    gdk_dmabuf_texture_builder_set_modifier(builder, scanout->modifier);
    gdk_dmabuf_texture_builder_set_n_planes(builder, scanout->num_planes);
    gdk_dmabuf_texture_builder_set_premultiplied(builder, TRUE);

    for (j = 0; j < scanout->num_planes; j++) {
        int fd = scanout->fd[j] >= 0 ? scanout->fd[j] : scanout->fd[0];
        gdk_dmabuf_texture_builder_set_fd(builder, j, fd);
        gdk_dmabuf_texture_builder_set_stride(builder, j, scanout->stride[j]);
        gdk_dmabuf_texture_builder_set_offset(builder, j, scanout->offset[j]);
    }

    d->egl.scanout_texture = gdk_dmabuf_texture_builder_build(builder,
                                                               NULL, NULL,
                                                               err);
    g_object_unref(builder);

    if (d->egl.scanout_texture == NULL) {
        g_prefix_error(err, "Failed to create dmabuf texture: ");
        return FALSE;
    }

    DISPLAY_DEBUG(display, "GTK4 dmabuf texture created: %ux%u",
                  gdk_texture_get_width(d->egl.scanout_texture),
                  gdk_texture_get_height(d->egl.scanout_texture));

    return TRUE;
}
