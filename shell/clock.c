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

struct MaynardClockPrivate {
  GtkWidget *label;

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

  G_OBJECT_CLASS (maynard_clock_parent_class)->constructed (object);

  self->priv->wall_clock = g_object_new (GNOME_TYPE_WALL_CLOCK, NULL);
  g_signal_connect (self->priv->wall_clock, "notify::clock",
      G_CALLBACK (wall_clock_notify_cb), self);

  gtk_window_set_title (GTK_WINDOW (self), "gtk shell");
  gtk_window_set_decorated (GTK_WINDOW (self), FALSE);
  gtk_widget_realize (GTK_WIDGET (self));

  gtk_style_context_add_class (
      gtk_widget_get_style_context (GTK_WIDGET (self)),
      "wgs-clock");

  self->priv->label = gtk_label_new ("");
  gtk_label_set_justify (GTK_LABEL (self->priv->label), GTK_JUSTIFY_CENTER);
  gtk_container_add (GTK_CONTAINER (self), self->priv->label);

  wall_clock_notify_cb (self->priv->wall_clock, NULL, self);
}

static void
maynard_clock_dispose (GObject *object)
{
  MaynardClock *self = MAYNARD_CLOCK (object);

  if (self->priv->wall_clock)
    g_object_unref (self->priv->wall_clock);
  self->priv->wall_clock = NULL;

  G_OBJECT_CLASS (maynard_clock_parent_class)->dispose (object);
}

static void
maynard_clock_class_init (MaynardClockClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  object_class->constructed = maynard_clock_constructed;
  object_class->dispose = maynard_clock_dispose;

  g_type_class_add_private (object_class, sizeof (MaynardClockPrivate));
}

GtkWidget *
maynard_clock_new (void)
{
  return g_object_new (MAYNARD_CLOCK_TYPE,
      NULL);
}
