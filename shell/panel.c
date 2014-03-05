/*
 * Copyright (C) 2014 Collabora Ltd.
 *
 * Author: Jonny Lamb <jonny.lamb@collabora.co.uk>
 */

#include "config.h"

#include "panel.h"

#include "app-icon.h"
#include "favorites.h"
#include "vertical-clock.h"

enum {
  APP_MENU_TOGGLED,
  N_SIGNALS
};
static guint signals[N_SIGNALS] = { 0 };

struct MaynardPanelPrivate {
  gboolean hidden;

  GtkWidget *revealer_buttons; /* for the wifi and sound buttons */
  GtkWidget *revealer_clock; /* for the vertical clock */
};

G_DEFINE_TYPE(MaynardPanel, maynard_panel, GTK_TYPE_WINDOW)

static void
maynard_panel_init (MaynardPanel *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      MAYNARD_PANEL_TYPE,
      MaynardPanelPrivate);
}

static gboolean
widget_enter_notify_event_cb (GtkWidget *widget,
    GdkEventCrossing *event,
    MaynardPanel *self)
{
  g_signal_emit_by_name (self, "enter-notify-event", event);
}

static void
widget_connect_enter_signal (MaynardPanel *self,
    GtkWidget *widget)
{
  g_signal_connect (widget, "enter-notify-event",
      G_CALLBACK (widget_enter_notify_event_cb), self);
}

static void
app_menu_button_clicked_cb (GtkButton *button,
    MaynardPanel *self)
{
  g_signal_emit (self, signals[APP_MENU_TOGGLED], 0);
}

static void
maynard_panel_constructed (GObject *object)
{
  MaynardPanel *self = MAYNARD_PANEL (object);
  GtkWidget *main_box, *menu_box, *buttons_box;
  GtkWidget *ebox;
  GtkWidget *image;
  GtkWidget *button;

  G_OBJECT_CLASS (maynard_panel_parent_class)->constructed (object);

  /* window properties */
  gtk_window_set_title (GTK_WINDOW (self), "gtk shell");
  gtk_window_set_decorated (GTK_WINDOW (self), FALSE);
  gtk_widget_realize(GTK_WIDGET (self));

  /* make it black and slightly alpha */
  gtk_style_context_add_class (
      gtk_widget_get_style_context (GTK_WIDGET (self)),
      "wgs-panel");

  /* main vbox */
  main_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (self), main_box);

  /* for the wifi/sound buttons and vertical clock we have a few more
   * boxes. the hbox has two cells. in each cell there is a
   * GtkRevealer for hiding and showing the content. only one revealer
   * is ever visibile at one point and transitions happen at the same
   * time so the width stays constant (the animation duration is the
   * same). the first revealer contains another box which has the two
   * wifi and sound buttons. the second revealer has the vertical
   * clock widget.
   */

  /* GtkBoxes seem to eat up enter/leave events, so let's use an event
   * box for the entire thing. */
  ebox = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (main_box), ebox, FALSE, FALSE, 0);
  widget_connect_enter_signal (self, ebox);

  menu_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add (GTK_CONTAINER (ebox), menu_box);

  /* revealer for the wifi & sound buttons */
  self->priv->revealer_buttons = gtk_revealer_new ();
  gtk_revealer_set_transition_type (GTK_REVEALER (self->priv->revealer_buttons),
      GTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT);
  gtk_revealer_set_reveal_child (GTK_REVEALER (self->priv->revealer_buttons),
      TRUE);
  gtk_box_pack_start (GTK_BOX (menu_box),
      self->priv->revealer_buttons, FALSE, FALSE, 0);

  /* the box for the wifi & sound buttons */
  buttons_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (self->priv->revealer_buttons), buttons_box);

  /* wifi button */
  image = gtk_image_new_from_icon_name ("network-wireless-signal-excellent-symbolic",
      GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_style_context_add_class (gtk_widget_get_style_context (image),
      "wgs-wifi");
  gtk_box_pack_start (GTK_BOX (buttons_box), image, FALSE, FALSE, 0);

  /* sound button */
  image = gtk_image_new_from_icon_name ("audio-volume-high-symbolic",
      GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_style_context_add_class (gtk_widget_get_style_context (image),
      "wgs-audio");
  gtk_box_pack_start (GTK_BOX (buttons_box), image, FALSE, FALSE, 0);

  /* revealer for the vertical clock */
  self->priv->revealer_clock = gtk_revealer_new ();
  gtk_revealer_set_transition_type (GTK_REVEALER (self->priv->revealer_clock),
      GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT);
  gtk_revealer_set_reveal_child (GTK_REVEALER (self->priv->revealer_clock),
      FALSE);
  gtk_box_pack_start (GTK_BOX (menu_box),
      self->priv->revealer_clock, FALSE, FALSE, 0);

  /* vertical clock */
  gtk_container_add (GTK_CONTAINER (self->priv->revealer_clock),
      maynard_vertical_clock_new ());

  /* end of the menu buttons and vertical clock */

  /* favorites */
  ebox = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (main_box), ebox, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (ebox), maynard_favorites_new ());
  widget_connect_enter_signal (self, ebox);

  /* bottom app menu button */
  ebox = gtk_event_box_new ();
  gtk_box_pack_end (GTK_BOX (main_box), ebox, FALSE, FALSE, 0);
  button = maynard_app_icon_new ("view-grid-symbolic");
  g_signal_connect (button, "clicked",
      G_CALLBACK (app_menu_button_clicked_cb), self);
  gtk_container_add (GTK_CONTAINER (ebox), button);
  widget_connect_enter_signal (self, ebox);

  /* done */
  self->priv->hidden = FALSE;
}

static void
maynard_panel_class_init (MaynardPanelClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  GParamSpec *param_spec;

  object_class->constructed = maynard_panel_constructed;

  signals[APP_MENU_TOGGLED] = g_signal_new ("app-menu-toggled",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
      NULL, G_TYPE_NONE, 0);

  g_type_class_add_private (object_class, sizeof (MaynardPanelPrivate));
}

GtkWidget *
maynard_panel_new (void)
{
  return g_object_new (MAYNARD_PANEL_TYPE,
      NULL);
}

void
maynard_panel_set_expand (MaynardPanel *self,
    gboolean expand)
{
  gtk_revealer_set_reveal_child (GTK_REVEALER (self->priv->revealer_buttons), expand);
  gtk_revealer_set_reveal_child (GTK_REVEALER (self->priv->revealer_clock), !expand);
}
