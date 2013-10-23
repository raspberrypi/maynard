/*
 * Copyright (C) 2007-2010 Openismus GmbH
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

/**
 * SECTION:eggflowbox
 * @Short_Description: A container that allows reflowing its children
 * @Title: EggFlowBox
 *
 * #EggFlowBox positions child widgets in sequence according to its
 * orientation. For instance, with the horizontal orientation, the widgets
 * will be arranged from left to right, starting a new row under the
 * previous row when necessary. Reducing the width in this case will
 * require more rows, so a larger height will be requested.
 *
 * Likewise, with the vertical orientation, the widgets will be arranged
 * from top to bottom, starting a new column to the right when necessary.
 * Reducing the height will require more columns, so a larger width will be
 * requested.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include "egg-flow-box.h"
#include "egg-flow-box-accessible.h"

/* This already exists in gtk as _gtk_marshal_VOID__ENUM_INT, inline it here for now
   to avoid separate marshallers file */
static void
_egg_marshal_VOID__ENUM_INT (GClosure * closure,
                             GValue * return_value,
                             guint n_param_values,
                             const GValue * param_values,
                             gpointer invocation_hint,
                             gpointer marshal_data)
{
  typedef void (*GMarshalFunc_VOID__ENUM_INT) (gpointer data1, gint arg_1, gint arg_2, gpointer data2);
  register GMarshalFunc_VOID__ENUM_INT callback;
  register GCClosure * cc;
  register gpointer data1;
  register gpointer data2;
  cc = (GCClosure *) closure;
  g_return_if_fail (n_param_values == 3);
  if (G_CCLOSURE_SWAP_DATA (closure)) {
    data1 = closure->data;
    data2 = param_values->data[0].v_pointer;
  } else {
    data1 = param_values->data[0].v_pointer;
    data2 = closure->data;
  }
  callback = (GMarshalFunc_VOID__ENUM_INT) (marshal_data ? marshal_data : cc->callback);
  callback (data1, g_value_get_enum (param_values + 1), g_value_get_int (param_values + 2), data2);
}

#define P_(msgid) (msgid)

#define DEFAULT_MAX_CHILDREN_PER_LINE 7

enum {
  CHILD_ACTIVATED,
  SELECTED_CHILDREN_CHANGED,
  ACTIVATE_CURSOR_CHILD,
  TOGGLE_CURSOR_CHILD,
  MOVE_CURSOR,
  SELECT_ALL,
  UNSELECT_ALL,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_ORIENTATION,
  PROP_HOMOGENEOUS,
  PROP_HALIGN_POLICY,
  PROP_VALIGN_POLICY,
  PROP_COLUMN_SPACING,
  PROP_ROW_SPACING,
  PROP_MIN_CHILDREN_PER_LINE,
  PROP_MAX_CHILDREN_PER_LINE,
  PROP_SELECTION_MODE,
  PROP_ACTIVATE_ON_SINGLE_CLICK
};

typedef struct _EggFlowBoxChildInfo EggFlowBoxChildInfo;

struct _EggFlowBoxPrivate {
  GtkOrientation orientation;
  GtkAlign       halign_policy;
  GtkAlign       valign_policy;
  guint          homogeneous : 1;
  guint          activate_on_single_click : 1;
  GtkSelectionMode selection_mode;
  GtkAdjustment *adjustment;

  guint          row_spacing;
  guint          column_spacing;

  gboolean       active_child_active;
  EggFlowBoxChildInfo *active_child;
  EggFlowBoxChildInfo *prelight_child;
  EggFlowBoxChildInfo *cursor_child;
  EggFlowBoxChildInfo *selected_child;

  guint16        min_children_per_line;
  guint16        max_children_per_line;
  guint16        cur_children_per_line;

  GSequence     *children;
  GHashTable    *child_hash;
};

struct _EggFlowBoxChildInfo
{
  GSequenceIter *iter;
  GtkWidget *widget;
  guint selected : 1;
  GdkRectangle area;
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_CODE (EggFlowBox, egg_flow_box, GTK_TYPE_CONTAINER,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_ORIENTABLE, NULL))


#define ORIENTATION_ALIGN_POLICY(box)                                   \
  (((EggFlowBox *)(box))->priv->orientation == GTK_ORIENTATION_HORIZONTAL ? \
   ((EggFlowBox *)(box))->priv->halign_policy :                         \
   ((EggFlowBox *)(box))->priv->valign_policy)

#define OPPOSING_ORIENTATION_ALIGN_POLICY(box)                          \
  (((EggFlowBox *)(box))->priv->orientation == GTK_ORIENTATION_HORIZONTAL ? \
   ((EggFlowBox *)(box))->priv->valign_policy :                         \
   ((EggFlowBox *)(box))->priv->halign_policy)

static EggFlowBoxChildInfo*
egg_flow_box_child_info_new (GtkWidget *widget)
{
  EggFlowBoxChildInfo *info;

  info = g_new0 (EggFlowBoxChildInfo, 1);
  info->widget = g_object_ref (widget);
  return info;
}

static void
egg_flow_box_child_info_free (EggFlowBoxChildInfo *info)
{
  g_clear_object (&info->widget);
  g_free (info);
}

static EggFlowBoxChildInfo*
egg_flow_box_lookup_info (EggFlowBox *flow_box, GtkWidget* child)
{
  EggFlowBoxPrivate *priv = flow_box->priv;

  return g_hash_table_lookup (priv->child_hash, child);
}

void
egg_flow_box_set_adjustment (EggFlowBox    *box,
                             GtkAdjustment *adjustment)
{
  EggFlowBoxPrivate *priv = box->priv;

  g_return_if_fail (box != NULL);

  g_object_ref (adjustment);
  g_clear_object (&priv->adjustment);
  priv->adjustment = adjustment;
  gtk_container_set_focus_vadjustment (GTK_CONTAINER (box),
                                       adjustment);
}

/**
 * egg_flow_box_get_homogeneous:
 * @box: a #EggFlowBox
 *
 * Returns whether the box is homogeneous (all children are the
 * same size). See gtk_box_set_homogeneous().
 *
 * Return value: %TRUE if the box is homogeneous.
 **/
gboolean
egg_flow_box_get_homogeneous (EggFlowBox *box)
{
  g_return_val_if_fail (GTK_IS_BOX (box), FALSE);

  return box->priv->homogeneous;
}

/**
 * egg_flow_box_set_homogeneous:
 * @box: a #EggFlowBox
 * @homogeneous: a boolean value, %TRUE to create equal allotments,
 *   %FALSE for variable allotments
 *
 * Sets the #EggFlowBox:homogeneous property of @box, controlling
 * whether or not all children of @box are given equal space
 * in the box.
 */
void
egg_flow_box_set_homogeneous (EggFlowBox *box,
                              gboolean    homogeneous)
{
  EggFlowBoxPrivate *priv;

  g_return_if_fail (EGG_IS_FLOW_BOX (box));

  priv = box->priv;

  if ((homogeneous ? TRUE : FALSE) != priv->homogeneous)
    {
      priv->homogeneous = homogeneous ? TRUE : FALSE;
      g_object_notify (G_OBJECT (box), "homogeneous");
      gtk_widget_queue_resize (GTK_WIDGET (box));
    }
}

/* Children are visible if they are shown by the app (visible)
   and not filtered out (child_visible) by the box */
static gboolean
child_is_visible (GtkWidget *child)
{
  return gtk_widget_get_visible (child) && gtk_widget_get_child_visible (child);
}

