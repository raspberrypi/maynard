/*
 * Copyright (C) 2013 Collabora Ltd.
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
 * Author: Emilio Pozuelo Monfort <emilio.pozuelo@collabora.co.uk>
 */

#ifndef __MAYNARD_FAVORITES_H__
#define __MAYNARD_FAVORITES_H__

#include <gtk/gtk.h>

#define MAYNARD_TYPE_FAVORITES                 (maynard_favorites_get_type ())
#define MAYNARD_FAVORITES(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAYNARD_TYPE_FAVORITES, MaynardFavorites))
#define MAYNARD_FAVORITES_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), MAYNARD_TYPE_FAVORITES, MaynardFavoritesClass))
#define MAYNARD_IS_FAVORITES(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MAYNARD_TYPE_FAVORITES))
#define MAYNARD_IS_FAVORITES_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), MAYNARD_TYPE_FAVORITES))
#define MAYNARD_FAVORITES_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), MAYNARD_TYPE_FAVORITES, MaynardFavoritesClass))

typedef struct MaynardFavorites MaynardFavorites;
typedef struct MaynardFavoritesClass MaynardFavoritesClass;
typedef struct MaynardFavoritesPrivate MaynardFavoritesPrivate;

struct MaynardFavorites
{
  GtkBox parent;

  MaynardFavoritesPrivate *priv;
};

struct MaynardFavoritesClass
{
  GtkBoxClass parent_class;
};

GType      maynard_favorites_get_type         (void) G_GNUC_CONST;
GtkWidget *maynard_favorites_new              (void);

#endif /* __MAYNARD_FAVORITES_H__ */
