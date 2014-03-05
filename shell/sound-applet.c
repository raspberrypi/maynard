/*
 * Copyright (C) 2013 Collabora Ltd.
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

struct WestonGtkSoundAppletPrivate {
  GtkWidget *image;
  GtkWidget *scale;

  GSettings *settings;
};

G_DEFINE_TYPE(WestonGtkSoundApplet, weston_gtk_sound_applet, GTK_TYPE_BOX)

static void
value_changed_cb (GtkRange *range,
    WestonGtkSoundApplet *self)
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
weston_gtk_sound_applet_dispose (GObject *object)
{
  WestonGtkSoundApplet *self = WESTON_GTK_SOUND_APPLET (object);

  g_clear_object (&self->priv->settings);

  G_OBJECT_CLASS (weston_gtk_sound_applet_parent_class)->dispose (object);
}

static void
weston_gtk_sound_applet_init (WestonGtkSoundApplet *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                            WESTON_GTK_TYPE_SOUND_APPLET,
                                            WestonGtkSoundAppletPrivate);

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
weston_gtk_sound_applet_class_init (WestonGtkSoundAppletClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  object_class->dispose = weston_gtk_sound_applet_dispose;

  g_type_class_add_private (object_class, sizeof (WestonGtkSoundAppletPrivate));
}

GtkWidget *
weston_gtk_sound_applet_new (void)
{
  return g_object_new (WESTON_GTK_TYPE_SOUND_APPLET, NULL);
}