static gint
get_visible_children (EggFlowBox *box)
{
  EggFlowBoxPrivate *priv = box->priv;
  GSequenceIter *iter;
  gint i = 0;

  for (iter = g_sequence_get_begin_iter (priv->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      EggFlowBoxChildInfo *child_info;
      GtkWidget *child;

      child_info = g_sequence_get (iter);
      child = child_info->widget;

      if (!child_is_visible (child))
        continue;

      i++;
    }

  return i;
}

/* Used in columned modes where all items share at least their
 * equal widths or heights
 */
static void
get_average_item_size (EggFlowBox    *box,
                       GtkOrientation orientation,
                       gint          *min_size,
                       gint          *nat_size)
{
  EggFlowBoxPrivate *priv = box->priv;
  GSequenceIter *iter;
  gint max_min_size = 0;
  gint max_nat_size = 0;

  for (iter = g_sequence_get_begin_iter (priv->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      EggFlowBoxChildInfo *child_info;
      GtkWidget *child;
      gint child_min, child_nat;

      child_info = g_sequence_get (iter);
      child = child_info->widget;

      if (!child_is_visible (child))
        continue;

      if (orientation == GTK_ORIENTATION_HORIZONTAL)
        gtk_widget_get_preferred_width (child, &child_min, &child_nat);
      else
        gtk_widget_get_preferred_height (child, &child_min, &child_nat);

      max_min_size = MAX (max_min_size, child_min);
      max_nat_size = MAX (max_nat_size, child_nat);
    }

  if (min_size)
    *min_size = max_min_size;

  if (nat_size)
    *nat_size = max_nat_size;
}


/* Gets the largest minimum/natural size for a given size
 * (used to get the largest item heights for a fixed item width and the opposite) */
static void
get_largest_size_for_opposing_orientation (EggFlowBox    *box,
                                           GtkOrientation orientation,
                                           gint           item_size,
                                           gint          *min_item_size,
                                           gint          *nat_item_size)
{
  EggFlowBoxPrivate *priv = box->priv;
  GSequenceIter *iter;
  gint max_min_size = 0;
  gint max_nat_size = 0;

  for (iter = g_sequence_get_begin_iter (priv->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      EggFlowBoxChildInfo *child_info;
      GtkWidget *child;
      gint       child_min, child_nat;

      child_info = g_sequence_get (iter);
      child = child_info->widget;

      if (!child_is_visible (child))
        continue;

      if (orientation == GTK_ORIENTATION_HORIZONTAL)
        gtk_widget_get_preferred_height_for_width (child,
                                                   item_size,
                                                   &child_min, &child_nat);
      else
        gtk_widget_get_preferred_width_for_height (child,
                                                   item_size,
                                                   &child_min, &child_nat);

      max_min_size = MAX (max_min_size, child_min);
      max_nat_size = MAX (max_nat_size, child_nat);
    }

  if (min_item_size)
    *min_item_size = max_min_size;

  if (nat_item_size)
    *nat_item_size = max_nat_size;
}


/* Gets the largest minimum/natural size on a single line for a given size
 * (used to get the largest line heights for a fixed item width and the opposite
 * while itterating over a list of children, note the new index is returned) */
static GSequenceIter *
get_largest_size_for_line_in_opposing_orientation (EggFlowBox       *box,
                                                   GtkOrientation    orientation,
                                                   GSequenceIter    *cursor,
                                                   gint              line_length,
                                                   GtkRequestedSize *item_sizes,
                                                   gint              extra_pixels,
                                                   gint             *min_item_size,
                                                   gint             *nat_item_size)
{
  GSequenceIter *iter;
  gint max_min_size = 0;
  gint max_nat_size = 0;
  gint i;

  i = 0;
  for (iter = cursor;
       !g_sequence_iter_is_end (iter) && i < line_length;
       iter = g_sequence_iter_next (iter))
    {
      GtkWidget *child;
      EggFlowBoxChildInfo *child_info;
      gint child_min, child_nat, this_item_size;

      child_info = g_sequence_get (iter);
      child = child_info->widget;

      if (!child_is_visible (child))
        continue;

      /* Distribute the extra pixels to the first children in the line
       * (could be fancier and spread them out more evenly) */
      this_item_size = item_sizes[i].minimum_size;
      if (extra_pixels > 0 && ORIENTATION_ALIGN_POLICY (box) == GTK_ALIGN_FILL)
        {
          this_item_size++;
          extra_pixels--;
        }

      if (orientation == GTK_ORIENTATION_HORIZONTAL)
        gtk_widget_get_preferred_height_for_width (child,
                                                   this_item_size,
                                                   &child_min, &child_nat);
      else
        gtk_widget_get_preferred_width_for_height (child,
                                                   this_item_size,
                                                   &child_min, &child_nat);

      max_min_size = MAX (max_min_size, child_min);
      max_nat_size = MAX (max_nat_size, child_nat);

      i++;
    }

  if (min_item_size)
    *min_item_size = max_min_size;

  if (nat_item_size)
    *nat_item_size = max_nat_size;

  /* Return next item in the list */
  return iter;
}

/* fit_aligned_item_requests() helper */
static gint
gather_aligned_item_requests (EggFlowBox       *box,
                              GtkOrientation    orientation,
                              gint              line_length,
                              gint              item_spacing,
                              gint              n_children,
                              GtkRequestedSize *item_sizes)
{
  EggFlowBoxPrivate *priv   = box->priv;
  GSequenceIter *iter;
  gint i;
  gint extra_items, natural_line_size = 0;

  extra_items = n_children % line_length;

  i = 0;
  for (iter = g_sequence_get_begin_iter (priv->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter), i++)
    {
      EggFlowBoxChildInfo *child_info;
      GtkWidget *child;
      GtkAlign item_align;
      gint child_min, child_nat;
      gint position;

      child_info = g_sequence_get (iter);
      child = child_info->widget;

      if (!child_is_visible (child))
        continue;

      if (orientation == GTK_ORIENTATION_HORIZONTAL)
        gtk_widget_get_preferred_width (child,
                                        &child_min, &child_nat);
      else
        gtk_widget_get_preferred_height (child,
                                         &child_min, &child_nat);

      /* Get the index and push it over for the last line when spreading to the end */
      position = i % line_length;

      item_align = ORIENTATION_ALIGN_POLICY (box);
      if (item_align == GTK_ALIGN_END && i >= n_children - extra_items)
        position += line_length - extra_items;

      /* Round up the size of every column/row */
      item_sizes[position].minimum_size = MAX (item_sizes[position].minimum_size, child_min);
      item_sizes[position].natural_size = MAX (item_sizes[position].natural_size, child_nat);
    }

  for (i = 0; i < line_length; i++)
    natural_line_size += item_sizes[i].natural_size;

  natural_line_size += (line_length - 1) * item_spacing;

  return natural_line_size;
}

static GtkRequestedSize *
fit_aligned_item_requests (EggFlowBox     *box,
                           GtkOrientation  orientation,
                           gint            avail_size,
                           gint            item_spacing,
                           gint           *line_length, /* in-out */
                           gint            items_per_line,
                           gint            n_children)
{
  GtkRequestedSize *sizes, *try_sizes;
  gint try_line_size, try_length;

  sizes = g_new0 (GtkRequestedSize, *line_length);

  /* get the sizes for the initial guess */
  try_line_size = gather_aligned_item_requests (box,
                                                orientation,
                                                *line_length,
                                                item_spacing,
                                                n_children,
                                                sizes);

  /* Try columnizing the whole thing and adding an item to the end of the line;
   * try to fit as many columns into the available size as possible */
  for (try_length = *line_length + 1; try_line_size < avail_size; try_length++)
    {
      try_sizes = g_new0 (GtkRequestedSize, try_length);
      try_line_size = gather_aligned_item_requests (box,
                                                    orientation,
                                                    try_length,
                                                    item_spacing,
                                                    n_children,
                                                    try_sizes);

      if (try_line_size <= avail_size
          && items_per_line >= try_length)
        {
          *line_length = try_length;

          g_free (sizes);
          sizes = try_sizes;
        }
      else
        {
          /* oops, this one failed; stick to the last size that fit and then return */
          g_free (try_sizes);
          break;
        }
    }

  return sizes;
}

typedef struct {
  GArray *requested;
  gint    extra_pixels;
} AllocatedLine;

static gint
get_offset_pixels (GtkAlign align,
                   gint     pixels)
{
  gint offset;

  switch (align) {
  case GTK_ALIGN_START:
  case GTK_ALIGN_FILL:
    offset = 0;
    break;
  case GTK_ALIGN_CENTER:
    offset = pixels / 2;
    break;
  case GTK_ALIGN_END:
    offset = pixels;
    break;
  default:
    g_assert_not_reached ();
    break;
  }

  return offset;
}

static void
egg_flow_box_real_size_allocate (GtkWidget     *widget,
                                 GtkAllocation *allocation)
{
  EggFlowBox *box = EGG_FLOW_BOX (widget);
  EggFlowBoxPrivate  *priv = box->priv;
  GtkAllocation child_allocation;
  gint avail_size, avail_other_size, min_items, item_spacing, line_spacing;
  GtkAlign item_align;
  GtkAlign line_align;
  GdkWindow *window;
  GtkRequestedSize *line_sizes = NULL;
  GtkRequestedSize *item_sizes = NULL;
  gint min_item_size, nat_item_size;
  gint line_length;
  gint item_size = 0;
  gint line_size = 0, min_fixed_line_size = 0, nat_fixed_line_size = 0;
  gint line_offset, item_offset, n_children, n_lines, line_count;
  gint extra_pixels = 0, extra_per_item = 0, extra_extra = 0;
  gint extra_line_pixels = 0, extra_per_line = 0, extra_line_extra = 0;
  gint i, this_line_size;
  GSequenceIter *iter;
  GtkStyleContext *context;
  gint focus_width;
  gint focus_pad;

  child_allocation.x = 0;
  child_allocation.y = 0;
  child_allocation.width = 0;
  child_allocation.height = 0;

  gtk_widget_set_allocation (widget, allocation);
  window = gtk_widget_get_window (widget);
  if (window != NULL)
    gdk_window_move_resize (window,
                            allocation->x, allocation->y,
                            allocation->width, allocation->height);

  context = gtk_widget_get_style_context (GTK_WIDGET (box));
  gtk_style_context_get_style (context,
                               "focus-line-width", &focus_width,
                               "focus-padding", &focus_pad,
                               NULL);
  child_allocation.x = 0 + focus_width + focus_pad;
  child_allocation.y = 0;
  child_allocation.width = allocation->width - 2 * (focus_width + focus_pad);

  min_items = MAX (1, priv->min_children_per_line);

  if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      avail_size = allocation->width;
      avail_other_size = allocation->height;
      item_spacing = priv->column_spacing;
      line_spacing = priv->row_spacing;
    }
  else /* GTK_ORIENTATION_VERTICAL */
    {
      avail_size = allocation->height;
      avail_other_size = allocation->width;
      item_spacing = priv->row_spacing;
      line_spacing = priv->column_spacing;
    }

  item_align = ORIENTATION_ALIGN_POLICY (box);
  line_align = OPPOSING_ORIENTATION_ALIGN_POLICY (box);

  /* Get how many lines we'll be needing to flow */
  n_children = get_visible_children (box);
  if (n_children <= 0)
    return;

  /*
   * Deal with ALIGNED/HOMOGENEOUS modes first, start with
   * initial guesses at item/line sizes
   */
  get_average_item_size (box, priv->orientation, &min_item_size, &nat_item_size);
  if (nat_item_size <= 0)
    return;

  /* By default flow at the natural item width */
  line_length = avail_size / (nat_item_size + item_spacing);

  /* After the above aproximation, check if we cant fit one more on the line */
  if (line_length * item_spacing + (line_length + 1) * nat_item_size <= avail_size)
    line_length++;

  /* Its possible we were allocated just less than the natural width of the
   * minimum item flow length */
  line_length = MAX (min_items, line_length);
  line_length = MIN (line_length, priv->max_children_per_line);

  /* Here we just use the largest height-for-width and use that for the height
   * of all lines */
  if (priv->homogeneous)
    {
      n_lines = n_children / line_length;
      if ((n_children % line_length) > 0)
        n_lines++;

      n_lines = MAX (n_lines, 1);

      /* Now we need the real item allocation size */
      item_size = (avail_size - (line_length - 1) * item_spacing) / line_length;

      /* Cut out the expand space if we're not distributing any */
      if (item_align != GTK_ALIGN_FILL)
        item_size = MIN (item_size, nat_item_size);

      get_largest_size_for_opposing_orientation (box,
                                                 priv->orientation,
                                                 item_size,
                                                 &min_fixed_line_size,
                                                 &nat_fixed_line_size);

      /* resolve a fixed 'line_size' */
      line_size = (avail_other_size - (n_lines - 1) * line_spacing) / n_lines;

      if (line_align != GTK_ALIGN_FILL)
        line_size = MIN (line_size, nat_fixed_line_size);

      /* Get the real extra pixels incase of GTK_ALIGN_START lines */
      extra_pixels      = avail_size       - (line_length - 1) * item_spacing - item_size * line_length;
      extra_line_pixels = avail_other_size - (n_lines - 1)     * line_spacing - line_size * n_lines;
    }
  else
    {
      gboolean first_line = TRUE;

      /* Find the amount of columns that can fit aligned into the available space
       * and collect their requests.
       */
      item_sizes = fit_aligned_item_requests (box,
                                              priv->orientation,
                                              avail_size,
                                              item_spacing,
                                              &line_length,
                                              priv->max_children_per_line,
                                              n_children);

      /* Calculate the number of lines after determining the final line_length */
      n_lines = n_children / line_length;
      if ((n_children % line_length) > 0)
        n_lines++;

      n_lines = MAX (n_lines, 1);
      line_sizes = g_new0 (GtkRequestedSize, n_lines);

      /* Get the available remaining size */
      avail_size -= (line_length - 1) * item_spacing;
      for (i = 0; i < line_length; i++)
        avail_size -= item_sizes[i].minimum_size;

      /* Perform a natural allocation on the columnized items and get the remaining pixels */
      if (avail_size > 0)
        extra_pixels = gtk_distribute_natural_allocation (avail_size, line_length, item_sizes);

      /* Now that we have the size of each column of items find the size of each individual
       * line based on the aligned item sizes.
       */

      for (i = 0, iter = g_sequence_get_begin_iter (priv->children);
           !g_sequence_iter_is_end (iter);
           i++)
        {

          iter = get_largest_size_for_line_in_opposing_orientation (box,
                                                                    priv->orientation,
                                                                    iter,
                                                                    line_length,
                                                                    item_sizes,
                                                                    extra_pixels,
                                                                    &line_sizes[i].minimum_size,
                                                                    &line_sizes[i].natural_size);


          /* Its possible a line is made of completely invisible children */
          if (line_sizes[i].natural_size > 0)
            {
              if (first_line)
                first_line = FALSE;
              else
                avail_other_size -= line_spacing;

              avail_other_size -= line_sizes[i].minimum_size;

              line_sizes[i].data = GINT_TO_POINTER (i);
            }
        }

      /* Distribute space among lines naturally */
      if (avail_other_size > 0)
        extra_line_pixels = gtk_distribute_natural_allocation (avail_other_size, n_lines, line_sizes);
    }

  /*
   * Initial sizes of items/lines guessed at this point,
   * go on to distribute expand space if needed.
   */

  priv->cur_children_per_line = line_length;

  /* FIXME: This portion needs to consider which columns
   * and rows asked for expand space and distribute those
   * accordingly for the case of ALIGNED allocation.
   *
   * If at least one child in a column/row asked for expand;
   * we should make that row/column expand entirely.
   */

  /* Calculate expand space per item */
  if (item_align == GTK_ALIGN_FILL)
    {
      extra_per_item = extra_pixels / line_length;
      extra_extra    = extra_pixels % line_length;
    }

  /* Calculate expand space per line */
  if (line_align == GTK_ALIGN_FILL)
    {
      extra_per_line   = extra_line_pixels / n_lines;
      extra_line_extra = extra_line_pixels % n_lines;
    }

  /*
   * Prepare item/line initial offsets and jump into the
   * real allocation loop.
   */
  line_offset = item_offset = 0;

  /* prepend extra space to item_offset/line_offset for SPREAD_END */
  item_offset += get_offset_pixels (item_align, extra_pixels);
  line_offset += get_offset_pixels (line_align, extra_line_pixels);

  /* Get the allocation size for the first line */
  if (priv->homogeneous)
    this_line_size = line_size;
  else
    {
      this_line_size  = line_sizes[0].minimum_size;

      if (line_align == GTK_ALIGN_FILL)
        {
          this_line_size += extra_per_line;

          if (extra_line_extra > 0)
            this_line_size++;
        }
    }

  i = 0;
  line_count = 0;
  for (iter = g_sequence_get_begin_iter (priv->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      EggFlowBoxChildInfo *child_info;
      GtkWidget *child;
      gint position;
      gint this_item_size;

      child_info = g_sequence_get (iter);
      child = child_info->widget;

      if (!child_is_visible (child))
        {
          child_info->area.x = child_allocation.x;
          child_info->area.y = child_allocation.y;
          child_info->area.width = 0;
          child_info->area.height = 0;
          continue;
        }

      /* Get item position */
      position = i % line_length;

      /* adjust the line_offset/count at the beginning of each new line */
      if (i > 0 && position == 0)
        {
          /* Push the line_offset */
          line_offset += this_line_size + line_spacing;

          line_count++;

          /* Get the new line size */
          if (priv->homogeneous)
            this_line_size = line_size;
          else
            {
              this_line_size = line_sizes[line_count].minimum_size;

              if (line_align == GTK_ALIGN_FILL)
                {
                  this_line_size += extra_per_line;

                  if (line_count < extra_line_extra)
                    this_line_size++;
                }
            }

          item_offset = 0;

          if (item_align == GTK_ALIGN_CENTER)
            {
              item_offset += get_offset_pixels (item_align, extra_pixels);
            }
          else if (item_align == GTK_ALIGN_END)
            {
              item_offset += get_offset_pixels (item_align, extra_pixels);

              /* If we're on the last line, prepend the space for
               * any leading items */
              if (line_count == n_lines -1)
                {
                  gint extra_items = n_children % line_length;

                  if (priv->homogeneous)
                    {
                      item_offset += item_size * (line_length - extra_items);
                      item_offset += item_spacing * (line_length - extra_items);
                    }
                  else
                    {
                      gint j;

                      for (j = 0; j < (line_length - extra_items); j++)
                        {
                          item_offset += item_sizes[j].minimum_size;
                          item_offset += item_spacing;
                        }
                    }
                }
            }
        }

      /* Push the index along for the last line when spreading to the end */
      if (item_align == GTK_ALIGN_END && line_count == n_lines -1)
        {
          gint extra_items = n_children % line_length;

          position += line_length - extra_items;
        }

      if (priv->homogeneous)
        this_item_size = item_size;
      else
        this_item_size = item_sizes[position].minimum_size;

      if (item_align == GTK_ALIGN_FILL)
        {
          this_item_size += extra_per_item;

          if (position < extra_extra)
            this_item_size++;
        }

      /* Do the actual allocation */
      if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
        {
          child_allocation.x = item_offset;
          child_allocation.y = line_offset;
          child_allocation.width = this_item_size;
          child_allocation.height = this_line_size;
        }
      else /* GTK_ORIENTATION_VERTICAL */
        {
          child_allocation.x = line_offset;
          child_allocation.y = item_offset;
          child_allocation.width = this_line_size;
          child_allocation.height = this_item_size;
        }

      if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
        child_allocation.x = allocation->x + allocation->width - (child_allocation.x - allocation->x) - child_allocation.width;
      child_info->area.x = child_allocation.x;
      child_info->area.y = child_allocation.y;
      child_info->area.width = child_allocation.width;
      child_info->area.height = child_allocation.height;
      gtk_widget_size_allocate (child, &child_allocation);

      item_offset += this_item_size;
      item_offset += item_spacing;

      i++;
    }

  g_free (item_sizes);
  g_free (line_sizes);
}

static void
egg_flow_box_real_add (GtkContainer *container,
                       GtkWidget    *child)
{
  EggFlowBox *box = EGG_FLOW_BOX (container);
  EggFlowBoxPrivate *priv = box->priv;
  EggFlowBoxChildInfo *info;
  GSequenceIter *iter = NULL;

  g_return_if_fail (EGG_IS_FLOW_BOX (box));
  g_return_if_fail (GTK_IS_WIDGET (child));

  info = egg_flow_box_child_info_new (child);
  g_hash_table_insert (priv->child_hash, child, info);
  iter = g_sequence_append (priv->children, info);
  info->iter = iter;

  gtk_widget_set_parent (child, GTK_WIDGET (box));
}

static void
egg_flow_box_real_remove (GtkContainer *container,
                          GtkWidget    *child)
{
  EggFlowBox *box = EGG_FLOW_BOX (container);
  EggFlowBoxPrivate *priv = box->priv;
  gboolean was_visible;
  gboolean was_selected;
  EggFlowBoxChildInfo *child_info;

  g_return_if_fail (child != NULL);

  was_visible = child_is_visible (child);

  child_info = egg_flow_box_lookup_info (box, child);
  if (child_info == NULL)
    {
      g_warning ("Tried to remove non-child %p\n", child);
      return;
    }

  was_selected = child_info->selected;

  if (child_info == priv->prelight_child)
    priv->prelight_child = NULL;
  if (child_info == priv->active_child)
    priv->active_child = NULL;
  if (child_info == priv->selected_child)
    priv->selected_child = NULL;

  gtk_widget_unparent (child);
  g_hash_table_remove (priv->child_hash, child);
  g_sequence_remove (child_info->iter);

  if (was_visible && gtk_widget_get_visible (GTK_WIDGET (box)))
    gtk_widget_queue_resize (GTK_WIDGET (box));

  if (was_selected)
    g_signal_emit (box, signals[SELECTED_CHILDREN_CHANGED], 0);
}

static void
egg_flow_box_real_forall (GtkContainer *container,
                          gboolean      include_internals,
                          GtkCallback   callback,
                          gpointer      callback_target)
{
  EggFlowBox *box = EGG_FLOW_BOX (container);
  EggFlowBoxPrivate *priv   = box->priv;
  GSequenceIter *iter;
  EggFlowBoxChildInfo *child_info;

  iter = g_sequence_get_begin_iter (priv->children);
  while (!g_sequence_iter_is_end (iter))
    {
      child_info = g_sequence_get (iter);
      iter = g_sequence_iter_next (iter);
      callback (child_info->widget, callback_target);
    }
}

static GType
egg_flow_box_real_child_type (GtkContainer *container)
{
  return GTK_TYPE_WIDGET;
}

static GtkSizeRequestMode
egg_flow_box_real_get_request_mode (GtkWidget *widget)
{
  EggFlowBox *box = EGG_FLOW_BOX (widget);
  EggFlowBoxPrivate *priv   = box->priv;

  return (priv->orientation == GTK_ORIENTATION_HORIZONTAL) ?
    GTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH : GTK_SIZE_REQUEST_WIDTH_FOR_HEIGHT;
}

/* Gets the largest minimum and natural length of
 * 'line_length' consecutive items when aligned into rows/columns */
static void
get_largest_aligned_line_length (EggFlowBox     *box,
                                 GtkOrientation  orientation,
                                 gint            line_length,
                                 gint           *min_size,
                                 gint           *nat_size)
{
  EggFlowBoxPrivate *priv = box->priv;
  GSequenceIter *iter;
  gint max_min_size = 0;
  gint max_nat_size = 0;
  gint spacing, i;
  GtkRequestedSize *aligned_item_sizes;

  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    spacing = priv->column_spacing;
  else
    spacing = priv->row_spacing;

  aligned_item_sizes = g_new0 (GtkRequestedSize, line_length);

  /* Get the largest sizes of each index in the line.
   */
  i = 0;
  for (iter = g_sequence_get_begin_iter (priv->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      EggFlowBoxChildInfo *child_info;
      GtkWidget *child;
      gint child_min, child_nat;

      child_info = g_sequence_get (iter);
      child = child_info->widget;

      if (!child_is_visible (child))
        continue;

      if (orientation == GTK_ORIENTATION_HORIZONTAL)
        gtk_widget_get_preferred_width (child,
                                        &child_min, &child_nat);
      else /* GTK_ORIENTATION_VERTICAL */
        gtk_widget_get_preferred_height (child,
                                         &child_min, &child_nat);

      aligned_item_sizes[i % line_length].minimum_size =
        MAX (aligned_item_sizes[i % line_length].minimum_size, child_min);

      aligned_item_sizes[i % line_length].natural_size =
        MAX (aligned_item_sizes[i % line_length].natural_size, child_nat);

      i++;
    }

  /* Add up the largest indexes */
  for (i = 0; i < line_length; i++)
    {
      max_min_size += aligned_item_sizes[i].minimum_size;
      max_nat_size += aligned_item_sizes[i].natural_size;
    }

  g_free (aligned_item_sizes);

  max_min_size += (line_length - 1) * spacing;
  max_nat_size += (line_length - 1) * spacing;

  if (min_size)
    *min_size = max_min_size;

  if (nat_size)
    *nat_size = max_nat_size;
}


static void
egg_flow_box_real_get_preferred_width (GtkWidget *widget,
                                       gint      *minimum_size,
                                       gint      *natural_size)
{
  EggFlowBox *box  = EGG_FLOW_BOX (widget);
  EggFlowBoxPrivate *priv = box->priv;
  gint min_item_width, nat_item_width;
  gint min_items, nat_items;
  gint min_width, nat_width;

  min_items = MAX (1, priv->min_children_per_line);
  nat_items = MAX (min_items, priv->max_children_per_line);

  if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      min_width = nat_width = 0;

      if (! priv->homogeneous)
        {
          /* When not homogeneous; horizontally oriented boxes
           * need enough width for the widest row */
          if (min_items == 1)
            {
              get_average_item_size (box,
                                     GTK_ORIENTATION_HORIZONTAL,
                                     &min_item_width,
                                     &nat_item_width);

              min_width += min_item_width;
              nat_width += nat_item_width;
            }
          else
            {
              gint min_line_length, nat_line_length;

              get_largest_aligned_line_length (box,
                                               GTK_ORIENTATION_HORIZONTAL,
                                               min_items,
                                               &min_line_length,
                                               &nat_line_length);

              if (nat_items > min_items)
                get_largest_aligned_line_length (box,
                                                 GTK_ORIENTATION_HORIZONTAL,
                                                 nat_items,
                                                 NULL,
                                                 &nat_line_length);

              min_width += min_line_length;
              nat_width += nat_line_length;
            }
        }
      else /* In homogeneous mode; horizontally oriented boxs
            * give the same width to all children */
        {
          get_average_item_size (box, GTK_ORIENTATION_HORIZONTAL,
                                 &min_item_width, &nat_item_width);

          min_width += min_item_width * min_items;
          min_width += (min_items -1) * priv->column_spacing;

          nat_width += nat_item_width * nat_items;
          nat_width += (nat_items -1) * priv->column_spacing;
        }
    }
  else /* GTK_ORIENTATION_VERTICAL */
    {
      /* Return the width for the minimum height */
      gint min_height;

      GTK_WIDGET_GET_CLASS (widget)->get_preferred_height (widget, &min_height, NULL);
      GTK_WIDGET_GET_CLASS (widget)->get_preferred_width_for_height (widget,
                                                                     min_height,
                                                                     &min_width,
                                                                     &nat_width);

    }

  if (minimum_size)
    *minimum_size = min_width;

  if (natural_size)
    *natural_size = nat_width;
}

static void
egg_flow_box_real_get_preferred_height (GtkWidget *widget,
                                        gint      *minimum_size,
                                        gint      *natural_size)
{
  EggFlowBox *box  = EGG_FLOW_BOX (widget);
  EggFlowBoxPrivate *priv = box->priv;
  gint min_item_height, nat_item_height;
  gint min_items, nat_items;
  gint min_height, nat_height;

  min_items = MAX (1, priv->min_children_per_line);
  nat_items = MAX (min_items, priv->max_children_per_line);

  if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      /* Return the height for the minimum width */
      gint min_width;

      GTK_WIDGET_GET_CLASS (widget)->get_preferred_width (widget, &min_width, NULL);
      GTK_WIDGET_GET_CLASS (widget)->get_preferred_height_for_width (widget,
                                                                     min_width,
                                                                     &min_height,
                                                                     &nat_height);
    }
  else /* GTK_ORIENTATION_VERTICAL */
    {
      min_height   = nat_height = 0;

      if (! priv->homogeneous)
        {
          /* When not homogeneous; vertically oriented boxes
           * need enough height for the tallest column */
          if (min_items == 1)
            {
              get_average_item_size (box, GTK_ORIENTATION_VERTICAL,
                                     &min_item_height, &nat_item_height);

              min_height += min_item_height;
              nat_height += nat_item_height;
            }
          else
            {
              gint min_line_length, nat_line_length;

              get_largest_aligned_line_length (box,
                                               GTK_ORIENTATION_VERTICAL,
                                               min_items,
                                               &min_line_length,
                                               &nat_line_length);

              if (nat_items > min_items)
                get_largest_aligned_line_length (box,
                                                 GTK_ORIENTATION_VERTICAL,
                                                 nat_items,
                                                 NULL,
                                                 &nat_line_length);

              min_height += min_line_length;
              nat_height += nat_line_length;
            }

        }
      else /* In homogeneous mode; vertically oriented boxs
            * give the same height to all children */
        {
          get_average_item_size (box,
                                 GTK_ORIENTATION_VERTICAL,
                                 &min_item_height,
                                 &nat_item_height);

          min_height += min_item_height * min_items;
          min_height += (min_items -1) * priv->row_spacing;

          nat_height += nat_item_height * nat_items;
          nat_height += (nat_items -1) * priv->row_spacing;
        }
    }

  if (minimum_size)
    *minimum_size = min_height;

  if (natural_size)
    *natural_size = nat_height;
}

static void
egg_flow_box_real_get_preferred_height_for_width (GtkWidget *widget,
                                                  gint       width,
                                                  gint      *minimum_height,
                                                  gint      *natural_height)
{
  EggFlowBox *box = EGG_FLOW_BOX (widget);
  EggFlowBoxPrivate *priv   = box->priv;
  gint min_item_width, nat_item_width;
  gint min_items;
  gint min_height, nat_height;
  gint avail_size, n_children;

  min_items = MAX (1, priv->min_children_per_line);

  min_height = 0;
  nat_height = 0;

  if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      gint min_width;
      gint line_length;
      gint item_size, extra_pixels;

      n_children = get_visible_children (box);
      if (n_children <= 0)
        goto out;

      /* Make sure its no smaller than the minimum */
      GTK_WIDGET_GET_CLASS (widget)->get_preferred_width (widget, &min_width, NULL);

      avail_size  = MAX (width, min_width);
      if (avail_size <= 0)
        goto out;

      get_average_item_size (box, GTK_ORIENTATION_HORIZONTAL, &min_item_width, &nat_item_width);
      if (nat_item_width <= 0)
        goto out;
      /* By default flow at the natural item width */
      line_length = avail_size / (nat_item_width + priv->column_spacing);

      /* After the above aproximation, check if we cant fit one more on the line */
      if (line_length * priv->column_spacing + (line_length + 1) * nat_item_width <= avail_size)
        line_length++;

      /* Its possible we were allocated just less than the natural width of the
       * minimum item flow length */
      line_length = MAX (min_items, line_length);
      line_length = MIN (line_length, priv->max_children_per_line);

      /* Now we need the real item allocation size */
      item_size = (avail_size - (line_length - 1) * priv->column_spacing) / line_length;

      /* Cut out the expand space if we're not distributing any */
      if (priv->halign_policy != GTK_ALIGN_FILL)
        {
          item_size    = MIN (item_size, nat_item_width);
          extra_pixels = 0;
        }
      else
        /* Collect the extra pixels for expand children */
        extra_pixels = (avail_size - (line_length - 1) * priv->column_spacing) % line_length;

      if (priv->homogeneous)
        {
          gint min_item_height, nat_item_height;
          gint lines;

          /* Here we just use the largest height-for-width and
           * add up the size accordingly */
          get_largest_size_for_opposing_orientation (box,
                                                     GTK_ORIENTATION_HORIZONTAL,
                                                     item_size,
                                                     &min_item_height,
                                                     &nat_item_height);

          /* Round up how many lines we need to allocate for */
          lines = n_children / line_length;
          if ((n_children % line_length) > 0)
            lines++;

          min_height = min_item_height * lines;
          nat_height = nat_item_height * lines;

          min_height += (lines - 1) * priv->row_spacing;
          nat_height += (lines - 1) * priv->row_spacing;
        }
      else
        {
          gint min_line_height, nat_line_height, i;
          gboolean first_line = TRUE;
          GtkRequestedSize *item_sizes;
          GSequenceIter *iter;

          /* First get the size each set of items take to span the line
           * when aligning the items above and below after flowping.
           */
          item_sizes = fit_aligned_item_requests (box,
                                                  priv->orientation,
                                                  avail_size,
                                                  priv->column_spacing,
                                                  &line_length,
                                                  priv->max_children_per_line,
                                                  n_children);

          /* Get the available remaining size */
          avail_size -= (line_length - 1) * priv->column_spacing;
          for (i = 0; i < line_length; i++)
            avail_size -= item_sizes[i].minimum_size;

          if (avail_size > 0)
            extra_pixels = gtk_distribute_natural_allocation (avail_size, line_length, item_sizes);

          for (iter = g_sequence_get_begin_iter (priv->children);
               !g_sequence_iter_is_end (iter);)
            {
              iter = get_largest_size_for_line_in_opposing_orientation (box,
                                                                        GTK_ORIENTATION_HORIZONTAL,
                                                                        iter,
                                                                        line_length,
                                                                        item_sizes,
                                                                        extra_pixels,
                                                                        &min_line_height,
                                                                        &nat_line_height);
              /* Its possible the line only had invisible widgets */
              if (nat_line_height > 0)
                {
                  if (first_line)
                    first_line = FALSE;
                  else
                    {
                      min_height += priv->row_spacing;
                      nat_height += priv->row_spacing;
                    }

                  min_height += min_line_height;
                  nat_height += nat_line_height;
                }
            }

          g_free (item_sizes);
        }
    }
  else /* GTK_ORIENTATION_VERTICAL */
    {
      /* Return the minimum height */
      GTK_WIDGET_GET_CLASS (widget)->get_preferred_height (widget, &min_height, &nat_height);
    }

 out:

  if (minimum_height)
    *minimum_height = min_height;

  if (natural_height)
    *natural_height = nat_height;
}

static void
egg_flow_box_real_get_preferred_width_for_height (GtkWidget *widget,
                                                  gint       height,
                                                  gint      *minimum_width,
                                                  gint      *natural_width)
{
  EggFlowBox *box = EGG_FLOW_BOX (widget);
  EggFlowBoxPrivate *priv   = box->priv;
  gint min_item_height, nat_item_height;
  gint min_items;
  gint min_width, nat_width;
  gint avail_size, n_children;

  min_items = MAX (1, priv->min_children_per_line);

  min_width = 0;
  nat_width = 0;

  if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      /* Return the minimum width */
      GTK_WIDGET_GET_CLASS (widget)->get_preferred_width (widget, &min_width, &nat_width);
    }
  else /* GTK_ORIENTATION_VERTICAL */
    {
      gint min_height;
      gint line_length;
      gint item_size, extra_pixels;

      n_children = get_visible_children (box);
      if (n_children <= 0)
        goto out;

      /* Make sure its no smaller than the minimum */
      GTK_WIDGET_GET_CLASS (widget)->get_preferred_height (widget, &min_height, NULL);

      avail_size = MAX (height, min_height);
      if (avail_size <= 0)
        goto out;

      get_average_item_size (box, GTK_ORIENTATION_VERTICAL, &min_item_height, &nat_item_height);

      /* By default flow at the natural item width */
      line_length = avail_size / (nat_item_height + priv->row_spacing);

      /* After the above aproximation, check if we cant fit one more on the line */
      if (line_length * priv->row_spacing + (line_length + 1) * nat_item_height <= avail_size)
        line_length++;

      /* Its possible we were allocated just less than the natural width of the
       * minimum item flow length */
      line_length = MAX (min_items, line_length);
      line_length = MIN (line_length, priv->max_children_per_line);

      /* Now we need the real item allocation size */
      item_size = (avail_size - (line_length - 1) * priv->row_spacing) / line_length;

      /* Cut out the expand space if we're not distributing any */
      if (priv->valign_policy != GTK_ALIGN_FILL)
        {
          item_size    = MIN (item_size, nat_item_height);
          extra_pixels = 0;
        }
      else
        /* Collect the extra pixels for expand children */
        extra_pixels = (avail_size - (line_length - 1) * priv->row_spacing) % line_length;

      if (priv->homogeneous)
        {
          gint min_item_width, nat_item_width;
          gint lines;

          /* Here we just use the largest height-for-width and
           * add up the size accordingly */
          get_largest_size_for_opposing_orientation (box,
                                                     GTK_ORIENTATION_VERTICAL,
                                                     item_size,
                                                     &min_item_width,
                                                     &nat_item_width);

          /* Round up how many lines we need to allocate for */
          n_children = get_visible_children (box);
          lines = n_children / line_length;
          if ((n_children % line_length) > 0)
            lines++;

          min_width = min_item_width * lines;
          nat_width = nat_item_width * lines;

          min_width += (lines - 1) * priv->column_spacing;
          nat_width += (lines - 1) * priv->column_spacing;
        }
      else
        {
          gint min_line_width, nat_line_width, i;
          gboolean first_line = TRUE;
          GtkRequestedSize *item_sizes;
          GSequenceIter *iter;

          /* First get the size each set of items take to span the line
           * when aligning the items above and below after flowping.
           */
          item_sizes = fit_aligned_item_requests (box,
                                                  priv->orientation,
                                                  avail_size,
                                                  priv->row_spacing,
                                                  &line_length,
                                                  priv->max_children_per_line,
                                                  n_children);

          /* Get the available remaining size */
          avail_size -= (line_length - 1) * priv->column_spacing;
          for (i = 0; i < line_length; i++)
            avail_size -= item_sizes[i].minimum_size;

          if (avail_size > 0)
            extra_pixels = gtk_distribute_natural_allocation (avail_size, line_length, item_sizes);

          for (iter = g_sequence_get_begin_iter (priv->children);
               !g_sequence_iter_is_end (iter);)
            {
              iter = get_largest_size_for_line_in_opposing_orientation (box,
                                                                        GTK_ORIENTATION_VERTICAL,
                                                                        iter,
                                                                        line_length,
                                                                        item_sizes,
                                                                        extra_pixels,
                                                                        &min_line_width,
                                                                        &nat_line_width);

              /* Its possible the last line only had invisible widgets */
              if (nat_line_width > 0)
                {
                  if (first_line)
                    first_line = FALSE;
                  else
                    {
                      min_width += priv->column_spacing;
                      nat_width += priv->column_spacing;
                    }

                  min_width += min_line_width;
                  nat_width += nat_line_width;
                }
            }
          g_free (item_sizes);
        }
    }

 out:
  if (minimum_width)
    *minimum_width = min_width;

  if (natural_width)
    *natural_width = nat_width;
}

