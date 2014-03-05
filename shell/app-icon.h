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

#ifndef __MAYNARD_APP_ICON_H__
#define __MAYNARD_APP_ICON_H__

#include <gtk/gtk.h>

#define MAYNARD_APP_ICON_TYPE                 (maynard_app_icon_get_type ())
#define MAYNARD_APP_ICON(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAYNARD_APP_ICON_TYPE, MaynardAppIcon))
#define MAYNARD_APP_ICON_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), MAYNARD_APP_ICON_TYPE, MaynardAppIconClass))
#define MAYNARD_IS_APP_ICON(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MAYNARD_APP_ICON_TYPE))
#define MAYNARD_IS_APP_ICON_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), MAYNARD_APP_ICON_TYPE))
#define MAYNARD_APP_ICON_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), MAYNARD_APP_ICON_TYPE, MaynardAppIconClass))

typedef struct MaynardAppIcon MaynardAppIcon;
typedef struct MaynardAppIconClass MaynardAppIconClass;
typedef struct MaynardAppIconPrivate MaynardAppIconPrivate;

struct MaynardAppIcon
{
  GtkButton parent;

  MaynardAppIconPrivate *priv;
};

struct MaynardAppIconClass
{
  GtkButtonClass parent_class;
};

GType      maynard_app_icon_get_type       (void) G_GNUC_CONST;
GtkWidget *maynard_app_icon_new            (const gchar *icon_name);
GtkWidget *maynard_app_icon_new_from_gicon (GIcon *icon);

#endif /* __MAYNARD_APP_ICON_H__ */
