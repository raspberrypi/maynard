/*
 * Copyright (C) 2014 Collabora Ltd.
 *
 * Author: Jonny Lamb <jonny.lamb@collabora.co.uk>
 */

#include "config.h"

#include "clock.h"

struct WestonGtkClockPrivate {
  GtkWidget *label;
};

G_DEFINE_TYPE(WestonGtkClock, weston_gtk_clock, GTK_TYPE_WINDOW)

static void
weston_gtk_clock_init (WestonGtkClock *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      WESTON_GTK_CLOCK_TYPE,
      WestonGtkClockPrivate);
}

static void
weston_gtk_clock_constructed (GObject *object)
{
  WestonGtkClock *self = WESTON_GTK_CLOCK (object);

  G_OBJECT_CLASS (weston_gtk_clock_parent_class)->constructed (object);

  gtk_window_set_title (GTK_WINDOW (self), "gtk shell");
  gtk_window_set_decorated (GTK_WINDOW (self), FALSE);
  gtk_widget_realize (GTK_WIDGET (self));

  gtk_style_context_add_class (
      gtk_widget_get_style_context (GTK_WIDGET (self)),
      "wgs-clock");

  self->priv->label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (self->priv->label),
      "<span font=\"Droid Sans 32\">11:47</span>\n"
      "<span font=\"Droid Sans 12\">13/02/2014</span>");
  gtk_label_set_justify (GTK_LABEL (self->priv->label), GTK_JUSTIFY_CENTER);
  gtk_container_add (GTK_CONTAINER (self), self->priv->label);
}

static void
weston_gtk_clock_dispose (GObject *object)
{
  WestonGtkClock *self = WESTON_GTK_CLOCK (object);

  G_OBJECT_CLASS (weston_gtk_clock_parent_class)->dispose (object);
}

static void
weston_gtk_clock_class_init (WestonGtkClockClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  object_class->constructed = weston_gtk_clock_constructed;
  object_class->dispose = weston_gtk_clock_dispose;

  g_type_class_add_private (object_class, sizeof (WestonGtkClockPrivate));
}

GtkWidget *
weston_gtk_clock_new (void)
{
  return g_object_new (WESTON_GTK_CLOCK_TYPE,
      NULL);
}
