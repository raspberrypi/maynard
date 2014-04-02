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

typedef enum {
  MAYNARD_CLOCK_SECTION_CLOCK,
  MAYNARD_CLOCK_SECTION_SYSTEM,
  MAYNARD_CLOCK_SECTION_VOLUME
} MaynardClockSection;

GType maynard_clock_get_type (void) G_GNUC_CONST;

GtkWidget * maynard_clock_new (void);

void maynard_clock_show_section (MaynardClock *self, MaynardClockSection section);

#endif /* __MAYNARD_CLOCK_H__ */
