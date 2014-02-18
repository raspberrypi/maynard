/*
 * Copyright (C) 2014 Collabora Ltd.
 *
 * Author: Jonny Lamb <jonny.lamb@collabora.co.uk>
 */

#ifndef __WESTON_GTK_PANEL_H__
#define __WESTON_GTK_PANEL_H__

#include <gtk/gtk.h>

#define WESTON_GTK_PANEL_TYPE                 (weston_gtk_panel_get_type ())
#define WESTON_GTK_PANEL(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), WESTON_GTK_PANEL_TYPE, WestonGtkPanel))
#define WESTON_GTK_PANEL_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), WESTON_GTK_PANEL_TYPE, WestonGtkPanelClass))
#define WESTON_GTK_IS_PANEL(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WESTON_GTK_PANEL_TYPE))
#define WESTON_GTK_IS_PANEL_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), WESTON_GTK_PANEL_TYPE))
#define WESTON_GTK_PANEL_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), WESTON_GTK_PANEL_TYPE, WestonGtkPanelClass))

typedef struct WestonGtkPanel WestonGtkPanel;
typedef struct WestonGtkPanelClass WestonGtkPanelClass;
typedef struct WestonGtkPanelPrivate WestonGtkPanelPrivate;

struct WestonGtkPanel
{
  GtkWindow parent;

  WestonGtkPanelPrivate *priv;
};

struct WestonGtkPanelClass
{
  GtkWindowClass parent_class;
};

#define WESTON_GTK_PANEL_WIDTH 56

GType weston_gtk_panel_get_type (void) G_GNUC_CONST;

GtkWidget * weston_gtk_panel_new (void);

void weston_gtk_panel_set_expand (WestonGtkPanel *self, gboolean expand);

#endif /* __WESTON_GTK_PANEL_H__ */
