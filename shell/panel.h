/*
 * Copyright (C) 2014 Collabora Ltd.
 *
 * Author: Jonny Lamb <jonny.lamb@collabora.co.uk>
 */

#ifndef __MAYNARD_PANEL_H__
#define __MAYNARD_PANEL_H__

#include <gtk/gtk.h>

#define MAYNARD_PANEL_TYPE                 (maynard_panel_get_type ())
#define MAYNARD_PANEL(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAYNARD_PANEL_TYPE, MaynardPanel))
#define MAYNARD_PANEL_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), MAYNARD_PANEL_TYPE, MaynardPanelClass))
#define MAYNARD_IS_PANEL(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MAYNARD_PANEL_TYPE))
#define MAYNARD_IS_PANEL_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), MAYNARD_PANEL_TYPE))
#define MAYNARD_PANEL_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), MAYNARD_PANEL_TYPE, MaynardPanelClass))

typedef struct MaynardPanel MaynardPanel;
typedef struct MaynardPanelClass MaynardPanelClass;
typedef struct MaynardPanelPrivate MaynardPanelPrivate;

struct MaynardPanel
{
  GtkWindow parent;

  MaynardPanelPrivate *priv;
};

struct MaynardPanelClass
{
  GtkWindowClass parent_class;
};

#define MAYNARD_PANEL_WIDTH 56
#define MAYNARD_PANEL_HEIGHT_RATIO 0.73

GType maynard_panel_get_type (void) G_GNUC_CONST;

GtkWidget * maynard_panel_new (void);

void maynard_panel_set_expand (MaynardPanel *self, gboolean expand);

#endif /* __MAYNARD_PANEL_H__ */
