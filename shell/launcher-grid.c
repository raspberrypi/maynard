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

static gint
sort_apps (gconstpointer a,
    gconstpointer b)
{
  GAppInfo *info1 = G_APP_INFO (a);
  GAppInfo *info2 = G_APP_INFO (b);
  gchar *s1, *s2;
  gint ret;

  s1 = g_utf8_casefold (g_app_info_get_display_name (info1), -1);
  s2 = g_utf8_casefold (g_app_info_get_display_name (info2), -1);

  ret = g_strcmp0 (s1, s2);

  g_free (s1);
  g_free (s2);

  return ret;
}

static void
installed_changed_cb (ShellAppSystem *app_system, GtkWidget *grid)
{
  GHashTable *entries = shell_app_system_get_entries (app_system);
  GList *l, *values;

  /* Remove all children first */
  gtk_container_foreach (GTK_CONTAINER (grid), destroy_widget, NULL);

  values = g_hash_table_get_values (entries);
  values = g_list_sort (values, sort_apps);

  for (l = values; l; l = l->next)
    {
      GDesktopAppInfo *info = G_DESKTOP_APP_INFO (l->data);
      GtkWidget *app = app_launcher_new_from_desktop_info (info);

      gtk_container_add (GTK_CONTAINER (grid), app);
    }

  g_list_free (values);

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
