/*
 * Copyright (C) 2013-2014 Collabora Ltd.
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
 * Authors: Emilio Pozuelo Monfort <emilio.pozuelo@collabora.co.uk>
 *          Jonny Lamb <jonny.lamb@collabora.co.uk>
 */

#include "config.h"

#include "launcher.h"

#include "clock.h"
#include "panel.h"
#include "shell-app-system.h"

enum {
  PROP_0,
  PROP_BACKGROUND,
};

enum {
  APP_LAUNCHED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

struct MaynardLauncherPrivate {
  /* background widget so we know the output size */
  GtkWidget *background;
  ShellAppSystem *app_system;
  GtkWidget *scrolled_window;
  GtkWidget *grid;
};

G_DEFINE_TYPE(MaynardLauncher, maynard_launcher, GTK_TYPE_WINDOW)

/* each grid item is 114x114 */
#define GRID_ITEM_WIDTH 114
#define GRID_ITEM_HEIGHT 114

static void
maynard_launcher_init (MaynardLauncher *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      MAYNARD_LAUNCHER_TYPE,
      MaynardLauncherPrivate);
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

static gboolean
get_child_position_cb (GtkOverlay *overlay,
    GtkWidget *widget,
    GdkRectangle *allocation,
    gpointer user_data)
{
  GtkOverlayClass *klass = GTK_OVERLAY_GET_CLASS (overlay);

  klass->get_child_position (overlay, widget, allocation);

  /* use the same valign and halign properties, but given we have a
   * border of 1px, respect it and don't draw the overlay widget over
   * the border. */
  allocation->x += 1;
  allocation->y -= 1;
  allocation->width -= 2;

  return TRUE;
}

static gboolean
app_launched_idle_cb (gpointer data)
{
  MaynardLauncher *self = data;
  GtkAdjustment *adjustment;

  /* make the scrolled window go back to the top */

  adjustment = gtk_scrolled_window_get_vadjustment (
      GTK_SCROLLED_WINDOW (self->priv->scrolled_window));

  gtk_adjustment_set_value (adjustment, 0.0);

  return G_SOURCE_REMOVE;
}

static void
clicked_cb (GtkWidget *widget,
    GDesktopAppInfo *info)
{
  MaynardLauncher *self;

  g_app_info_launch (G_APP_INFO (info), NULL, NULL, NULL);

  self = g_object_get_data (G_OBJECT (widget), "launcher");
  g_assert (self);
  g_signal_emit (self, signals[APP_LAUNCHED], 0);

  /* do this in an idle so it's not done so obviously onscreen */
  g_idle_add (app_launched_idle_cb, self);
}

static gboolean
app_enter_cb (GtkWidget *widget,
    GdkEvent *event,
    GtkWidget *revealer)
{
  gtk_revealer_set_reveal_child (GTK_REVEALER (revealer), TRUE);
  return FALSE;
}

static gboolean
app_leave_cb (GtkWidget *widget,
    GdkEvent *event,
    GtkWidget *revealer)
{
  gtk_revealer_set_reveal_child (GTK_REVEALER (revealer), FALSE);
  return FALSE;
}

static GtkWidget *
app_launcher_new_from_desktop_info (MaynardLauncher *self,
    GDesktopAppInfo *info)
{
  GIcon *icon;
  GtkWidget *alignment;
  GtkWidget *image;
  GtkWidget *button;
  GtkWidget *overlay;
  GtkWidget *revealer;
  GtkWidget *label;
  GtkWidget *ebox;

  /* we need an ebox to catch enter and leave events */
  ebox = gtk_event_box_new ();
  gtk_style_context_add_class (gtk_widget_get_style_context (ebox),
      "maynard-grid-item");

  /* we use an overlay so we can have the app icon showing but use a
   * GtkRevealer to show a label of the app's name. */
  overlay = gtk_overlay_new ();
  gtk_container_add (GTK_CONTAINER (ebox), overlay);

  /* ...but each item has a border of 1px and we don't want the
   * revealer to paint into the border, so overload this function to
   * know where to put it. */
  g_signal_connect (overlay, "get-child-position",
      G_CALLBACK (get_child_position_cb), NULL);

  revealer = gtk_revealer_new ();
  g_object_set (revealer,
      "halign", GTK_ALIGN_FILL, /* all the width */
      "valign", GTK_ALIGN_END, /* only at the bottom */
      NULL);
  gtk_revealer_set_transition_type (GTK_REVEALER (revealer),
      GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP);
  gtk_revealer_set_reveal_child (GTK_REVEALER (revealer), FALSE);
  gtk_overlay_add_overlay (GTK_OVERLAY (overlay), revealer);

  /* app name */
  label = gtk_label_new (g_app_info_get_display_name (G_APP_INFO (info)));
  gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
  gtk_style_context_add_class (gtk_widget_get_style_context (label), "maynard-grid-label");
  gtk_container_add (GTK_CONTAINER (revealer), label);

  /* icon button to load the app */
  alignment = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_container_add (GTK_CONTAINER (overlay), alignment);

  icon = g_app_info_get_icon (G_APP_INFO (info));
  image = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_DIALOG);
  button = gtk_button_new ();
  gtk_style_context_remove_class (
      gtk_widget_get_style_context (button),
      "button");
  gtk_style_context_remove_class (
      gtk_widget_get_style_context (button),
      "image-button");
  gtk_button_set_image (GTK_BUTTON (button), image);
  g_object_set (image,
      "margin", 30,
      NULL);
  gtk_container_add (GTK_CONTAINER (alignment), button);

