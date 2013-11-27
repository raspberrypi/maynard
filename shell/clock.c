/*
 * Copyright (C) 2013 Collabora Ltd.
 *
 * Author: Emilio Pozuelo Monfort <emilio.pozuelo@collabora.co.uk>
 */

#include "config.h"

#include "clock.h"

#include <gtk/gtk.h>

#define GNOME_DESKTOP_USE_UNSTABLE_API
#include <libgnome-desktop/gnome-wall-clock.h>

enum {
  PROP_0,
};

struct WestonGtkClockPrivate {
  GnomeWallClock *wall_clock;
};

G_DEFINE_TYPE(WestonGtkClock, weston_gtk_clock, GTK_TYPE_LABEL)

static void
update_clock (GnomeWallClock *wall_clock,
    GParamSpec *pspec,
    WestonGtkClock *self)
{
  gtk_label_set_text (GTK_LABEL (self), gnome_wall_clock_get_clock (wall_clock));
}

static void
weston_gtk_clock_dispose (GObject *object)
{
  WestonGtkClock *self = WESTON_GTK_CLOCK (object);

  g_clear_object (&self->priv->wall_clock);

  G_OBJECT_CLASS (weston_gtk_clock_parent_class)->dispose (object);
}

static void
weston_gtk_clock_init (WestonGtkClock *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                            WESTON_GTK_CLOCK_TYPE,
                                            WestonGtkClockPrivate);

  self->priv->wall_clock = g_object_new (GNOME_TYPE_WALL_CLOCK, NULL);
  g_signal_connect (self->priv->wall_clock, "notify::clock",
                    G_CALLBACK (update_clock), self);
  update_clock (self->priv->wall_clock, NULL, self);
}

static void
weston_gtk_clock_class_init (WestonGtkClockClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  object_class->dispose = weston_gtk_clock_dispose;

  g_type_class_add_private (object_class, sizeof (WestonGtkClockPrivate));
}

GtkWidget *
weston_gtk_clock_new (void)
{
  return g_object_new (WESTON_GTK_CLOCK_TYPE,
                       NULL);
}
