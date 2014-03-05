/*
 * Copyright (C) 2014 Collabora Ltd.
 *
 * Author: Jonny Lamb <jonny.lamb@collabora.co.uk>
 */

#ifndef __MAYNARD_CLOCK_H__
#define __MAYNARD_CLOCK_H__

#include <gtk/gtk.h>

#include "panel.h"

#define MAYNARD_CLOCK_TYPE                 (maynard_clock_get_type ())
#define MAYNARD_CLOCK(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAYNARD_CLOCK_TYPE, MaynardClock))
#define MAYNARD_CLOCK_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), MAYNARD_CLOCK_TYPE, MaynardClockClass))
#define MAYNARD_IS_CLOCK(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MAYNARD_CLOCK_TYPE))
#define MAYNARD_IS_CLOCK_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), MAYNARD_CLOCK_TYPE))
#define MAYNARD_CLOCK_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), MAYNARD_CLOCK_TYPE, MaynardClockClass))

typedef struct MaynardClock MaynardClock;
typedef struct MaynardClockClass MaynardClockClass;
typedef struct MaynardClockPrivate MaynardClockPrivate;

struct MaynardClock
{
  GtkWindow parent;

  MaynardClockPrivate *priv;
};

struct MaynardClockClass
{
  GtkWindowClass parent_class;
};

#define MAYNARD_CLOCK_WIDTH (MAYNARD_PANEL_WIDTH * 2.6)
#define MAYNARD_CLOCK_HEIGHT (MAYNARD_PANEL_WIDTH * 2)

GType maynard_clock_get_type (void) G_GNUC_CONST;

GtkWidget * maynard_clock_new (void);

#endif /* __MAYNARD_CLOCK_H__ */