  /* TODO: a bit ugly */
  g_object_set_data (G_OBJECT (button), "launcher", self);
  g_signal_connect (button, "clicked", G_CALLBACK (clicked_cb), info);

  /* now we have set everything up, we can refernce the ebox and the
   * revealer. enter will show the label and leave will hide the label. */
  g_signal_connect (ebox, "enter-notify-event", G_CALLBACK (app_enter_cb), revealer);
  g_signal_connect (ebox, "leave-notify-event", G_CALLBACK (app_leave_cb), revealer);

  return ebox;
}

static void
installed_changed_cb (ShellAppSystem *app_system,
    MaynardLauncher *self)
{
  GHashTable *entries = shell_app_system_get_entries (app_system);
  GList *l, *values;

  gint output_width, output_height;
  guint cols;
  guint left, top;

  /* remove all children first */
  gtk_container_foreach (GTK_CONTAINER (self->priv->grid),
      (GtkCallback) gtk_widget_destroy, NULL);

  values = g_hash_table_get_values (entries);
  values = g_list_sort (values, sort_apps);

  maynard_launcher_calculate (self, NULL, NULL, &cols);
  cols--; /* because we start from zero here */

  left = top = 0;
  for (l = values; l; l = l->next)
    {
      GDesktopAppInfo *info = G_DESKTOP_APP_INFO (l->data);
      GtkWidget *app = app_launcher_new_from_desktop_info (self, info);

      gtk_grid_attach (GTK_GRID (self->priv->grid), app, left++, top, 1, 1);

      if (left > cols)
        {
          left = 0;
          top++;
        }
    }

  g_list_free (values);

  gtk_widget_show_all (self->priv->grid);
}

static void
background_size_allocate_cb (GtkWidget *widget,
    GdkRectangle *allocation,
    MaynardLauncher *self)
{
  installed_changed_cb (self->priv->app_system, self);
}

