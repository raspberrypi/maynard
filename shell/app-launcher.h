/*
 * Copyright (C) 2013 Collabora Ltd.
 *
 * Author: Emilio Pozuelo Monfort <emilio.pozuelo@collabora.co.uk>
 */

#ifndef __APP_LAUNCHER_H__
#define __APP_LAUNCHER_H__

#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>
#include <gtk/gtk.h>

#define APP_TYPE_LAUNCHER                 (app_launcher_get_type ())
#define APP_LAUNCHER(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), APP_TYPE_LAUNCHER, AppLauncher))
#define APP_LAUNCHER_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), APP_TYPE_LAUNCHER, AppLauncherClass))
#define APP_IS_LAUNCHER(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), APP_TYPE_LAUNCHER))
#define APP_IS_LAUNCHER_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), APP_TYPE_LAUNCHER))
#define APP_LAUNCHER_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), APP_TYPE_LAUNCHER, AppLauncherClass))

typedef struct AppLauncher AppLauncher;
typedef struct AppLauncherClass AppLauncherClass;
typedef struct AppLauncherPrivate AppLauncherPrivate;

struct AppLauncher
{
  GtkBox parent;

  AppLauncherPrivate *priv;
};

struct AppLauncherClass
{
  GtkBoxClass parent_class;
};

GType      app_launcher_get_type              (void) G_GNUC_CONST;
GtkWidget *app_launcher_new_from_desktop_info (GDesktopAppInfo *info);
void       app_launcher_activate              (AppLauncher *self);

#endif /* __APP_LAUNCHER_H__ */