/**
 * egg_flow_box_set_halign_policy:
 * @box: An #EggFlowBox
 * @align: The #GtkAlign to use.
 *
 * Sets the horizontal align policy for @box's children.
 */
void
egg_flow_box_set_halign_policy (EggFlowBox *box,
                                GtkAlign    align)
{
  EggFlowBoxPrivate *priv;

  g_return_if_fail (EGG_IS_FLOW_BOX (box));

  priv = box->priv;

  if (priv->halign_policy != align)
    {
      priv->halign_policy = align;

      gtk_widget_queue_resize (GTK_WIDGET (box));

      g_object_notify (G_OBJECT (box), "halign-policy");
    }
}

/**
 * egg_flow_box_get_halign_policy:
 * @box: An #EggFlowBox
 *
 * Gets the horizontal alignment policy.
 *
 * Returns: The horizontal #GtkAlign for @box.
 */
GtkAlign
egg_flow_box_get_halign_policy (EggFlowBox *box)
{
  g_return_val_if_fail (EGG_IS_FLOW_BOX (box), FALSE);

  return box->priv->halign_policy;
}


/**
 * egg_flow_box_set_valign_policy:
 * @box: An #EggFlowBox
 * @align: The #GtkAlign to use.
 *
 * Sets the vertical alignment policy for @box's children.
 */