static void
maynard_launcher_constructed (GObject *object)
{
  MaynardLauncher *self = MAYNARD_LAUNCHER (object);

  G_OBJECT_CLASS (maynard_launcher_parent_class)->constructed (object);

  /* window properties */
  gtk_window_set_title (GTK_WINDOW (self), "maynard");
  gtk_window_set_decorated (GTK_WINDOW (self), FALSE);
  gtk_widget_realize (GTK_WIDGET (self));

  /* make it black and slightly alpha */
  gtk_style_context_add_class (
      gtk_widget_get_style_context (GTK_WIDGET (self)),
      "maynard-grid");

  /* scroll it */
  self->priv->scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (self), self->priv->scrolled_window);

  /* main grid for apps */
  self->priv->grid = gtk_grid_new ();
  gtk_container_add (GTK_CONTAINER (self->priv->scrolled_window),
      self->priv->grid);

  /* fill the grid with apps */
  self->priv->app_system = shell_app_system_get_default ();
  g_signal_connect (self->priv->app_system, "installed-changed",
      G_CALLBACK (installed_changed_cb), self);

  /* refill the grid if the background is changed */
  g_assert (self->priv->background != NULL);
  g_signal_connect (self->priv->background, "size-allocate",
      G_CALLBACK (background_size_allocate_cb), self);

  /* now actually fill the grid */
  installed_changed_cb (self->priv->app_system, self);
}

static void
maynard_launcher_get_property (GObject *object,
    guint param_id,
    GValue *value,
    GParamSpec *pspec)
{
  MaynardLauncher *self = MAYNARD_LAUNCHER (object);

  switch (param_id)
    {
      case PROP_BACKGROUND:
        g_value_set_object (value, self->priv->background);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
        break;
    }
}

static void
maynard_launcher_set_property (GObject *object,
    guint param_id,
    const GValue *value,
    GParamSpec *pspec)
{
  MaynardLauncher *self = MAYNARD_LAUNCHER (object);

  switch (param_id)
    {
      case PROP_BACKGROUND:
        self->priv->background = g_value_get_object (value);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
        break;
    }
}

static void
maynard_launcher_class_init (MaynardLauncherClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;

  object_class->constructed = maynard_launcher_constructed;
  object_class->get_property = maynard_launcher_get_property;
  object_class->set_property = maynard_launcher_set_property;

  g_object_class_install_property (object_class, PROP_BACKGROUND,
      g_param_spec_object ("background",
          "background",
          "The #GtkWidget of the background of the output",
          GTK_TYPE_WIDGET,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  signals[APP_LAUNCHED] = g_signal_new ("app-launched",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);

  g_type_class_add_private (object_class, sizeof (MaynardLauncherPrivate));
}

GtkWidget *
maynard_launcher_new (GtkWidget *background_widget)
{
  return g_object_new (MAYNARD_LAUNCHER_TYPE,
      "background", background_widget,
      NULL);
}

void
maynard_launcher_calculate (MaynardLauncher *self,
    gint *grid_window_width,
    gint *grid_window_height,
    gint *grid_cols)
{
  gint output_width, output_height, panel_height;
  gint usable_width, usable_height;
  guint cols, rows;
  guint num_apps;
  guint scrollbar_width = 13;

  gtk_widget_get_size_request (self->priv->background,
      &output_width, &output_height);
  panel_height = output_height * MAYNARD_PANEL_HEIGHT_RATIO;

  usable_width = output_width - MAYNARD_PANEL_WIDTH - scrollbar_width;
  /* don't go further down than the panel */
  usable_height = panel_height - MAYNARD_CLOCK_HEIGHT;

  /* try and fill half the screen, otherwise round down */
  cols = (int) ((usable_width / 2.0) / GRID_ITEM_WIDTH);
  /* try to fit as many rows as possible in the panel height we have */
  rows = (int) (usable_height / GRID_ITEM_HEIGHT);

  /* we don't need to include the scrollbar if we already have enough
   * space for all the apps. */
  num_apps = g_hash_table_size (
      shell_app_system_get_entries (self->priv->app_system));
  if ((cols * rows) >= num_apps)
    scrollbar_width = 0;

  /* done! */

  if (grid_window_width)
    *grid_window_width = (cols * GRID_ITEM_WIDTH) + scrollbar_width;

  if (grid_window_height)
    *grid_window_height = (rows * GRID_ITEM_HEIGHT);

  if (grid_cols)
    *grid_cols = cols;
}
