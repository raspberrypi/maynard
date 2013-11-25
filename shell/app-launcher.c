/*
 * Copyright (C) 2013 Collabora Ltd.
 *
 * Author: Emilio Pozuelo Monfort <emilio.pozuelo@collabora.co.uk>
 */

#include "config.h"

#include "app-launcher.h"

#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>
#include <gtk/gtk.h>

enum {
  PROP_0,
  PROP_INFO,
};

struct AppLauncherPrivate {
  GDesktopAppInfo *info;
};

G_DEFINE_TYPE(AppLauncher, app_launcher, GTK_TYPE_BOX)

static void
get_property (GObject *object,
    guint param_id,
    GValue *value,
    GParamSpec *pspec)
{
  AppLauncher *self = APP_LAUNCHER (object);

  switch (param_id)
    {
      case PROP_INFO:
        g_value_set_object (value, self->priv->info);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
        break;
    }
}

static void
set_property (GObject *object,
    guint param_id,
    const GValue *value,
    GParamSpec *pspec)
{
  AppLauncher *self = APP_LAUNCHER (object);

  switch (param_id)
    {
      case PROP_INFO:
        self->priv->info = g_value_dup_object (value);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
        break;
    }
}

static void
app_launcher_dispose (GObject *object)
{
  AppLauncher *self = APP_LAUNCHER (object);

  g_clear_object (&self->priv->info);

  G_OBJECT_CLASS (app_launcher_parent_class)->dispose (object);
}

static void
app_launcher_init (AppLauncher *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                            APP_TYPE_LAUNCHER,
                                            AppLauncherPrivate);

  gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_VERTICAL);
}

static void
app_launcher_constructed (GObject *object)
{
  AppLauncher *self = APP_LAUNCHER (object);
  GtkWidget *label;
  GtkWidget *image;
  GIcon *icon;

  G_OBJECT_CLASS (app_launcher_parent_class)->constructed (object);

  icon = g_app_info_get_icon (G_APP_INFO (self->priv->info));
  image = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_box_pack_start (GTK_BOX (self), image, FALSE, FALSE, 6);

  label = gtk_label_new (g_app_info_get_display_name (G_APP_INFO (self->priv->info)));
  gtk_box_pack_start (GTK_BOX (self), label, FALSE, FALSE, 6);
}

static void
app_launcher_class_init (AppLauncherClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  object_class->constructed = app_launcher_constructed;
  object_class->dispose = app_launcher_dispose;
  object_class->get_property = get_property;
  object_class->set_property = set_property;

  g_object_class_install_property (object_class, PROP_INFO,
      g_param_spec_object ("info",
          "Info",
          "The #GDesktopAppInfo that is being displayed",
          G_TYPE_DESKTOP_APP_INFO,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (object_class, sizeof (AppLauncherPrivate));
}

GtkWidget *
app_launcher_new_from_desktop_info (GDesktopAppInfo *info)
{
  return g_object_new (APP_TYPE_LAUNCHER,
                       "info", info,
                       NULL);
}

void
app_launcher_activate (AppLauncher *self)
{
  GError *error = NULL;

  g_app_info_launch (G_APP_INFO (self->priv->info), NULL, NULL, &error);
  if (error)
    {
      g_warning ("Could not launch app %s: %s",
          g_app_info_get_name (G_APP_INFO (self->priv->info)),
          error->message);
      g_clear_error (&error);
    }
}
