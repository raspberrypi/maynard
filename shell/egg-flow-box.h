/*
 * Copyright (C) 2010 Openismus GmbH
 * Copyright (C) 2013 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Authors:
 *      Tristan Van Berkom <tristanvb@openismus.com>
 *      Matthias Clasen <mclasen@redhat.com>
 *      William Jon McCann <jmccann@redhat.com>
 */

#ifndef __EGG_FLOW_BOX_H__
#define __EGG_FLOW_BOX_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS


#define EGG_TYPE_FLOW_BOX                  (egg_flow_box_get_type ())
#define EGG_FLOW_BOX(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), EGG_TYPE_FLOW_BOX, EggFlowBox))
#define EGG_FLOW_BOX_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), EGG_TYPE_FLOW_BOX, EggFlowBoxClass))
#define EGG_IS_FLOW_BOX(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EGG_TYPE_FLOW_BOX))
#define EGG_IS_FLOW_BOX_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), EGG_TYPE_FLOW_BOX))
#define EGG_FLOW_BOX_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), EGG_TYPE_FLOW_BOX, EggFlowBoxClass))

typedef struct _EggFlowBox            EggFlowBox;
typedef struct _EggFlowBoxPrivate     EggFlowBoxPrivate;
typedef struct _EggFlowBoxClass       EggFlowBoxClass;

/**
 * EggFlowBoxForeachFunc:
 * @flow_box: an #EggFlowBox
 * @child: The child #GtkWidget
 * @data: user data
 *
 * A function used by egg_flow_box_selected_foreach() to map all
 * selected children.  It will be called on every selected child in the box.
 */
typedef void (* EggFlowBoxForeachFunc)     (EggFlowBox      *flow_box,
                                            GtkWidget       *child,
                                            gpointer         data);

struct _EggFlowBox
{
  GtkContainer container;

  /*< private >*/
  EggFlowBoxPrivate *priv;
};

struct _EggFlowBoxClass
{
  GtkContainerClass parent_class;

  void (* child_activated) (EggFlowBox *self, GtkWidget *child);
  void (* selected_children_changed) (EggFlowBox *self);
  void (*activate_cursor_child) (EggFlowBox *self);
  void (*toggle_cursor_child) (EggFlowBox *self);
  void (*move_cursor) (EggFlowBox *self, GtkMovementStep step, gint count);
  void (*select_all) (EggFlowBox *self);
  void (*unselect_all) (EggFlowBox *self);
};

GType                 egg_flow_box_get_type                  (void) G_GNUC_CONST;

GtkWidget            *egg_flow_box_new                       (void);

void                  egg_flow_box_set_homogeneous           (EggFlowBox           *box,
                                                              gboolean              homogeneous);
gboolean              egg_flow_box_get_homogeneous           (EggFlowBox           *box);
void                  egg_flow_box_set_halign_policy         (EggFlowBox           *box,
                                                              GtkAlign              align);
GtkAlign              egg_flow_box_get_halign_policy         (EggFlowBox           *box);
void                  egg_flow_box_set_valign_policy         (EggFlowBox           *box,
                                                              GtkAlign              align);
GtkAlign              egg_flow_box_get_valign_policy         (EggFlowBox           *box);
void                  egg_flow_box_set_row_spacing           (EggFlowBox           *box,
                                                              guint                 spacing);
guint                 egg_flow_box_get_row_spacing           (EggFlowBox           *box);

void                  egg_flow_box_set_column_spacing        (EggFlowBox           *box,
                                                              guint                 spacing);
guint                 egg_flow_box_get_column_spacing        (EggFlowBox           *box);

void                  egg_flow_box_set_min_children_per_line (EggFlowBox           *box,
                                                              guint                 n_children);
guint                 egg_flow_box_get_min_children_per_line (EggFlowBox           *box);

void                  egg_flow_box_set_max_children_per_line (EggFlowBox           *box,
                                                              guint                 n_children);
guint                 egg_flow_box_get_max_children_per_line (EggFlowBox           *box);

gboolean              egg_flow_box_get_activate_on_single_click (EggFlowBox        *box);
void                  egg_flow_box_set_activate_on_single_click (EggFlowBox        *box,
                                                                 gboolean           single);

GList                *egg_flow_box_get_selected_children        (EggFlowBox        *box);
void                  egg_flow_box_selected_foreach             (EggFlowBox        *box,
                                                                 EggFlowBoxForeachFunc func,
                                                                 gpointer           data);
void                  egg_flow_box_select_child                 (EggFlowBox        *box,
                                                                 GtkWidget         *child);
void                  egg_flow_box_unselect_child               (EggFlowBox        *box,
                                                                 GtkWidget         *child);
void                  egg_flow_box_select_all                   (EggFlowBox        *box);
void                  egg_flow_box_unselect_all                 (EggFlowBox        *box);
gboolean              egg_flow_box_is_child_selected            (EggFlowBox        *box,
                                                                 GtkWidget         *child);
GtkSelectionMode      egg_flow_box_get_selection_mode           (EggFlowBox        *box);
void                  egg_flow_box_set_selection_mode           (EggFlowBox        *box,
                                                                 GtkSelectionMode   mode);
void                  egg_flow_box_set_adjustment               (EggFlowBox        *box,
                                                                 GtkAdjustment     *adjustment);

G_END_DECLS


#endif /* __EGG_FLOW_BOX_H__ */
