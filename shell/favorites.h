/*
 * Copyright (C) 2013 Collabora Ltd.
 *
 * Author: Emilio Pozuelo Monfort <emilio.pozuelo@collabora.co.uk>
 */

#ifndef __WESTON_GTK_FAVORITES_H__
#define __WESTON_GTK_FAVORITES_H__

#include <gtk/gtk.h>

#define WESTON_GTK_TYPE_FAVORITES                 (weston_gtk_favorites_get_type ())
#define WESTON_GTK_FAVORITES(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), WESTON_GTK_TYPE_FAVORITES, WestonGtkFavorites))
#define WESTON_GTK_FAVORITES_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), WESTON_GTK_TYPE_FAVORITES, WestonGtkFavoritesClass))
#define WESTON_GTK_IS_FAVORITES(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WESTON_GTK_TYPE_FAVORITES))
#define WESTON_GTK_IS_FAVORITES_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), WESTON_GTK_TYPE_FAVORITES))
#define WESTON_GTK_FAVORITES_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), WESTON_GTK_TYPE_FAVORITES, WestonGtkFavoritesClass))

typedef struct WestonGtkFavorites WestonGtkFavorites;
typedef struct WestonGtkFavoritesClass WestonGtkFavoritesClass;
typedef struct WestonGtkFavoritesPrivate WestonGtkFavoritesPrivate;

struct WestonGtkFavorites
{
  GtkBox parent;

  WestonGtkFavoritesPrivate *priv;
};

struct WestonGtkFavoritesClass
{
  GtkBoxClass parent_class;
};

GType      weston_gtk_favorites_get_type         (void) G_GNUC_CONST;
GtkWidget *weston_gtk_favorites_new              (void);

#endif /* __WESTON_GTK_FAVORITES_H__ */