void
egg_flow_box_set_valign_policy (EggFlowBox *box,
                                GtkAlign    align)
{
  EggFlowBoxPrivate *priv;

  g_return_if_fail (EGG_IS_FLOW_BOX (box));

  priv = box->priv;

  if (priv->valign_policy != align)
    {
      priv->valign_policy = align;

      gtk_widget_queue_resize (GTK_WIDGET (box));

      g_object_notify (G_OBJECT (box), "valign-policy");
    }
}

/**
 * egg_flow_box_get_valign_policy:
 * @box: An #EggFlowBox
 *
 * Gets the vertical alignment policy.
 *
 * Returns: The vertical #GtkAlign for @box.
 */
GtkAlign
egg_flow_box_get_valign_policy (EggFlowBox *box)
{
  g_return_val_if_fail (EGG_IS_FLOW_BOX (box), FALSE);

  return box->priv->valign_policy;
}


/**
 * egg_flow_box_set_row_spacing:
 * @box: An #EggFlowBox
 * @spacing: The spacing to use.
 *
 * Sets the vertical space to add between children.
 */
void
egg_flow_box_set_row_spacing  (EggFlowBox *box,
                               guint       spacing)
{
  EggFlowBoxPrivate *priv;

  g_return_if_fail (EGG_IS_FLOW_BOX (box));

  priv = box->priv;

  if (priv->row_spacing != spacing)
    {
      priv->row_spacing = spacing;

      gtk_widget_queue_resize (GTK_WIDGET (box));

      g_object_notify (G_OBJECT (box), "vertical-spacing");
    }
}

/**
 * egg_flow_box_get_row_spacing:
 * @box: An #EggFlowBox
 *
 * Gets the vertical spacing.
 *
 * Returns: The vertical spacing.
 */
guint
egg_flow_box_get_row_spacing  (EggFlowBox *box)
{
  g_return_val_if_fail (EGG_IS_FLOW_BOX (box), FALSE);

  return box->priv->row_spacing;
}

/**
 * egg_flow_box_set_column_spacing:
 * @box: An #EggFlowBox
 * @spacing: The spacing to use.
 *
 * Sets the horizontal space to add between children.
 */
void
egg_flow_box_set_column_spacing (EggFlowBox    *box,
                                 guint          spacing)
{
  EggFlowBoxPrivate *priv;

  g_return_if_fail (EGG_IS_FLOW_BOX (box));

  priv = box->priv;

  if (priv->column_spacing != spacing)
    {
      priv->column_spacing = spacing;

      gtk_widget_queue_resize (GTK_WIDGET (box));

      g_object_notify (G_OBJECT (box), "horizontal-spacing");
    }
}

/**
 * egg_flow_box_get_column_spacing:
 * @box: An #EggFlowBox
 *
 * Gets the horizontal spacing.
 *
 * Returns: The horizontal spacing.
 */
guint
egg_flow_box_get_column_spacing (EggFlowBox *box)
{
  g_return_val_if_fail (EGG_IS_FLOW_BOX (box), FALSE);

  return box->priv->column_spacing;
}

/**
 * egg_flow_box_set_min_children_per_line:
 * @box: An #EggFlowBox
 * @n_children: The minimum amount of children per line.
 *
 * Sets the minimum amount of children to line up
 * in @box's orientation before flowping.
 */
void
egg_flow_box_set_min_children_per_line (EggFlowBox *box,
                                        guint       n_children)
{
  EggFlowBoxPrivate *priv;

  g_return_if_fail (EGG_IS_FLOW_BOX (box));

  priv = box->priv;

  if (priv->min_children_per_line != n_children)
    {
      priv->min_children_per_line = n_children;

      gtk_widget_queue_resize (GTK_WIDGET (box));

      g_object_notify (G_OBJECT (box), "min-children-per-line");
    }
}

/**
 * egg_flow_box_get_min_children_per_line:
 * @box: An #EggFlowBox
 *
 * Gets the minimum amount of children per line.
 *
 * Returns: The minimum amount of children per line.
 */
guint
egg_flow_box_get_min_children_per_line (EggFlowBox *box)
{
  g_return_val_if_fail (EGG_IS_FLOW_BOX (box), FALSE);

  return box->priv->min_children_per_line;
}

/**
 * egg_flow_box_set_max_children_per_line:
 * @box: An #EggFlowBox
 * @n_children: The natural amount of children per line.
 *
 * Sets the natural length of items to request and
 * allocate space for in @box's orientation.
 *
 * Setting the natural amount of children per line
 * limits the overall natural size request to be no more
 * than @n_children items long in the given orientation.
 */
void
egg_flow_box_set_max_children_per_line (EggFlowBox *box,
                                        guint       n_children)
{
  EggFlowBoxPrivate *priv;

  g_return_if_fail (EGG_IS_FLOW_BOX (box));

  priv = box->priv;

  if (priv->max_children_per_line != n_children)
    {
      priv->max_children_per_line = n_children;

      gtk_widget_queue_resize (GTK_WIDGET (box));

      g_object_notify (G_OBJECT (box), "max-children-per-line");
    }
}

/**
 * egg_flow_box_get_max_children_per_line:
 * @box: An #EggFlowBox
 *
 * Gets the natural amount of children per line.
 *
 * Returns: The natural amount of children per line.
 */
guint
egg_flow_box_get_max_children_per_line (EggFlowBox *box)
{
  g_return_val_if_fail (EGG_IS_FLOW_BOX (box), FALSE);

  return box->priv->max_children_per_line;
}

/**
 * egg_flow_box_set_activate_on_single_click:
 * @box: An #EggFlowBox
 * @single: %TRUE to emit child-activated on a single click
 *
 * Causes the #EggFlowBox::child-activated signal to be emitted on
 * a single click instead of a double click.
 **/
void
egg_flow_box_set_activate_on_single_click (EggFlowBox *box,
                                           gboolean    single)
{
  g_return_if_fail (EGG_IS_FLOW_BOX (box));

  single = single != FALSE;

  if (box->priv->activate_on_single_click == single)
    return;

  box->priv->activate_on_single_click = single;
  g_object_notify (G_OBJECT (box), "activate-on-single-click");
}

