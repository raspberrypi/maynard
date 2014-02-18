/*
 * Copyright (C) 2014 Collabora Ltd.
 *
 * Author: Jonny Lamb <jonny.lamb@collabora.co.uk>
 */

#include "config.h"

#include "vertical-clock.h"

#include "panel.h"

struct WestonGtkVerticalClockPrivate {
  GtkWidget *label;
};

G_DEFINE_TYPE(WestonGtkVerticalClock, weston_gtk_vertical_clock, GTK_TYPE_BOX)

/* this widget takes up the entire width of the panel and displays
 * padding for the first (panel width - vertical clock width) pixels,
 * then shows the vertical clock itself. the idea is to put this into
 * a GtkRevealer and only show it when appropriate. */

static void
weston_gtk_vertical_clock_init (WestonGtkVerticalClock *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      WESTON_GTK_VERTICAL_CLOCK_TYPE,
      WestonGtkVerticalClockPrivate);
}

static void
weston_gtk_vertical_clock_constructed (GObject *object)
{
  WestonGtkVerticalClock *self = WESTON_GTK_VERTICAL_CLOCK (object);
  GtkWidget *padding;
  gint width;

  G_OBJECT_CLASS (weston_gtk_vertical_clock_parent_class)->constructed (object);

  gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_HORIZONTAL);

  /* a label just to pad things out to the correct width */
  width = WESTON_GTK_PANEL_WIDTH - WESTON_GTK_VERTICAL_CLOCK_WIDTH;

  padding = gtk_label_new ("");
  gtk_style_context_add_class (gtk_widget_get_style_context (padding),
      "wgs-clock");
  gtk_widget_set_size_request (padding, width, -1);
  gtk_box_pack_start (GTK_BOX (self), padding, FALSE, FALSE, 0);

  /* the actual clock label */
  self->priv->label = gtk_label_new ("");
  gtk_label_set_markup (GTK_LABEL (self->priv->label),
      "<span font=\"Droid Sans 12\">11\n"
      ":\n"
      "47</span>");
  gtk_style_context_add_class (gtk_widget_get_style_context (self->priv->label),
      "wgs-clock");
  gtk_label_set_justify (GTK_LABEL (self->priv->label), GTK_JUSTIFY_CENTER);
  gtk_widget_set_size_request (self->priv->label,
      WESTON_GTK_VERTICAL_CLOCK_WIDTH, -1);
  gtk_box_pack_start (GTK_BOX (self), self->priv->label, FALSE, FALSE, 0);
}

static void
weston_gtk_vertical_clock_dispose (GObject *object)
{
  WestonGtkVerticalClock *self = WESTON_GTK_VERTICAL_CLOCK (object);

  G_OBJECT_CLASS (weston_gtk_vertical_clock_parent_class)->dispose (object);
}

static void
weston_gtk_vertical_clock_class_init (WestonGtkVerticalClockClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  object_class->constructed = weston_gtk_vertical_clock_constructed;
  object_class->dispose = weston_gtk_vertical_clock_dispose;

  g_type_class_add_private (object_class, sizeof (WestonGtkVerticalClockPrivate));
}

GtkWidget *
weston_gtk_vertical_clock_new (void)
{
  return g_object_new (WESTON_GTK_VERTICAL_CLOCK_TYPE,
      NULL);
}
