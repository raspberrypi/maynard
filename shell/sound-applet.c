/*
 * Copyright (C) 2013 Collabora Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 *
 * Author: Emilio Pozuelo Monfort <emilio.pozuelo@collabora.co.uk>
 */

#include "config.h"

#include "sound-applet.h"

#include <gio/gio.h>
#include <gtk/gtk.h>

enum {
  PROP_0,
};

struct MaynardSoundAppletPrivate {
  GtkWidget *image;
  GtkWidget *scale;

  GSettings *settings;
};

G_DEFINE_TYPE(MaynardSoundApplet, maynard_sound_applet, GTK_TYPE_BOX)

static void
value_changed_cb (GtkRange *range,
    MaynardSoundApplet *self)
{
  GError *error = NULL;
  gdouble value;
  gchar *cmd;

  value = gtk_range_get_value (range);

  cmd = g_strdup_printf ("amixer set PCM %d%%", (int) value);

  g_spawn_command_line_async (cmd, &error);

  if (error)
    {
      g_print ("failed to set volume: %s\n", error->message);
      g_clear_error (&error);
    }

  g_free (cmd);
}

static void
maynard_sound_applet_dispose (GObject *object)
{
  MaynardSoundApplet *self = MAYNARD_SOUND_APPLET (object);

  g_clear_object (&self->priv->settings);

  G_OBJECT_CLASS (maynard_sound_applet_parent_class)->dispose (object);
}

static void
maynard_sound_applet_init (MaynardSoundApplet *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                            MAYNARD_TYPE_SOUND_APPLET,
                                            MaynardSoundAppletPrivate);

  self->priv->settings = g_settings_new ("org.raspberrypi.maynard");

  gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_HORIZONTAL);

  /* FIXME: make this nicer. GtkVolumeButton doesn't work well for us
   * because the scale will be placed behind the panel. But we can
   * mimic/reimplement it... or fix GtkVolumeButton.
   */
  self->priv->image = gtk_image_new_from_icon_name ("audio-volume-high", GTK_ICON_SIZE_MENU);
  gtk_box_pack_start (GTK_BOX (self), self->priv->image, FALSE, FALSE, 0);

  self->priv->scale = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL,
      0, 100, 1);
  gtk_scale_set_draw_value (GTK_SCALE (self->priv->scale), FALSE);
  gtk_widget_set_size_request (self->priv->scale, 100, -1);
  g_signal_connect (self->priv->scale, "value-changed",
      G_CALLBACK (value_changed_cb), self);
  gtk_box_pack_start (GTK_BOX (self), self->priv->scale, FALSE, FALSE, 0);
}

static void
maynard_sound_applet_class_init (MaynardSoundAppletClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  object_class->dispose = maynard_sound_applet_dispose;

  g_type_class_add_private (object_class, sizeof (MaynardSoundAppletPrivate));
}

GtkWidget *
maynard_sound_applet_new (void)
{
  return g_object_new (MAYNARD_TYPE_SOUND_APPLET, NULL);
}
