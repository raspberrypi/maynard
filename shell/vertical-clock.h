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
