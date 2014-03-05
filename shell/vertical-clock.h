/*
 * Copyright (C) 2014 Collabora Ltd.
 *
 * Author: Jonny Lamb <jonny.lamb@collabora.co.uk>
 */

#ifndef __MAYNARD_VERTICAL_CLOCK_H__
#define __MAYNARD_VERTICAL_CLOCK_H__

#include <gtk/gtk.h>

#define MAYNARD_VERTICAL_CLOCK_TYPE                 (maynard_vertical_clock_get_type ())
#define MAYNARD_VERTICAL_CLOCK(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAYNARD_VERTICAL_CLOCK_TYPE, MaynardVerticalClock))
#define MAYNARD_VERTICAL_CLOCK_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), MAYNARD_VERTICAL_CLOCK_TYPE, MaynardVerticalClockClass))
#define MAYNARD_IS_VERTICAL_CLOCK(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MAYNARD_VERTICAL_CLOCK_TYPE))
#define MAYNARD_IS_VERTICAL_CLOCK_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), MAYNARD_VERTICAL_CLOCK_TYPE))
#define MAYNARD_VERTICAL_CLOCK_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), MAYNARD_VERTICAL_CLOCK_TYPE, MaynardVerticalClockClass))

typedef struct MaynardVerticalClock MaynardVerticalClock;
typedef struct MaynardVerticalClockClass MaynardVerticalClockClass;
typedef struct MaynardVerticalClockPrivate MaynardVerticalClockPrivate;

struct MaynardVerticalClock
{
  GtkBox parent;

  MaynardVerticalClockPrivate *priv;
};

struct MaynardVerticalClockClass
{
  GtkBoxClass parent_class;
};

#define MAYNARD_VERTICAL_CLOCK_WIDTH 25

GType maynard_vertical_clock_get_type (void) G_GNUC_CONST;

GtkWidget * maynard_vertical_clock_new (void);

#endif /* __MAYNARD_VERTICAL_CLOCK_H__ */
