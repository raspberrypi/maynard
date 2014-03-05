/*
 * Copyright (C) 2014 Collabora Ltd.
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
