/*
 * Copyright (C) 2013-2014 Collabora Ltd.
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
 * Authors: Emilio Pozuelo Monfort <emilio.pozuelo@collabora.co.uk>
 *          Jonny Lamb <jonny.lamb@collabora.co.uk>
 */

#ifndef __MAYNARD_LAUNCHER_H__
#define __MAYNARD_LAUNCHER_H__

#include <gtk/gtk.h>

#define MAYNARD_LAUNCHER_TYPE                 (maynard_launcher_get_type ())
#define MAYNARD_LAUNCHER(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAYNARD_LAUNCHER_TYPE, MaynardLauncher))
#define MAYNARD_LAUNCHER_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), MAYNARD_LAUNCHER_TYPE, MaynardLauncherClass))
#define MAYNARD_IS_LAUNCHER(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MAYNARD_LAUNCHER_TYPE))
#define MAYNARD_IS_LAUNCHER_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), MAYNARD_LAUNCHER_TYPE))
#define MAYNARD_LAUNCHER_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), MAYNARD_LAUNCHER_TYPE, MaynardLauncherClass))

typedef struct MaynardLauncher MaynardLauncher;
typedef struct MaynardLauncherClass MaynardLauncherClass;
typedef struct MaynardLauncherPrivate MaynardLauncherPrivate;

struct MaynardLauncher
{
  GtkWindow parent;

  MaynardLauncherPrivate *priv;
};

struct MaynardLauncherClass
{
  GtkWindowClass parent_class;
};

GType maynard_launcher_get_type (void) G_GNUC_CONST;

GtkWidget * maynard_launcher_new (GtkWidget *background_widget);

void maynard_launcher_calculate (MaynardLauncher *self,
    gint *grid_window_width, gint *grid_window_height,
    gint *grid_cols);

#endif /* __MAYNARD_LAUNCHER_H__ */
