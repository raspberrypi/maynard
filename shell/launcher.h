/*
 * Copyright (C) 2014 Collabora Ltd.
 *
 * Author: Jonny Lamb <jonny.lamb@collabora.co.uk>
 */

#ifndef __WESTON_GTK_LAUNCHER_H__
#define __WESTON_GTK_LAUNCHER_H__

#include <gtk/gtk.h>

#define WESTON_GTK_LAUNCHER_TYPE                 (weston_gtk_launcher_get_type ())
#define WESTON_GTK_LAUNCHER(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), WESTON_GTK_LAUNCHER_TYPE, WestonGtkLauncher))
#define WESTON_GTK_LAUNCHER_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), WESTON_GTK_LAUNCHER_TYPE, WestonGtkLauncherClass))
#define WESTON_GTK_IS_LAUNCHER(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WESTON_GTK_LAUNCHER_TYPE))
#define WESTON_GTK_IS_LAUNCHER_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), WESTON_GTK_LAUNCHER_TYPE))
#define WESTON_GTK_LAUNCHER_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), WESTON_GTK_LAUNCHER_TYPE, WestonGtkLauncherClass))

typedef struct WestonGtkLauncher WestonGtkLauncher;
typedef struct WestonGtkLauncherClass WestonGtkLauncherClass;
typedef struct WestonGtkLauncherPrivate WestonGtkLauncherPrivate;

struct WestonGtkLauncher
{
  GtkWindow parent;

  WestonGtkLauncherPrivate *priv;
};

struct WestonGtkLauncherClass
{
  GtkWindowClass parent_class;
};

GType weston_gtk_launcher_get_type (void) G_GNUC_CONST;

GtkWidget * weston_gtk_launcher_new (GtkWidget *background_widget);

void weston_gtk_launcher_calculate (WestonGtkLauncher *self,
    gint *grid_window_width, gint *grid_window_height,
    gint *grid_cols);

#endif /* __WESTON_GTK_LAUNCHER_H__ */
