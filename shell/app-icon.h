/*
 * Copyright (C) 2014 Collabora Ltd.
 *
 * Author: Jonny Lamb <jonny.lamb@collabora.co.uk>
 */

#ifndef __WESTON_GTK_APP_ICON_H__
#define __WESTON_GTK_APP_ICON_H__

#include <gtk/gtk.h>

#define WESTON_GTK_APP_ICON_TYPE                 (weston_gtk_app_icon_get_type ())
#define WESTON_GTK_APP_ICON(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), WESTON_GTK_APP_ICON_TYPE, WestonGtkAppIcon))
#define WESTON_GTK_APP_ICON_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), WESTON_GTK_APP_ICON_TYPE, WestonGtkAppIconClass))
#define WESTON_GTK_IS_APP_ICON(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WESTON_GTK_APP_ICON_TYPE))
#define WESTON_GTK_IS_APP_ICON_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), WESTON_GTK_APP_ICON_TYPE))
#define WESTON_GTK_APP_ICON_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), WESTON_GTK_APP_ICON_TYPE, WestonGtkAppIconClass))

typedef struct WestonGtkAppIcon WestonGtkAppIcon;
typedef struct WestonGtkAppIconClass WestonGtkAppIconClass;
typedef struct WestonGtkAppIconPrivate WestonGtkAppIconPrivate;

struct WestonGtkAppIcon
{
  GtkButton parent;

  WestonGtkAppIconPrivate *priv;
};

struct WestonGtkAppIconClass
{
  GtkButtonClass parent_class;
};

GType      weston_gtk_app_icon_get_type       (void) G_GNUC_CONST;
GtkWidget *weston_gtk_app_icon_new            (const gchar *icon_name);
GtkWidget *weston_gtk_app_icon_new_from_gicon (GIcon *icon);

#endif /* __WESTON_GTK_APP_ICON_H__ */