/**
 * egg_flow_box_get_activate_on_single_click:
 * @box: An #EggFlowBox
 *
 * Gets the setting set by egg_flow_box_set_activate_on_single_click().
 *
 * Return value: %TRUE if child-activated will be emitted on a single click
 **/
gboolean
egg_flow_box_get_activate_on_single_click (EggFlowBox *box)
{
  g_return_val_if_fail (EGG_IS_FLOW_BOX (box), FALSE);

  return box->priv->activate_on_single_click;
}

static void
egg_flow_box_get_property (GObject      *object,
                           guint         prop_id,
                           GValue       *value,
                           GParamSpec   *pspec)
{
  EggFlowBox *box  = EGG_FLOW_BOX (object);
  EggFlowBoxPrivate *priv = box->priv;

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, priv->orientation);
      break;
    case PROP_HOMOGENEOUS:
      g_value_set_boolean (value, priv->homogeneous);
      break;
    case PROP_HALIGN_POLICY:
      g_value_set_enum (value, priv->halign_policy);
      break;
    case PROP_VALIGN_POLICY:
      g_value_set_enum (value, priv->valign_policy);
      break;
    case PROP_COLUMN_SPACING:
      g_value_set_uint (value, priv->column_spacing);
      break;
    case PROP_ROW_SPACING:
      g_value_set_uint (value, priv->row_spacing);
      break;
    case PROP_MIN_CHILDREN_PER_LINE:
      g_value_set_uint (value, priv->min_children_per_line);
      break;
    case PROP_MAX_CHILDREN_PER_LINE:
      g_value_set_uint (value, priv->max_children_per_line);
      break;
    case PROP_SELECTION_MODE:
      g_value_set_enum (value, priv->selection_mode);
      break;
    case PROP_ACTIVATE_ON_SINGLE_CLICK:
      g_value_set_boolean (value, priv->activate_on_single_click);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
