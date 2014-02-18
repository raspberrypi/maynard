/*
 * Copyright (C) 2014 Collabora Ltd.
 *
 * Author: Jonny Lamb <jonny.lamb@collabora.co.uk>
 */

#ifndef __WESTON_GTK_VERTICAL_CLOCK_H__
#define __WESTON_GTK_VERTICAL_CLOCK_H__

#include <gtk/gtk.h>

#define WESTON_GTK_VERTICAL_CLOCK_TYPE                 (weston_gtk_vertical_clock_get_type ())
#define WESTON_GTK_VERTICAL_CLOCK(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), WESTON_GTK_VERTICAL_CLOCK_TYPE, WestonGtkVerticalClock))
#define WESTON_GTK_VERTICAL_CLOCK_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), WESTON_GTK_VERTICAL_CLOCK_TYPE, WestonGtkVerticalClockClass))
#define WESTON_GTK_IS_VERTICAL_CLOCK(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WESTON_GTK_VERTICAL_CLOCK_TYPE))
#define WESTON_GTK_IS_VERTICAL_CLOCK_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), WESTON_GTK_VERTICAL_CLOCK_TYPE))
#define WESTON_GTK_VERTICAL_CLOCK_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), WESTON_GTK_VERTICAL_CLOCK_TYPE, WestonGtkVerticalClockClass))

typedef struct WestonGtkVerticalClock WestonGtkVerticalClock;
typedef struct WestonGtkVerticalClockClass WestonGtkVerticalClockClass;
typedef struct WestonGtkVerticalClockPrivate WestonGtkVerticalClockPrivate;

struct WestonGtkVerticalClock
{
  GtkBox parent;

  WestonGtkVerticalClockPrivate *priv;
};

struct WestonGtkVerticalClockClass
{
  GtkBoxClass parent_class;
};

#define WESTON_GTK_VERTICAL_CLOCK_WIDTH 25

GType weston_gtk_vertical_clock_get_type (void) G_GNUC_CONST;

GtkWidget * weston_gtk_vertical_clock_new (void);

#endif /* __WESTON_GTK_VERTICAL_CLOCK_H__ */
