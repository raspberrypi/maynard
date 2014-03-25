/*
 * Copyright (C) 2014 Collabora Ltd.
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
 * Author: Jonny Lamb <jonny.lamb@collabora.co.uk>
 */

#include "config.h"

#include "clock.h"

#define GNOME_DESKTOP_USE_UNSTABLE_API
#include <libgnome-desktop/gnome-wall-clock.h>

enum {
  VOLUME_CHANGED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

struct MaynardClockPrivate {
  GtkWidget *revealer_clock;
  GtkWidget *revealer_volume;

  GtkWidget *label;

  GtkWidget *volume_scale;
  GtkWidget *volume_image;

  GnomeWallClock *wall_clock;
};

G_DEFINE_TYPE(MaynardClock, maynard_clock, GTK_TYPE_WINDOW)

static void
maynard_clock_init (MaynardClock *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      MAYNARD_CLOCK_TYPE,
      MaynardClockPrivate);
}

static void
volume_changed_cb (GtkRange *range,
    MaynardClock *self)
{
  GError *error = NULL;
  gdouble value;
  gchar *cmd;
  const gchar *icon_name;
  GtkWidget *box;

  value = gtk_range_get_value (range);

  cmd = g_strdup_printf ("amixer set PCM %d%%", (int) value);

  g_spawn_command_line_async (cmd, &error);

  if (error)
    {
      g_print ("failed to set volume: %s\n", error->message);
      g_clear_error (&error);
    }

  g_free (cmd);

  /* update the icon */
  if (value > 75)
    icon_name = "audio-volume-high-symbolic";
  else if (value > 50)
    icon_name = "audio-volume-medium-symbolic";
  else if (value > 25)
    icon_name = "audio-volume-low-symbolic";
  else if (value == 0)
    icon_name = "audio-volume-muted-symbolic";

  box = gtk_widget_get_parent (self->priv->volume_image);
  gtk_widget_destroy (self->priv->volume_image);
  self->priv->volume_image = gtk_image_new_from_icon_name (
      icon_name, GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_box_pack_start (GTK_BOX (box), self->priv->volume_image,
      FALSE, FALSE, 0);
  gtk_widget_show (self->priv->volume_image);

  g_signal_emit (self, signals[VOLUME_CHANGED], 0, value, icon_name);
}

static GtkWidget *
create_volume_box (MaynardClock *self)
{
  GtkWidget *box;

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

  self->priv->volume_image = gtk_image_new_from_icon_name (
      "audio-volume-high-symbolic",
      GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_box_pack_start (GTK_BOX (box), self->priv->volume_image,
      FALSE, FALSE, 0);

  self->priv->volume_scale = gtk_scale_new_with_range (
      GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
  gtk_scale_set_draw_value (GTK_SCALE (self->priv->volume_scale), FALSE);
  gtk_widget_set_size_request (self->priv->volume_scale, 100, -1);
  gtk_box_pack_end (GTK_BOX (box), self->priv->volume_scale, TRUE, TRUE, 0);

  g_signal_connect (self->priv->volume_scale, "value-changed",
      G_CALLBACK (volume_changed_cb), self);

  return box;
}

static void
wall_clock_notify_cb (GnomeWallClock *wall_clock,
    GParamSpec *pspec,
    MaynardClock *self)
{
  GDateTime *datetime;
  gchar *str;

  datetime = g_date_time_new_now_local ();

  str = g_date_time_format (datetime,
      "<span font=\"Droid Sans 32\">%H:%M</span>\n"
      "<span font=\"Droid Sans 12\">%d/%m/%Y</span>");
  gtk_label_set_markup (GTK_LABEL (self->priv->label), str);

  g_free (str);
  g_date_time_unref (datetime);
}

static void
maynard_clock_constructed (GObject *object)
{
  MaynardClock *self = MAYNARD_CLOCK (object);
  GtkWidget *box, *volume_box;

  G_OBJECT_CLASS (maynard_clock_parent_class)->constructed (object);

  self->priv->wall_clock = g_object_new (GNOME_TYPE_WALL_CLOCK, NULL);
  g_signal_connect (self->priv->wall_clock, "notify::clock",
      G_CALLBACK (wall_clock_notify_cb), self);

  gtk_window_set_title (GTK_WINDOW (self), "maynard");
  gtk_window_set_decorated (GTK_WINDOW (self), FALSE);
  gtk_widget_realize (GTK_WIDGET (self));

  gtk_style_context_add_class (
      gtk_widget_get_style_context (GTK_WIDGET (self)),
      "maynard-clock");

  /* the box for the revealers */
  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add (GTK_CONTAINER (self), box);

  /* volume */
  self->priv->revealer_volume = gtk_revealer_new ();
  gtk_revealer_set_transition_type (
      GTK_REVEALER (self->priv->revealer_volume),
      GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT);
  gtk_revealer_set_reveal_child (
      GTK_REVEALER (self->priv->revealer_volume), FALSE);
  gtk_box_pack_start (GTK_BOX (box), self->priv->revealer_volume,
      TRUE, TRUE, 0);

  volume_box = create_volume_box (self);
  gtk_container_add (GTK_CONTAINER (self->priv->revealer_volume),
      volume_box);

  /* clock */
  self->priv->revealer_clock = gtk_revealer_new ();
  gtk_revealer_set_transition_type (
      GTK_REVEALER (self->priv->revealer_clock),
      GTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT);
  gtk_revealer_set_reveal_child (
      GTK_REVEALER (self->priv->revealer_clock), TRUE);
  gtk_box_pack_start (GTK_BOX (box), self->priv->revealer_clock,
      TRUE, TRUE, 0);

  self->priv->label = gtk_label_new ("");
  gtk_label_set_justify (GTK_LABEL (self->priv->label), GTK_JUSTIFY_CENTER);
  gtk_container_add (GTK_CONTAINER (self->priv->revealer_clock),
      self->priv->label);

  wall_clock_notify_cb (self->priv->wall_clock, NULL, self);
}

static void
maynard_clock_dispose (GObject *object)
{
  MaynardClock *self = MAYNARD_CLOCK (object);

  g_clear_object (&self->priv->wall_clock);

  G_OBJECT_CLASS (maynard_clock_parent_class)->dispose (object);
}

static void
maynard_clock_class_init (MaynardClockClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  object_class->constructed = maynard_clock_constructed;
  object_class->dispose = maynard_clock_dispose;

  signals[VOLUME_CHANGED] = g_signal_new ("volume-changed",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 2, G_TYPE_DOUBLE, G_TYPE_STRING);

  g_type_class_add_private (object_class, sizeof (MaynardClockPrivate));
}

GtkWidget *
maynard_clock_new (void)
{
  return g_object_new (MAYNARD_CLOCK_TYPE,
      NULL);
}

void
maynard_clock_show_volume (MaynardClock *self,
    gboolean value)
{
  gtk_revealer_set_reveal_child (
      GTK_REVEALER (self->priv->revealer_volume), value);
  gtk_revealer_set_reveal_child (
      GTK_REVEALER (self->priv->revealer_clock), !value);
}