egg_flow_box_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  EggFlowBox *box = EGG_FLOW_BOX (object);
  EggFlowBoxPrivate *priv   = box->priv;

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      priv->orientation = g_value_get_enum (value);

      /* Re-box the children in the new orientation */
      gtk_widget_queue_resize (GTK_WIDGET (box));
      break;
    case PROP_HOMOGENEOUS:
      egg_flow_box_set_homogeneous (box, g_value_get_boolean (value));
      break;
    case PROP_HALIGN_POLICY:
      egg_flow_box_set_halign_policy (box, g_value_get_enum (value));
      break;
    case PROP_VALIGN_POLICY:
      egg_flow_box_set_valign_policy (box, g_value_get_enum (value));
      break;
    case PROP_COLUMN_SPACING:
      egg_flow_box_set_column_spacing (box, g_value_get_uint (value));
      break;
    case PROP_ROW_SPACING:
      egg_flow_box_set_row_spacing (box, g_value_get_uint (value));
      break;
    case PROP_MIN_CHILDREN_PER_LINE:
      egg_flow_box_set_min_children_per_line (box, g_value_get_uint (value));
      break;
    case PROP_MAX_CHILDREN_PER_LINE:
      egg_flow_box_set_max_children_per_line (box, g_value_get_uint (value));
      break;
    case PROP_SELECTION_MODE:
      egg_flow_box_set_selection_mode (box, g_value_get_enum (value));
      break;
    case PROP_ACTIVATE_ON_SINGLE_CLICK:
      egg_flow_box_set_activate_on_single_click (box, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static EggFlowBoxChildInfo *
egg_flow_box_find_child_at_pos (EggFlowBox *box,
                                gint        x,
                                gint        y)
{
  EggFlowBoxPrivate *priv = box->priv;
  EggFlowBoxChildInfo *child_info;
  GSequenceIter *iter;
  EggFlowBoxChildInfo *info;

  child_info = NULL;
  for (iter = g_sequence_get_begin_iter (priv->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      info = (EggFlowBoxChildInfo *) g_sequence_get (iter);
      if (x >= info->area.x && x < (info->area.x + info->area.width)
          && y >= info->area.y && y < (info->area.y + info->area.height))
        {
          child_info = info;
          break;
        }
    }

  return child_info;
}

static void
egg_flow_box_update_prelight (EggFlowBox          *box,
                              EggFlowBoxChildInfo *child)
{
  EggFlowBoxPrivate *priv = box->priv;

  if (child != priv->prelight_child)
    {
      priv->prelight_child = child;
      gtk_widget_queue_draw (GTK_WIDGET (box));
    }
}

static void
egg_flow_box_update_active (EggFlowBox          *box,
                            EggFlowBoxChildInfo *child)
{
  EggFlowBoxPrivate *priv = box->priv;
  gboolean val;

  val = priv->active_child == child;
  if (priv->active_child != NULL &&
      val != priv->active_child_active)
    {
      priv->active_child_active = val;
      gtk_widget_queue_draw (GTK_WIDGET (box));
    }
}

static gboolean
egg_flow_box_real_enter_notify_event (GtkWidget        *widget,
                                      GdkEventCrossing *event)
{
  EggFlowBox *box = EGG_FLOW_BOX (widget);
  EggFlowBoxChildInfo *child_info;


  if (event->window != gtk_widget_get_window (GTK_WIDGET (box)))
    return FALSE;

  child_info = egg_flow_box_find_child_at_pos (box, event->x, event->y);
  egg_flow_box_update_prelight (box, child_info);
  egg_flow_box_update_active (box, child_info);

  return FALSE;
}

static gboolean
egg_flow_box_real_leave_notify_event (GtkWidget        *widget,
                                      GdkEventCrossing *event)
{
  EggFlowBox *box = EGG_FLOW_BOX (widget);
  EggFlowBoxChildInfo *child_info = NULL;

  if (event->window != gtk_widget_get_window (GTK_WIDGET (box)))
    return FALSE;

  if (event->detail != GDK_NOTIFY_INFERIOR)
    child_info = NULL;
  else
    child_info = egg_flow_box_find_child_at_pos (box, event->x, event->y);

  egg_flow_box_update_prelight (box, child_info);
  egg_flow_box_update_active (box, child_info);

  return FALSE;
}

static gboolean
egg_flow_box_real_motion_notify_event (GtkWidget      *widget,
                                       GdkEventMotion *event)
{
  EggFlowBox *box = EGG_FLOW_BOX (widget);
  EggFlowBoxChildInfo *child_info;
  GdkWindow *window;
  GdkWindow *event_window;
  gint relative_x;
  gint relative_y;
  gdouble parent_x;
  gdouble parent_y;

  window = gtk_widget_get_window (GTK_WIDGET (box));
  event_window = event->window;
  relative_x = event->x;
  relative_y = event->y;

  while ((event_window != NULL) && (event_window != window))
    {
      gdk_window_coords_to_parent (event_window,
                                   relative_x, relative_y,
                                   &parent_x, &parent_y);
      relative_x = parent_x;
      relative_y = parent_y;
      event_window = gdk_window_get_effective_parent (event_window);
    }

  child_info = egg_flow_box_find_child_at_pos (box, relative_x, relative_y);
  egg_flow_box_update_prelight (box, child_info);
  egg_flow_box_update_active (box, child_info);

  return FALSE;
}

static gboolean
egg_flow_box_real_button_press_event (GtkWidget      *widget,
                                      GdkEventButton *event)
{
  EggFlowBox *box = EGG_FLOW_BOX (widget);
  EggFlowBoxPrivate *priv = box->priv;

  if (event->button == GDK_BUTTON_PRIMARY)
    {
      EggFlowBoxChildInfo *child_info;
      child_info = egg_flow_box_find_child_at_pos (box, event->x, event->y);
      if (child_info != NULL)
        {
          priv->active_child = child_info;
          priv->active_child_active = TRUE;
          gtk_widget_queue_draw (GTK_WIDGET (box));
          if (event->type == GDK_2BUTTON_PRESS &&
              !priv->activate_on_single_click)
            g_signal_emit (box,
                           signals[CHILD_ACTIVATED], 0,
                           child_info->widget);
        }
    }

  return FALSE;
}

static void
egg_flow_box_queue_draw_child (EggFlowBox          *box,
                               EggFlowBoxChildInfo *child_info)
{
  GdkRectangle rect;
  GdkWindow *window;

  rect = child_info->area;

  window = gtk_widget_get_window (GTK_WIDGET (box));
  gdk_window_invalidate_rect (window, &rect, TRUE);
}

static gboolean
egg_flow_box_unselect_all_internal (EggFlowBox *box)
{
  EggFlowBoxChildInfo *child_info;
  GSequenceIter *iter;
  gboolean dirty = FALSE;

  if (box->priv->selection_mode == GTK_SELECTION_NONE)
    return FALSE;

  for (iter = g_sequence_get_begin_iter (box->priv->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      child_info = g_sequence_get (iter);
      if (child_info->selected)
        {
          child_info->selected = FALSE;
          egg_flow_box_queue_draw_child (box, child_info);
          dirty = TRUE;
        }
    }

  return dirty;
}

static void
egg_flow_box_unselect_child_info (EggFlowBox          *box,
                                  EggFlowBoxChildInfo *child_info)
{
  if (!child_info->selected)
    return;

  if (box->priv->selection_mode == GTK_SELECTION_NONE)
    return;
  else if (box->priv->selection_mode != GTK_SELECTION_MULTIPLE)
    egg_flow_box_unselect_all_internal (box);
  else
    child_info->selected = FALSE;

  g_signal_emit (box, signals[SELECTED_CHILDREN_CHANGED], 0);

  egg_flow_box_queue_draw_child (box, child_info);
}

static void
egg_flow_box_update_cursor (EggFlowBox          *box,
                            EggFlowBoxChildInfo *child_info)
{
  EggFlowBoxPrivate *priv = box->priv;

  priv->cursor_child = child_info;
  gtk_widget_grab_focus (GTK_WIDGET (box));
  gtk_widget_queue_draw (GTK_WIDGET (box));

  if (child_info != NULL && priv->adjustment != NULL)
    {
      GtkAllocation allocation;

      gtk_widget_get_allocation (GTK_WIDGET (box), &allocation);
      gtk_adjustment_clamp_page (priv->adjustment,
                                 priv->cursor_child->area.y + allocation.y,
                                 priv->cursor_child->area.y + allocation.y + priv->cursor_child->area.height);
  }

  _egg_flow_box_accessible_update_cursor (GTK_WIDGET (box), child_info ? child_info->widget : NULL);
}

static void
egg_flow_box_select_child_info (EggFlowBox          *box,
                                EggFlowBoxChildInfo *child_info)
{
  if (child_info->selected)
    return;
  if (box->priv->selection_mode == GTK_SELECTION_NONE)
    return;
  else if (box->priv->selection_mode != GTK_SELECTION_MULTIPLE)
    egg_flow_box_unselect_all_internal (box);

  child_info->selected = TRUE;
  box->priv->selected_child = child_info;

  g_signal_emit (box, signals[SELECTED_CHILDREN_CHANGED], 0);

  egg_flow_box_queue_draw_child (box, child_info);
  egg_flow_box_update_cursor (box, child_info);
}

static void
egg_flow_box_select_all_between (EggFlowBox          *box,
                                 EggFlowBoxChildInfo *child1,
                                 EggFlowBoxChildInfo *child2)
{
  GSequenceIter *iter, *iter1, *iter2;

  iter1 = child1 ? child1->iter : g_sequence_get_begin_iter (box->priv->children);
  iter2 = child2 ? child2->iter : g_sequence_get_end_iter (box->priv->children);
  if (g_sequence_iter_compare (iter2, iter1) < 0)
    {
      iter = iter1;
      iter1 = iter2;
      iter2 = iter;
    }

  for (iter = iter1; ; iter = g_sequence_iter_next (iter))
    {
      EggFlowBoxChildInfo *child_info;

      child_info = g_sequence_get (iter);
      if (child_info && child_is_visible (child_info->widget))
        {
          child_info->selected = TRUE;
          egg_flow_box_queue_draw_child (box, child_info);
        }

      if (g_sequence_iter_compare (iter, iter2) == 0)
        break;
    }
}

static void
egg_flow_box_update_selection (EggFlowBox          *box,
                               EggFlowBoxChildInfo *child_info,
                               gboolean             modify,
                               gboolean             extend)
{
  EggFlowBoxPrivate *priv = box->priv;

  if (priv->selection_mode == GTK_SELECTION_NONE)
    return;

  if (priv->selection_mode == GTK_SELECTION_BROWSE)
    {
      egg_flow_box_unselect_all_internal (box);
      child_info->selected = TRUE;
      priv->selected_child = child_info;
    }
  else if (priv->selection_mode == GTK_SELECTION_SINGLE)
    {
      gboolean was_selected;

      was_selected = child_info->selected;
      egg_flow_box_unselect_all_internal (box);
      child_info->selected = modify ? !was_selected : TRUE;
      priv->selected_child = child_info->selected ? child_info : NULL;
    }
  else /* GTK_SELECTION_MULTIPLE */
    {
      if (extend)
        {
          egg_flow_box_unselect_all_internal (box);
          if (priv->selected_child == NULL)
            {
              child_info->selected = TRUE;
              priv->selected_child = child_info;
            }
          else
            egg_flow_box_select_all_between (box, priv->selected_child, child_info);
        }
      else
        {
          child_info->selected = modify ? !child_info->selected : TRUE;
          priv->selected_child = child_info;
        }
    }

  g_signal_emit (box, signals[SELECTED_CHILDREN_CHANGED], 0);

  egg_flow_box_queue_draw_child (box, child_info);
  egg_flow_box_update_cursor (box, child_info);
}

static void
egg_flow_box_select_and_activate (EggFlowBox          *box,
                                  EggFlowBoxChildInfo *child_info)
{
  GtkWidget *w = NULL;

  if (child_info != NULL)
    {
      w = child_info->widget;
      egg_flow_box_select_child_info (box, child_info);
    }

  if (w != NULL)
    g_signal_emit (box, signals[CHILD_ACTIVATED], 0, w);
}

static gboolean
egg_flow_box_real_button_release_event (GtkWidget      *widget,
                                        GdkEventButton *event)
{
  EggFlowBox *box = EGG_FLOW_BOX (widget);
  EggFlowBoxPrivate *priv = box->priv;

  if (event->button == GDK_BUTTON_PRIMARY)
    {
      if (priv->active_child != NULL &&
          priv->active_child_active)
        {
          GdkModifierType state = 0;
          gboolean modify_selection_pressed;
          gboolean extend_selection_pressed;

          modify_selection_pressed = FALSE;
          extend_selection_pressed = FALSE;
          if (gtk_get_current_event_state (&state))
            {
              GdkModifierType extend_mod_mask;
              GdkModifierType modify_mod_mask;

              extend_mod_mask = gtk_widget_get_modifier_mask (GTK_WIDGET (box),
                                                              GDK_MODIFIER_INTENT_EXTEND_SELECTION);
              if ((state & extend_mod_mask) == extend_mod_mask)
                extend_selection_pressed = TRUE;

              modify_mod_mask = gtk_widget_get_modifier_mask (GTK_WIDGET (box),
                                                              GDK_MODIFIER_INTENT_MODIFY_SELECTION);
              if ((state & modify_mod_mask) == modify_mod_mask)
                modify_selection_pressed = TRUE;
            }

          if (priv->activate_on_single_click)
            egg_flow_box_select_and_activate (box, priv->active_child);
          else
            egg_flow_box_update_selection (box, priv->active_child, modify_selection_pressed, extend_selection_pressed);
        }
      priv->active_child = NULL;
      priv->active_child_active = FALSE;
      gtk_widget_queue_draw (GTK_WIDGET (box));
  }

  return FALSE;
}

static EggFlowBoxChildInfo *
egg_flow_box_get_first_visible (EggFlowBox *box)
{
  EggFlowBoxPrivate *priv = box->priv;
  EggFlowBoxChildInfo *child_info;
  GSequenceIter *iter;

  for (iter = g_sequence_get_begin_iter (priv->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
        child_info = g_sequence_get (iter);
        if (child_is_visible (child_info->widget))
          return child_info;
    }

  return NULL;
}

static EggFlowBoxChildInfo *
egg_flow_box_get_last_visible (EggFlowBox *box)
{
  EggFlowBoxPrivate *priv = box->priv;
  EggFlowBoxChildInfo *child_info;
  GSequenceIter *iter;

  iter = g_sequence_get_end_iter (priv->children);
  while (!g_sequence_iter_is_begin (iter))
    {
      iter = g_sequence_iter_prev (iter);
      child_info = g_sequence_get (iter);
      if (child_is_visible (child_info->widget))
        return child_info;
    }

  return NULL;
}

static GSequenceIter *
egg_flow_box_get_previous_visible (EggFlowBox    *box,
                                   GSequenceIter *iter)
{
  EggFlowBoxChildInfo *child_info;

  if (g_sequence_iter_is_begin (iter))
    return NULL;

  do
    {
      iter = g_sequence_iter_prev (iter);
      child_info = g_sequence_get (iter);
      if (child_is_visible (child_info->widget))
        return iter;
    }
  while (!g_sequence_iter_is_begin (iter));

  return NULL;
}

static GSequenceIter *
egg_flow_box_get_next_visible (EggFlowBox    *box,
                               GSequenceIter *iter)
{
  EggFlowBoxChildInfo *child_info;

  if (g_sequence_iter_is_end (iter))
    return iter;

  do
    {
      iter = g_sequence_iter_next (iter);
      if (!g_sequence_iter_is_end (iter))
        {
        child_info = g_sequence_get (iter);
        if (child_is_visible (child_info->widget))
          return iter;
        }
    }
  while (!g_sequence_iter_is_end (iter));

  return iter;
}

static GSequenceIter *
egg_flow_box_get_above_visible (EggFlowBox    *box,
                                GSequenceIter *iter)
{
  EggFlowBoxChildInfo *child_info;
  GSequenceIter *ret = NULL;
  gint i;

  if (g_sequence_iter_is_begin (iter))
    return NULL;

  i = 0;
  do
    {
      iter = g_sequence_iter_prev (iter);
      child_info = g_sequence_get (iter);
      if (child_is_visible (child_info->widget))
        i++;
    }
  while (!g_sequence_iter_is_begin (iter)
         && i < box->priv->cur_children_per_line);

  if (i == box->priv->cur_children_per_line)
    ret = iter;

  return ret;
}

static GSequenceIter *
egg_flow_box_get_below_visible (EggFlowBox    *box,
                                GSequenceIter *iter)
{
  EggFlowBoxChildInfo *child_info;
  GSequenceIter *ret = NULL;
  gint i;

  if (g_sequence_iter_is_end (iter))
    return iter;

  i = 0;
  do
    {
      iter = g_sequence_iter_next (iter);
      if (!g_sequence_iter_is_end (iter))
        {
          child_info = g_sequence_get (iter);
          if (child_is_visible (child_info->widget))
            i++;
        }
    }
  while (!g_sequence_iter_is_end (iter)
         && i < box->priv->cur_children_per_line);

  if (i == box->priv->cur_children_per_line)
    ret = iter;

  return ret;
}

static gboolean
egg_flow_box_real_focus (GtkWidget       *widget,
                         GtkDirectionType direction)
{
  EggFlowBox *box = EGG_FLOW_BOX (widget);
  EggFlowBoxPrivate *priv = box->priv;
  gboolean had_focus = FALSE;
  GtkWidget *recurse_into;
  EggFlowBoxChildInfo *current_focus_child;
  EggFlowBoxChildInfo *next_focus_child;
  gboolean modify_selection_pressed;
  gboolean extend_selection_pressed;
  GdkModifierType state = 0;

  recurse_into = NULL;

  g_object_get (GTK_WIDGET (box), "has-focus", &had_focus, NULL);
  current_focus_child = NULL;
  next_focus_child = NULL;

  if (had_focus)
    {
      /* If on row, going right, enter into possible container */
      if (direction == GTK_DIR_RIGHT || direction == GTK_DIR_TAB_FORWARD)
        {
          if (priv->cursor_child != NULL)
            recurse_into = priv->cursor_child->widget;
        }
      current_focus_child = priv->cursor_child;
    }
  else if (gtk_container_get_focus_child ((GtkContainer *) box) != NULL)
    {
      /* There is a focus child, always navigate inside it first */
      recurse_into = gtk_container_get_focus_child ((GtkContainer *) box);
      current_focus_child = egg_flow_box_lookup_info (box, recurse_into);

      /* If exiting child container to the left, select row or out */
      if (direction == GTK_DIR_LEFT || direction == GTK_DIR_TAB_BACKWARD)
        next_focus_child = current_focus_child;
    }
  else
    {
      /* If coming from the left, enter into possible container */
      if (direction == GTK_DIR_LEFT || direction == GTK_DIR_TAB_BACKWARD)
        {
          if (priv->selected_child != NULL)
            recurse_into = priv->selected_child->widget;
        }
   }

  if (recurse_into != NULL)
    {
      if (gtk_widget_child_focus (recurse_into, direction))
        return TRUE;
    }

  if (next_focus_child == NULL)
    {
      if (current_focus_child != NULL)
        {
          GSequenceIter *i;

          if (direction == GTK_DIR_LEFT)
            {
              i = egg_flow_box_get_previous_visible (box, current_focus_child->iter);
              if (i != NULL)
                next_focus_child = g_sequence_get (i);
            }
          else if (direction == GTK_DIR_RIGHT)
            {
              i = egg_flow_box_get_next_visible (box, current_focus_child->iter);
              if (i != NULL && !g_sequence_iter_is_end (i))
                next_focus_child = g_sequence_get (i);
            }
          else if (direction == GTK_DIR_UP)
            {
              i = egg_flow_box_get_above_visible (box, current_focus_child->iter);
              if (i != NULL && !g_sequence_iter_is_end (i))
                next_focus_child = g_sequence_get (i);
            }
          else if (direction == GTK_DIR_DOWN)
            {
              i = egg_flow_box_get_below_visible (box, current_focus_child->iter);
              if (i != NULL && !g_sequence_iter_is_end (i))
                next_focus_child = g_sequence_get (i);
            }
        }
      else
        {
          switch (direction)
            {
            case GTK_DIR_UP:
            case GTK_DIR_TAB_BACKWARD:
              next_focus_child = priv->selected_child;
              if (next_focus_child == NULL)
                next_focus_child = egg_flow_box_get_last_visible (box);
              break;
            default:
              next_focus_child = priv->selected_child;
              if (next_focus_child == NULL)
                next_focus_child =
                  egg_flow_box_get_first_visible (box);
              break;
            }
        }
    }

  if (next_focus_child == NULL)
    {
      if (direction == GTK_DIR_UP || direction == GTK_DIR_DOWN
          || direction == GTK_DIR_LEFT || direction == GTK_DIR_RIGHT)
        {
          if (gtk_widget_keynav_failed (GTK_WIDGET (box), direction))
            return TRUE;
        }

      return FALSE;
    }

  modify_selection_pressed = FALSE;
  extend_selection_pressed = FALSE;
  if (gtk_get_current_event_state (&state))
    {
      GdkModifierType extend_mod_mask;
      GdkModifierType modify_mod_mask;

      extend_mod_mask = gtk_widget_get_modifier_mask (GTK_WIDGET (box),
                                                      GDK_MODIFIER_INTENT_EXTEND_SELECTION);
      if ((state & extend_mod_mask) == extend_mod_mask)
        extend_selection_pressed = TRUE;

      modify_mod_mask = gtk_widget_get_modifier_mask (GTK_WIDGET (box),
                                                      GDK_MODIFIER_INTENT_MODIFY_SELECTION);
      if ((state & modify_mod_mask) == modify_mod_mask)
        modify_selection_pressed = TRUE;
    }

  egg_flow_box_update_cursor (box, next_focus_child);
  if (!modify_selection_pressed)
    egg_flow_box_update_selection (box, next_focus_child, FALSE, extend_selection_pressed);

  return TRUE;
}

typedef struct {
  EggFlowBoxChildInfo *child;
  GtkStateFlags state;
} ChildFlags;

static ChildFlags *
child_flags_find_or_add (ChildFlags          *array,
                         int                 *array_length,
                         EggFlowBoxChildInfo *to_find)
{
  gint i;

  for (i = 0; i < *array_length; i++)
    {
      if (array[i].child == to_find)
        return &array[i];
    }

  *array_length = *array_length + 1;
  array[*array_length - 1].child = to_find;
  array[*array_length - 1].state = 0;
  return &array[*array_length - 1];
}

static void
egg_flow_box_add_move_binding (GtkBindingSet  *binding_set,
                               guint           keyval,
                               GdkModifierType modmask,
                               GtkMovementStep step,
                               gint            count)
{
  gtk_binding_entry_add_signal (binding_set, keyval, modmask,
                                "move-cursor", (guint) 2,
                                GTK_TYPE_MOVEMENT_STEP, step,
                                G_TYPE_INT, count,
                                NULL);
  gtk_binding_entry_add_signal (binding_set, keyval, modmask | GDK_SHIFT_MASK,
                                "move-cursor", (guint) 2,
                                GTK_TYPE_MOVEMENT_STEP, step,
                                G_TYPE_INT, count,
                                NULL);

  if ((modmask & GDK_CONTROL_MASK) == GDK_CONTROL_MASK)
    return;

  gtk_binding_entry_add_signal (binding_set, keyval, GDK_CONTROL_MASK,
                                "move-cursor", (guint) 2,
                                GTK_TYPE_MOVEMENT_STEP, step,
                                G_TYPE_INT, count,
                                NULL);
  gtk_binding_entry_add_signal (binding_set, keyval, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
                                "move-cursor", (guint) 2,
                                GTK_TYPE_MOVEMENT_STEP, step,
                                G_TYPE_INT, count,
                                NULL);
}

static void
egg_flow_box_real_activate_cursor_child (EggFlowBox *box)
{
  EggFlowBoxPrivate *priv = box->priv;

  egg_flow_box_select_and_activate (box, priv->cursor_child);
}

static void
egg_flow_box_real_toggle_cursor_child (EggFlowBox *box)
{
  EggFlowBoxPrivate *priv = box->priv;

  if (priv->cursor_child == NULL)
    return;

  if ((priv->selection_mode == GTK_SELECTION_SINGLE ||
       priv->selection_mode == GTK_SELECTION_MULTIPLE) &&
      priv->cursor_child->selected)
    egg_flow_box_unselect_child_info (box, priv->cursor_child);
  else
    egg_flow_box_select_and_activate (box, priv->cursor_child);
}

static void
egg_flow_box_real_move_cursor (EggFlowBox     *box,
                               GtkMovementStep step,
                               gint            count)
{
  EggFlowBoxPrivate *priv = box->priv;
  GdkModifierType state;
  gboolean extend_selection_pressed;
  gboolean modify_selection_pressed;
  EggFlowBoxChildInfo *child;
  GdkModifierType extend_mod_mask;
  GdkModifierType modify_mod_mask;
  EggFlowBoxChildInfo *prev;
  EggFlowBoxChildInfo *next;
  gint page_size;
  GSequenceIter *iter;
  gint start_y;
  gint end_y;

  extend_selection_pressed = FALSE;
  modify_selection_pressed = FALSE;

  if (gtk_get_current_event_state (&state))
    {
      extend_mod_mask = gtk_widget_get_modifier_mask (GTK_WIDGET (box),
                                                      GDK_MODIFIER_INTENT_EXTEND_SELECTION);
      if ((state & extend_mod_mask) == extend_mod_mask)
        extend_selection_pressed = TRUE;

      modify_mod_mask = gtk_widget_get_modifier_mask (GTK_WIDGET (box),
                                                      GDK_MODIFIER_INTENT_MODIFY_SELECTION);
      if ((state & modify_mod_mask) == modify_mod_mask)
        modify_selection_pressed = TRUE;
    }

  child = NULL;
  switch (step)
    {
    case GTK_MOVEMENT_VISUAL_POSITIONS:
      if (priv->cursor_child != NULL)
        {
          iter = priv->cursor_child->iter;
          if (gtk_widget_get_direction (GTK_WIDGET (box)) == GTK_TEXT_DIR_RTL)
            count = - count;

          while (count < 0 && iter != NULL)
            {
              iter = egg_flow_box_get_previous_visible (box, iter);
              count = count + 1;
            }
          while (count > 0  && iter != NULL)
            {
              iter = egg_flow_box_get_next_visible (box, iter);
              count = count - 1;
            }

          if (iter != NULL && !g_sequence_iter_is_end (iter))
            child = g_sequence_get (iter);
        }
      break;
    case GTK_MOVEMENT_BUFFER_ENDS:
      if (count < 0)
        child = egg_flow_box_get_first_visible (box);
      else
        child = egg_flow_box_get_last_visible (box);
      break;
    case GTK_MOVEMENT_DISPLAY_LINES:
      if (priv->cursor_child != NULL)
        {
          iter = priv->cursor_child->iter;

          while (count < 0  && iter != NULL)
            {
              iter = egg_flow_box_get_above_visible (box, iter);
              count = count + 1;
            }
          while (count > 0  && iter != NULL)
            {
              iter = egg_flow_box_get_below_visible (box, iter);
              count = count - 1;
            }

          if (iter != NULL && !g_sequence_iter_is_end (iter))
            child = g_sequence_get (iter);
        }
      break;
    case GTK_MOVEMENT_PAGES:
      page_size = 100;
      if (priv->adjustment != NULL)
        page_size = gtk_adjustment_get_page_increment (priv->adjustment);

      if (priv->cursor_child != NULL)
        {
          start_y = priv->cursor_child->area.y;
          end_y = start_y;
          iter = priv->cursor_child->iter;

          child = priv->cursor_child;
          if (count < 0)
            {
              gint i = 0;

              /* Up */
              while (iter != NULL && !g_sequence_iter_is_begin (iter))
                {
                  iter = egg_flow_box_get_previous_visible (box, iter);
                  if (iter == NULL)
                    break;

                  prev = g_sequence_get (iter);

                  /* go up an even number of rows */
                  if (i % priv->cur_children_per_line == 0
                      && prev->area.y < start_y - page_size)
                    break;

                  child = prev;
                  i++;
                }
            }
          else
            {
              gint i = 0;

              /* Down */
              while (iter != NULL && !g_sequence_iter_is_end (iter))
                {
                  iter = egg_flow_box_get_next_visible (box, iter);
                  if (g_sequence_iter_is_end (iter))
                    break;

                  next = g_sequence_get (iter);

                  if (i % priv->cur_children_per_line == 0
                      && next->area.y > start_y + page_size)
                    break;

                  child = next;
                  i++;
                }
            }
          end_y = child->area.y;
        }
      break;
    default:
      return;
    }

  if (child == NULL || child == priv->cursor_child)
    {
      GtkDirectionType direction = count < 0 ? GTK_DIR_UP : GTK_DIR_DOWN;

      if (!gtk_widget_keynav_failed (GTK_WIDGET (box), direction))
        {
          GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (box));

          if (toplevel)
            gtk_widget_child_focus (toplevel,
                                    direction == GTK_DIR_UP ?
                                    GTK_DIR_TAB_BACKWARD :
                                    GTK_DIR_TAB_FORWARD);

        }

      return;
    }

  egg_flow_box_update_cursor (box, child);
  if (!modify_selection_pressed)
    egg_flow_box_update_selection (box, child, FALSE, extend_selection_pressed);
}

void
egg_flow_box_select_all (EggFlowBox *box)
{
  g_return_if_fail (EGG_IS_FLOW_BOX (box));

  if (box->priv->selection_mode != GTK_SELECTION_MULTIPLE)
    return;

  if (g_sequence_get_length (box->priv->children) > 0)
    {
      egg_flow_box_select_all_between (box, NULL, NULL);
      g_signal_emit (box, signals[SELECTED_CHILDREN_CHANGED], 0);
    }
}

void
egg_flow_box_unselect_all (EggFlowBox *box)
{
  gboolean dirty = FALSE;

  g_return_if_fail (EGG_IS_FLOW_BOX (box));

  if (box->priv->selection_mode == GTK_SELECTION_BROWSE)
    return;

  dirty = egg_flow_box_unselect_all_internal (box);

  if (dirty)
    g_signal_emit (box, signals[SELECTED_CHILDREN_CHANGED], 0);
}

static gboolean
egg_flow_box_real_draw (GtkWidget *widget,
                        cairo_t   *cr)
{
  EggFlowBox *box = EGG_FLOW_BOX (widget);
  EggFlowBoxPrivate *priv = box->priv;
  GtkAllocation allocation = {0};
  GtkStyleContext* context;
  GtkStateFlags state;
  gint focus_pad;
  int i;
  GSequenceIter *iter;

  gtk_widget_get_allocation (GTK_WIDGET (box), &allocation);
  context = gtk_widget_get_style_context (GTK_WIDGET (box));
  state = gtk_widget_get_state_flags (widget);
  gtk_render_background (context, cr, (gdouble) 0, (gdouble) 0, (gdouble) allocation.width, (gdouble) allocation.height);

  for (iter = g_sequence_get_begin_iter (priv->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      EggFlowBoxChildInfo *child_info;
      ChildFlags flags[3], *found;
      gint flags_length;

      child_info = g_sequence_get (iter);

      flags_length = 0;

      if (child_info->selected)
        {
          found = child_flags_find_or_add (flags, &flags_length, child_info);
          found->state |= (state | GTK_STATE_FLAG_SELECTED);
        }

      if (priv->prelight_child == child_info)
        {
          found = child_flags_find_or_add (flags, &flags_length, child_info);
          found->state |= (state | GTK_STATE_FLAG_PRELIGHT);
        }

      if (priv->active_child == child_info && priv->active_child_active)
        {
          found = child_flags_find_or_add (flags, &flags_length, child_info);
          found->state |= (state | GTK_STATE_FLAG_ACTIVE);
        }

      for (i = 0; i < flags_length; i++)
        {
          ChildFlags *flag = &flags[i];
          gtk_style_context_save (context);
          gtk_style_context_set_state (context, flag->state);
          gtk_render_background (context, cr,
                                 flag->child->area.x, flag->child->area.y,
                                 flag->child->area.width, flag->child->area.height);
          gtk_style_context_restore (context);
        }
    }

  if (gtk_widget_has_visible_focus (GTK_WIDGET (box)) && priv->cursor_child != NULL)
    {
      gtk_style_context_get_style (context,
                                   "focus-padding", &focus_pad,
                                   NULL);
      gtk_render_focus (context, cr,
                        priv->cursor_child->area.x + focus_pad,
                        priv->cursor_child->area.y + focus_pad,
                        priv->cursor_child->area.width - 2 * focus_pad,
                        priv->cursor_child->area.height - 2 * focus_pad);
    }


  GTK_WIDGET_CLASS (egg_flow_box_parent_class)->draw ((GtkWidget *) G_TYPE_CHECK_INSTANCE_CAST (box, GTK_TYPE_CONTAINER, GtkContainer), cr);

  return TRUE;
}

static void
egg_flow_box_real_realize (GtkWidget *widget)
{
  EggFlowBox *box = EGG_FLOW_BOX (widget);
  GtkAllocation allocation;
  GdkWindowAttr attributes = {0};
  GdkWindow *window;

  gtk_widget_get_allocation (GTK_WIDGET (box), &allocation);
  gtk_widget_set_realized (GTK_WIDGET (box), TRUE);

  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask = gtk_widget_get_events (GTK_WIDGET (box)) |
    GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_POINTER_MOTION_MASK |
    GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK;
  attributes.wclass = GDK_INPUT_OUTPUT;

  window = gdk_window_new (gtk_widget_get_parent_window (GTK_WIDGET (box)),
                           &attributes, GDK_WA_X | GDK_WA_Y);
  gtk_style_context_set_background (gtk_widget_get_style_context (GTK_WIDGET (box)), window);
  gdk_window_set_user_data (window, (GObject*) box);
  gtk_widget_set_window (GTK_WIDGET (box), window); /* Passes ownership */
}

static void
egg_flow_box_finalize (GObject *obj)
{
  EggFlowBox *flow_box = EGG_FLOW_BOX (obj);
  EggFlowBoxPrivate *priv = flow_box->priv;

  g_sequence_free (priv->children);
  g_hash_table_unref (priv->child_hash);
  g_clear_object (&priv->adjustment);

  G_OBJECT_CLASS (egg_flow_box_parent_class)->finalize (obj);
}

static void
egg_flow_box_real_selected_children_changed (EggFlowBox *box)
{
  _egg_flow_box_accessible_selection_changed (GTK_WIDGET (box));
}

static void
egg_flow_box_class_init (EggFlowBoxClass *class)
{
  GObjectClass      *object_class = G_OBJECT_CLASS (class);
  GtkWidgetClass    *widget_class = GTK_WIDGET_CLASS (class);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (class);
  GtkBindingSet     *binding_set;

  object_class->finalize = egg_flow_box_finalize;
  object_class->get_property = egg_flow_box_get_property;
  object_class->set_property = egg_flow_box_set_property;

  widget_class->enter_notify_event = egg_flow_box_real_enter_notify_event;
  widget_class->leave_notify_event = egg_flow_box_real_leave_notify_event;
  widget_class->motion_notify_event = egg_flow_box_real_motion_notify_event;
  widget_class->size_allocate = egg_flow_box_real_size_allocate;
  widget_class->realize = egg_flow_box_real_realize;
  widget_class->focus = egg_flow_box_real_focus;
  widget_class->draw = egg_flow_box_real_draw;
  widget_class->button_press_event = egg_flow_box_real_button_press_event;
  widget_class->button_release_event = egg_flow_box_real_button_release_event;
  widget_class->get_request_mode = egg_flow_box_real_get_request_mode;
  widget_class->get_preferred_width = egg_flow_box_real_get_preferred_width;
  widget_class->get_preferred_height = egg_flow_box_real_get_preferred_height;
  widget_class->get_preferred_height_for_width = egg_flow_box_real_get_preferred_height_for_width;
  widget_class->get_preferred_width_for_height = egg_flow_box_real_get_preferred_width_for_height;

  container_class->add = egg_flow_box_real_add;
  container_class->remove = egg_flow_box_real_remove;
  container_class->forall = egg_flow_box_real_forall;
  container_class->child_type = egg_flow_box_real_child_type;
  gtk_container_class_handle_border_width (container_class);

  class->activate_cursor_child = egg_flow_box_real_activate_cursor_child;
  class->toggle_cursor_child = egg_flow_box_real_toggle_cursor_child;
  class->move_cursor = egg_flow_box_real_move_cursor;
  class->select_all = egg_flow_box_select_all;
  class->unselect_all = egg_flow_box_unselect_all;
  class->selected_children_changed = egg_flow_box_real_selected_children_changed;

  g_object_class_override_property (object_class, PROP_ORIENTATION, "orientation");

  g_object_class_install_property (object_class,
                                   PROP_SELECTION_MODE,
                                   g_param_spec_enum ("selection-mode",
                                                      P_("Selection mode"),
                                                      P_("The selection mode"),
                                                      GTK_TYPE_SELECTION_MODE,
                                                      GTK_SELECTION_SINGLE,
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_ACTIVATE_ON_SINGLE_CLICK,
                                   g_param_spec_boolean ("activate-on-single-click",
                                                         P_("Activate on Single Click"),
                                                         P_("Activate row on a single click"),
                                                         TRUE,
                                                         G_PARAM_READWRITE));


  g_object_class_install_property (object_class,
                                   PROP_HOMOGENEOUS,
                                   g_param_spec_boolean ("homogeneous",
                                                         P_("Homogeneous"),
                                                         P_("Whether the children should all be the same size"),
                                                         FALSE,
                                                         G_PARAM_READWRITE));

  /**
   * EggFlowBox:halign-policy:
   *
   * The #GtkAlign to used to define what is done with extra
   * space in a given orientation.
   */
  g_object_class_install_property (object_class,
                                   PROP_HALIGN_POLICY,
                                   g_param_spec_enum ("halign-policy",
                                                      P_("Horizontal align policy"),
                                                      P_("The align policy horizontally"),
                                                      GTK_TYPE_ALIGN,
                                                      GTK_ALIGN_FILL,
                                                      G_PARAM_READWRITE));
  /**
   * EggFlowBox:valign-policy:
   *
   * The #GtkAlign to used to define what is done with extra
   * space in a given orientation.
   */
  g_object_class_install_property (object_class,
                                   PROP_VALIGN_POLICY,
                                   g_param_spec_enum ("valign-policy",
                                                      P_("Vertical align policy"),
                                                      P_("The align policy vertically"),
                                                      GTK_TYPE_ALIGN,
                                                      GTK_ALIGN_START,
                                                      G_PARAM_READWRITE));

  /**
   * EggFlowBox:min-children-per-line:
   *
   * The minimum number of children to allocate consecutively in the given orientation.
   *
   * <note><para>Setting the minimum children per line ensures
   * that a reasonably small height will be requested
   * for the overall minimum width of the box.</para></note>
   *
   */
  g_object_class_install_property (object_class,
                                   PROP_MIN_CHILDREN_PER_LINE,
                                   g_param_spec_uint ("min-children-per-line",
                                                      P_("Minimum Children Per Line"),
                                                      P_("The minimum number of children to allocate "
                                                         "consecutively in the given orientation."),
                                                      0,
                                                      G_MAXUINT,
                                                      0,
                                                      G_PARAM_READWRITE));

  /**
   * EggFlowBox:max-children-per-line:
   *
   * The maximum amount of children to request space for consecutively in the given orientation.
   *
   */
  g_object_class_install_property (object_class,
                                   PROP_MAX_CHILDREN_PER_LINE,
                                   g_param_spec_uint ("max-children-per-line",
                                                      P_("Maximum Children Per Line"),
                                                      P_("The maximum amount of children to request space for "
                                                         "consecutively in the given orientation."),
                                                      0,
                                                      G_MAXUINT,
                                                      DEFAULT_MAX_CHILDREN_PER_LINE,
                                                      G_PARAM_READWRITE));

  /**
   * EggFlowBox:vertical-spacing:
   *
   * The amount of vertical space between two children.
   *
   */
  g_object_class_install_property (object_class,
                                   PROP_ROW_SPACING,
                                   g_param_spec_uint ("vertical-spacing",
                                                      P_("Vertical spacing"),
                                                      P_("The amount of vertical space between two children"),
                                                      0,
                                                      G_MAXUINT,
                                                      0,
                                                      G_PARAM_READWRITE));

  /**
   * EggFlowBox:horizontal-spacing:
   *
   * The amount of horizontal space between two children.
   *
   */
  g_object_class_install_property (object_class,
                                   PROP_COLUMN_SPACING,
                                   g_param_spec_uint ("horizontal-spacing",
                                                      P_("Horizontal spacing"),
                                                      P_("The amount of horizontal space between two children"),
                                                      0,
                                                      G_MAXUINT,
                                                      0,
                                                      G_PARAM_READWRITE));

  signals[CHILD_ACTIVATED] = g_signal_new ("child-activated",
                                           EGG_TYPE_FLOW_BOX,
                                           G_SIGNAL_RUN_LAST,
                                           G_STRUCT_OFFSET (EggFlowBoxClass, child_activated),
                                           NULL, NULL,
                                           g_cclosure_marshal_VOID__OBJECT,
                                           G_TYPE_NONE, 1,
                                           GTK_TYPE_WIDGET);

  signals[SELECTED_CHILDREN_CHANGED] = g_signal_new ("selected-children-changed",
                                                     EGG_TYPE_FLOW_BOX,
                                                     G_SIGNAL_RUN_FIRST,
                                                     G_STRUCT_OFFSET (EggFlowBoxClass, selected_children_changed),
                                                     NULL, NULL,
                                                     g_cclosure_marshal_VOID__VOID,
                                                     G_TYPE_NONE, 0);
  signals[ACTIVATE_CURSOR_CHILD] = g_signal_new ("activate-cursor-child",
                                                 EGG_TYPE_FLOW_BOX,
                                                 G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                                 G_STRUCT_OFFSET (EggFlowBoxClass, activate_cursor_child),
                                                 NULL, NULL,
                                                 g_cclosure_marshal_VOID__VOID,
                                                 G_TYPE_NONE, 0);
  signals[TOGGLE_CURSOR_CHILD] = g_signal_new ("toggle-cursor-child",
                                               EGG_TYPE_FLOW_BOX,
                                               G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                               G_STRUCT_OFFSET (EggFlowBoxClass, toggle_cursor_child),
                                               NULL, NULL,
                                               g_cclosure_marshal_VOID__VOID,
                                               G_TYPE_NONE, 0);
  signals[MOVE_CURSOR] = g_signal_new ("move-cursor",
                                       EGG_TYPE_FLOW_BOX,
                                       G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                       G_STRUCT_OFFSET (EggFlowBoxClass, move_cursor),
                                       NULL, NULL,
                                       _egg_marshal_VOID__ENUM_INT,
                                       G_TYPE_NONE, 2,
                                       GTK_TYPE_MOVEMENT_STEP, G_TYPE_INT);
  signals[SELECT_ALL] = g_signal_new ("select-all",
                                      EGG_TYPE_FLOW_BOX,
                                      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                      G_STRUCT_OFFSET (EggFlowBoxClass, select_all),
                                      NULL, NULL,
                                      g_cclosure_marshal_VOID__VOID,
                                      G_TYPE_NONE, 0);
  signals[UNSELECT_ALL] = g_signal_new ("unselect-all",
                                      EGG_TYPE_FLOW_BOX,
                                      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                      G_STRUCT_OFFSET (EggFlowBoxClass, unselect_all),
                                      NULL, NULL,
                                      g_cclosure_marshal_VOID__VOID,
                                      G_TYPE_NONE, 0);

  widget_class->activate_signal = signals[ACTIVATE_CURSOR_CHILD];

  binding_set = gtk_binding_set_by_class (class);
  egg_flow_box_add_move_binding (binding_set, GDK_KEY_Home, 0,
                                 GTK_MOVEMENT_BUFFER_ENDS, -1);
  egg_flow_box_add_move_binding (binding_set, GDK_KEY_KP_Home, 0,
                                 GTK_MOVEMENT_BUFFER_ENDS, -1);
  egg_flow_box_add_move_binding (binding_set, GDK_KEY_End, 0,
                                 GTK_MOVEMENT_BUFFER_ENDS, 1);
  egg_flow_box_add_move_binding (binding_set, GDK_KEY_KP_End, 0,
                                 GTK_MOVEMENT_BUFFER_ENDS, 1);
  egg_flow_box_add_move_binding (binding_set, GDK_KEY_Up, 0,
                                 GTK_MOVEMENT_DISPLAY_LINES, -1);
  egg_flow_box_add_move_binding (binding_set, GDK_KEY_KP_Up, 0,
                                 GTK_MOVEMENT_DISPLAY_LINES, -1);
  egg_flow_box_add_move_binding (binding_set, GDK_KEY_Down, 0,
                                 GTK_MOVEMENT_DISPLAY_LINES, 1);
  egg_flow_box_add_move_binding (binding_set, GDK_KEY_KP_Down, 0,
                                 GTK_MOVEMENT_DISPLAY_LINES, 1);
  egg_flow_box_add_move_binding (binding_set, GDK_KEY_Page_Up, 0,
                                 GTK_MOVEMENT_PAGES, -1);
  egg_flow_box_add_move_binding (binding_set, GDK_KEY_KP_Page_Up, 0,
                                 GTK_MOVEMENT_PAGES, -1);
  egg_flow_box_add_move_binding (binding_set, GDK_KEY_Page_Down, 0,
                                 GTK_MOVEMENT_PAGES, 1);
  egg_flow_box_add_move_binding (binding_set, GDK_KEY_KP_Page_Down, 0,
                                 GTK_MOVEMENT_PAGES, 1);

  egg_flow_box_add_move_binding (binding_set, GDK_KEY_Right, 0,
                                 GTK_MOVEMENT_VISUAL_POSITIONS, 1);
  egg_flow_box_add_move_binding (binding_set, GDK_KEY_KP_Right, 0,
                                 GTK_MOVEMENT_VISUAL_POSITIONS, 1);
  egg_flow_box_add_move_binding (binding_set, GDK_KEY_Left, 0,
                                 GTK_MOVEMENT_VISUAL_POSITIONS, -1);
  egg_flow_box_add_move_binding (binding_set, GDK_KEY_KP_Left, 0,
                                 GTK_MOVEMENT_VISUAL_POSITIONS, -1);

  gtk_binding_entry_add_signal (binding_set, GDK_KEY_space, GDK_CONTROL_MASK,
                                "toggle-cursor-child", 0, NULL);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_KP_Space, GDK_CONTROL_MASK,
                                "toggle-cursor-child", 0, NULL);

  gtk_binding_entry_add_signal (binding_set, GDK_KEY_a, GDK_CONTROL_MASK,
                                "select-all", 0);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_a, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
                                "unselect-all", 0);

  g_type_class_add_private (class, sizeof (EggFlowBoxPrivate));

  gtk_widget_class_set_accessible_type (widget_class, EGG_TYPE_FLOW_BOX_ACCESSIBLE);
}

static void
egg_flow_box_init (EggFlowBox *box)
{
  EggFlowBoxPrivate *priv;

  box->priv = priv = G_TYPE_INSTANCE_GET_PRIVATE (box, EGG_TYPE_FLOW_BOX, EggFlowBoxPrivate);

  priv->orientation = GTK_ORIENTATION_HORIZONTAL;
  priv->selection_mode = GTK_SELECTION_SINGLE;
  priv->halign_policy = GTK_ALIGN_FILL;
  priv->valign_policy = GTK_ALIGN_START;
  priv->max_children_per_line = DEFAULT_MAX_CHILDREN_PER_LINE;
  priv->column_spacing = 0;
  priv->row_spacing = 0;
  priv->children = g_sequence_new ((GDestroyNotify)egg_flow_box_child_info_free);
  priv->child_hash = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, NULL);
  priv->activate_on_single_click = TRUE;

  gtk_widget_set_can_focus (GTK_WIDGET (box), TRUE);
  gtk_widget_set_has_window (GTK_WIDGET (box), TRUE);
}

/**
 * egg_flow_box_new:
 *
 * Creates an #EggFlowBox.
 *
 * Returns: A new #EggFlowBox container
 */
GtkWidget *
egg_flow_box_new (void)
{
  return (GtkWidget *)g_object_new (EGG_TYPE_FLOW_BOX, NULL);
}

/**
 * egg_flow_box_get_selected_children:
 * @box: An #EggFlowBox.
 *
 * Creates a list of all selected children.
 *
 * Return value: (element-type GtkWidget) (transfer container): A #GList containing the #GtkWidget for each selected child.
 **/
GList *
egg_flow_box_get_selected_children (EggFlowBox *box)
{
  EggFlowBoxChildInfo *child_info;
  GSequenceIter *iter;
  GList *selected = NULL;

  g_return_val_if_fail (box != NULL, NULL);

  for (iter = g_sequence_get_begin_iter (box->priv->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      child_info = g_sequence_get (iter);
      if (child_info->selected)
        selected = g_list_prepend (selected, child_info->widget);
    }

  return g_list_reverse (selected);
}

void
egg_flow_box_select_child (EggFlowBox *box,
                           GtkWidget  *child)
{
  EggFlowBoxChildInfo *child_info;

  g_return_if_fail (EGG_IS_FLOW_BOX (box));
  g_return_if_fail (child != NULL);

  child_info = egg_flow_box_lookup_info (box, child);
  if (child_info == NULL)
    {
      g_warning ("Tried to select non-child %p\n", child);
      return;
    }

  egg_flow_box_select_child_info (box, child_info);
}

void
egg_flow_box_unselect_child (EggFlowBox *box,
                             GtkWidget  *child)
{
  EggFlowBoxChildInfo *child_info;

  g_return_if_fail (EGG_IS_FLOW_BOX (box));
  g_return_if_fail (child != NULL);

  child_info = egg_flow_box_lookup_info (box, child);
  if (child_info == NULL)
    {
      g_warning ("Tried to unselect non-child %p\n", child);
      return;
    }

  egg_flow_box_unselect_child_info (box, child_info);
}

gboolean
egg_flow_box_is_child_selected (EggFlowBox *box,
                                GtkWidget  *child)
{
  EggFlowBoxChildInfo *child_info;

  child_info = egg_flow_box_lookup_info (box, child);
  if (child_info == NULL)
    {
      g_warning ("Tried to obtain selection status of non-child %p\n", child);
      return FALSE;
    }

  return child_info->selected;
}

/**
 * egg_flow_box_selected_foreach:
 * @box: An #EggFlowBox.
 * @func: (scope call): The function to call for each selected child.
 * @data: User data to pass to the function.
 *
 * Calls a function for each selected child. Note that the
 * selection cannot be modified from within this function.
 */
void
egg_flow_box_selected_foreach (EggFlowBox           *box,
                               EggFlowBoxForeachFunc func,
                               gpointer              data)
{
  EggFlowBoxChildInfo *child_info;
  GSequenceIter *iter;

  g_return_if_fail (EGG_IS_FLOW_BOX (box));

  for (iter = g_sequence_get_begin_iter (box->priv->children);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      child_info = g_sequence_get (iter);
      if (child_info->selected)
        (* func) (box, child_info->widget, data);
    }
}

void
egg_flow_box_set_selection_mode (EggFlowBox *box,
                                 GtkSelectionMode mode)
{
  gboolean dirty = FALSE;
  g_return_if_fail (EGG_IS_FLOW_BOX (box));

  if (mode == box->priv->selection_mode)
    return;

  if (mode == GTK_SELECTION_NONE ||
      box->priv->selection_mode == GTK_SELECTION_MULTIPLE)
    dirty = egg_flow_box_unselect_all_internal (box);

  box->priv->selection_mode = mode;

  g_object_notify (G_OBJECT (box), "selection-mode");

  if (dirty)
    g_signal_emit (box, signals[SELECTED_CHILDREN_CHANGED], 0);
}

GtkSelectionMode
egg_flow_box_get_selection_mode (EggFlowBox *box)
{
  g_return_val_if_fail (EGG_IS_FLOW_BOX (box), GTK_SELECTION_SINGLE);

  return box->priv->selection_mode;
}
