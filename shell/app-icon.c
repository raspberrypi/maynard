/*
 * Copyright (C) 2014 Collabora Ltd.
 *
 * Author: Jonny Lamb <jonny.lamb@collabora.co.uk>
 */

#include "config.h"

#include "app-icon.h"

G_DEFINE_TYPE(MaynardAppIcon, maynard_app_icon, GTK_TYPE_BUTTON)

static void
maynard_app_icon_init (MaynardAppIcon *self)
{
  gtk_style_context_add_class (
      gtk_widget_get_style_context (GTK_WIDGET (self)),
      "wgs-apps");

  gtk_style_context_remove_class (
      gtk_widget_get_style_context (GTK_WIDGET (self)),
      "button");
  gtk_style_context_remove_class (
      gtk_widget_get_style_context (GTK_WIDGET (self)),
      "image-button");

  gtk_button_set_relief (GTK_BUTTON (self), GTK_RELIEF_NONE);
}

static void
maynard_app_icon_class_init (MaynardAppIconClass *klass)
{
}

GtkWidget *
maynard_app_icon_new (const gchar *icon_name)
{
  GtkWidget *widget;
  GtkWidget *image;

  image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_DIALOG);
  widget = g_object_new (MAYNARD_APP_ICON_TYPE,
      "image", image,
      NULL);

  return widget;
}

GtkWidget *
maynard_app_icon_new_from_gicon (GIcon *icon)
{
  GtkWidget *widget, *image;

  image = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_DIALOG);

  widget = g_object_new (MAYNARD_APP_ICON_TYPE,
      "image", image,
      NULL);

  return widget;
}
