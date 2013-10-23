/*
 * Copyright (C) 2013 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __EGG_FLOW_BOX_ACCESSIBLE_H__
#define __EGG_FLOW_BOX_ACCESSIBLE_H__

#include <gtk/gtk-a11y.h>

G_BEGIN_DECLS

#define EGG_TYPE_FLOW_BOX_ACCESSIBLE                   (egg_flow_box_accessible_get_type ())
#define EGG_FLOW_BOX_ACCESSIBLE(obj)                   (G_TYPE_CHECK_INSTANCE_CAST ((obj), EGG_TYPE_FLOW_BOX_ACCESSIBLE, EggFlowBoxAccessible))
#define EGG_FLOW_BOX_ACCESSIBLE_CLASS(klass)           (G_TYPE_CHECK_CLASS_CAST ((klass), EGG_TYPE_FLOW_BOX_ACCESSIBLE, EggFlowBoxAccessibleClass))
#define EGG_IS_FLOW_BOX_ACCESSIBLE(obj)                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EGG_TYPE_FLOW_BOX_ACCESSIBLE))
#define EGG_IS_FLOW_BOX_ACCESSIBLE_CLASS(klass)        (G_TYPE_CHECK_CLASS_TYPE ((klass), EGG_TYPE_FLOW_BOX_ACCESSIBLE))
#define EGG_FLOW_BOX_ACCESSIBLE_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), EGG_TYPE_FLOW_BOX_ACCESSIBLE, EggFlowBoxAccessibleClass))

typedef struct _EggFlowBoxAccessible        EggFlowBoxAccessible;
typedef struct _EggFlowBoxAccessibleClass   EggFlowBoxAccessibleClass;
typedef struct _EggFlowBoxAccessiblePrivate EggFlowBoxAccessiblePrivate;

struct _EggFlowBoxAccessible
{
  GtkContainerAccessible parent;

  EggFlowBoxAccessiblePrivate *priv;
};

struct _EggFlowBoxAccessibleClass
{
  GtkContainerAccessibleClass parent_class;
};

GType egg_flow_box_accessible_get_type (void);

void _egg_flow_box_accessible_selection_changed (GtkWidget *box);
void _egg_flow_box_accessible_update_cursor     (GtkWidget *box, GtkWidget *child);

G_END_DECLS

#endif /* __EGG_FLOW_BOX_ACCESSIBLE_H__ */
