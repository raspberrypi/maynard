/*
 * Copyright (C) 2013 Collabora Ltd.
 *
 * Author: Emilio Pozuelo Monfort <emilio.pozuelo@collabora.co.uk>
 */

#include "launcher-grid.h"

#include <gtk/gtk.h>

#include "app-launcher.h"
#include "shell-app-system.h"
#include "egg-flow-box.h"

static void
child_activated_cb (GtkWidget *grid,
    GtkWidget *widget,
    GtkWidget *scrolled_window)
{
  AppLauncher *app = APP_LAUNCHER (widget);

  app_launcher_activate (app);

  /* A menu item has been activated so let's hide the menu */
  gtk_widget_hide (gtk_widget_get_parent (scrolled_window));
}

static void
destroy_widget (GtkWidget *widget,
    gpointer data)
{
  gtk_widget_destroy (widget);
}

static void
installed_changed_cb (ShellAppSystem *app_system, GtkWidget *grid)
{
  GHashTable *entries = shell_app_system_get_entries (app_system);
  GHashTableIter iter;
  gpointer key, value;

  /* Remove all children first */
  gtk_container_foreach (GTK_CONTAINER (grid), destroy_widget, NULL);

  g_hash_table_iter_init (&iter, entries);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      GDesktopAppInfo *info = G_DESKTOP_APP_INFO (value);
      GtkWidget *app = app_launcher_new_from_desktop_info (info);

      gtk_container_add (GTK_CONTAINER (grid), app);
    }

gtk_widget_show_all (grid);
}

GtkWidget *
launcher_grid_new (void)
{
  GtkWidget *scrolled_window;
  GtkWidget *grid;
  ShellAppSystem *app_system;

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);

  grid = egg_flow_box_new ();
  egg_flow_box_set_homogeneous (EGG_FLOW_BOX (grid), TRUE);
  g_signal_connect (grid, "child-activated",
      G_CALLBACK (child_activated_cb), scrolled_window);

  gtk_container_add (GTK_CONTAINER (scrolled_window), grid);

  app_system = shell_app_system_get_default ();
  g_signal_connect (app_system, "installed-changed",
      G_CALLBACK (installed_changed_cb), grid);
  installed_changed_cb (app_system, grid);

  gtk_widget_show_all (scrolled_window);

  return scrolled_window;
}
