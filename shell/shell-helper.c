/*
 * Copyright (C) 2014 Collabora Ltd.
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
 * Author: Jonny Lamb <jonny.lamb@collabora.co.uk>
 */

#include <stdio.h>
#include <assert.h>

#include "config.h"
#ifdef HAVE_NEW_WESTON
#include <libweston-1/compositor.h>
#else
#include <weston/compositor.h>
#endif

#include "shell-helper-server-protocol.h"

#ifndef container_of
#define container_of(ptr, type, member) ({                              \
        const __typeof__( ((type *)0)->member ) *__mptr = (ptr);        \
        (type *)( (char *)__mptr - offsetof(type,member) );})
#endif

struct shell_helper {
	struct weston_compositor *compositor;

	struct wl_listener destroy_listener;

	struct weston_layer *panel_layer;

	struct weston_layer curtain_layer;
	struct weston_view *curtain_view;
	struct weston_view_animation *curtain_animation;
	uint32_t curtain_show;

	struct wl_list slide_list;
};

static void
shell_helper_move_surface(struct wl_client *client,
			  struct wl_resource *resource,
			  struct wl_resource *surface_resource,
			  int32_t x,
			  int32_t y)
{
	struct shell_helper *helper = wl_resource_get_user_data(resource);
	struct weston_surface *surface =
		wl_resource_get_user_data(surface_resource);
	struct weston_view *view;

	view = container_of(surface->views.next, struct weston_view, surface_link);

	if (!view)
		return;

	weston_view_set_position(view, x, y);
	weston_view_update_transform(view);
}

static void
configure_surface(struct weston_surface *es, int32_t sx, int32_t sy)
{
	#ifdef HAVE_NEW_WESTON
	struct weston_view *existing_view = es->committed_private;
	#else
	struct weston_view *existing_view = es->configure_private;
	#endif
	struct weston_view *new_view;

	new_view = container_of(es->views.next, struct weston_view, surface_link);

	if (wl_list_empty(&new_view->layer_link.link)) {
		/* be sure to append to the list, not insert */
		weston_layer_entry_insert(&existing_view->layer_link, &new_view->layer_link);
		weston_compositor_schedule_repaint(es->compositor);
	}
}

static void
shell_helper_add_surface_to_layer(struct wl_client *client,
				  struct wl_resource *resource,
				  struct wl_resource *new_surface_resource,
				  struct wl_resource *existing_surface_resource)
{
	struct shell_helper *helper = wl_resource_get_user_data(resource);
	struct weston_surface *new_surface =
		wl_resource_get_user_data(new_surface_resource);
	struct weston_surface *existing_surface =
		wl_resource_get_user_data(existing_surface_resource);
	struct weston_view *new_view, *existing_view, *next;
	struct wl_layer *layer;

	#ifdef HAVE_NEW_WESTON
	if (new_surface->committed) {
		wl_resource_post_error(new_surface_resource,
				       WL_DISPLAY_ERROR_INVALID_OBJECT,
				       "surface role already assigned");
		return;
	}
	#else
	if (new_surface->configure) {
		wl_resource_post_error(new_surface_resource,
				       WL_DISPLAY_ERROR_INVALID_OBJECT,
				       "surface role already assigned");
		return;
	}
	#endif

	existing_view = container_of(existing_surface->views.next,
				     struct weston_view,
				     surface_link);

	wl_list_for_each_safe(new_view, next, &new_surface->views, surface_link)
		weston_view_destroy(new_view);
	new_view = weston_view_create(new_surface);

	#ifdef HAVE_NEW_WESTON
	new_surface->committed = configure_surface;
	new_surface->committed_private = existing_view;
	#else
	new_surface->configure = configure_surface;
	new_surface->configure_private = existing_view;
	#endif
	new_surface->output = existing_view->output;
}

static void
configure_panel(struct weston_surface *es, int32_t sx, int32_t sy)
{
	#ifdef HAVE_NEW_WESTON
	struct shell_helper *helper = es->committed_private;
	#else
	struct shell_helper *helper = es->configure_private;
	#endif
	struct weston_view *view;

	view = container_of(es->views.next, struct weston_view, surface_link);

	if (wl_list_empty(&view->layer_link.link)) {
		weston_layer_entry_insert(&helper->panel_layer->view_list, &view->layer_link);
		weston_compositor_schedule_repaint(es->compositor);
	}
}

static void
shell_helper_set_panel(struct wl_client *client,
		       struct wl_resource *resource,
		       struct wl_resource *surface_resource)
{
	struct shell_helper *helper = wl_resource_get_user_data(resource);
	struct weston_surface *surface =
		wl_resource_get_user_data(surface_resource);
	struct weston_view *view = container_of(surface->views.next,
						struct weston_view,
						surface_link);

	/* we need to save the panel's layer so we can use it later on, but
	 * it hasn't yet been defined because the original surface configure
	 * function hasn't yet been called. if we call it here we will have
	 * access to the layer. */
	#ifdef HAVE_NEW_WESTON
	surface->committed(surface, 0, 0);
	#else
	surface->configure(surface, 0, 0);
	#endif

	helper->panel_layer = container_of(view->layer_link.link.next,
					   struct weston_layer,
					   view_list.link);

	/* set new configure functions that only ensure the surface is in the
	 * correct layer. */
	#ifdef HAVE_NEW_WESTON
	surface->committed = configure_panel;
	surface->committed_private = helper;
	#else
	surface->configure = configure_panel;
	surface->configure_private = helper;
	#endif
}

enum SlideState {
	SLIDE_STATE_NONE,
	SLIDE_STATE_SLIDING_OUT,
	SLIDE_STATE_OUT,
	SLIDE_STATE_SLIDING_BACK,
	SLIDE_STATE_BACK
};

enum SlideRequest {
	SLIDE_REQUEST_NONE,
	SLIDE_REQUEST_OUT,
	SLIDE_REQUEST_BACK
};

struct slide {
	struct weston_surface *surface;
	struct weston_view *view;
	int x;
	int y;

	enum SlideState state;
	enum SlideRequest request;

	struct weston_transform transform;

	struct wl_list link;
};

static void slide_back(struct slide *slide);

static void
slide_done_cb(struct weston_view_animation *animation, void *data)
{
	struct slide *slide = data;

	slide->state = SLIDE_STATE_OUT;

	wl_list_insert(&slide->view->transform.position.link,
		       &slide->transform.link);
	weston_matrix_init(&slide->transform.matrix);
	weston_matrix_translate(&slide->transform.matrix,
				slide->x,
				slide->y,
				0);

	weston_view_geometry_dirty(slide->view);
	weston_compositor_schedule_repaint(slide->surface->compositor);

	if (slide->request == SLIDE_REQUEST_BACK) {
		slide->request = SLIDE_REQUEST_NONE;
		slide_back(slide);
	}
}

static void
slide_out(struct slide *slide)
{
	assert(slide->state == SLIDE_STATE_NONE || slide->state == SLIDE_STATE_BACK);

	slide->state = SLIDE_STATE_SLIDING_OUT;

	weston_move_scale_run(slide->view, slide->x, slide->y,
			      1.0, 1.0, 0,
			      slide_done_cb, slide);
}

static void
slide_back_done_cb(struct weston_view_animation *animation, void *data)
{
	struct slide *slide = data;

	slide->state = SLIDE_STATE_BACK;

	wl_list_remove(&slide->transform.link);
	weston_view_geometry_dirty(slide->view);

	if (slide->request == SLIDE_REQUEST_OUT) {
		slide->request = SLIDE_REQUEST_NONE;
		slide_out(slide);
	} else {
		wl_list_remove(&slide->link);
		free(slide);
	}
}

static void
slide_back(struct slide *slide)
{
	assert(slide->state == SLIDE_STATE_OUT);

	slide->state = SLIDE_STATE_SLIDING_BACK;

	weston_move_scale_run(slide->view, -slide->x, -slide->y,
			      1.0, 1.0, 0,
			      slide_back_done_cb, slide);
}

static void
shell_helper_slide_surface(struct wl_client *client,
			   struct wl_resource *resource,
			   struct wl_resource *surface_resource,
			   int32_t x,
			   int32_t y)
{
	struct shell_helper *helper = wl_resource_get_user_data(resource);
	struct weston_surface *surface =
		wl_resource_get_user_data(surface_resource);
	struct weston_view *view;
	struct slide *slide;

	wl_list_for_each(slide, &helper->slide_list, link) {
		if (slide->surface == surface) {
			if (slide->state == SLIDE_STATE_SLIDING_BACK)
				slide->request = SLIDE_REQUEST_OUT;
			return;
		}
	}

	view = container_of(surface->views.next, struct weston_view, surface_link);

	if (!view)
		return;

	slide = malloc(sizeof *slide);
	if (!slide)
		return;

	slide->surface = surface;
	slide->view = view;
	slide->x = x;
	slide->y = y;

	slide->state = SLIDE_STATE_NONE;
	slide->request = SLIDE_REQUEST_NONE;

	wl_list_insert(&helper->slide_list,
		       &slide->link);

	slide_out(slide);
}

static void
shell_helper_slide_surface_back(struct wl_client *client,
				struct wl_resource *resource,
				struct wl_resource *surface_resource)
{
	struct shell_helper *helper = wl_resource_get_user_data(resource);
	struct weston_surface *surface =
		wl_resource_get_user_data(surface_resource);
	struct weston_view *view;
	int found = 0;
	struct slide *slide;

	wl_list_for_each(slide, &helper->slide_list, link) {
		if (slide->surface == surface) {
			found = 1;
			break;
		}
	}

	if (!found || slide->state == SLIDE_STATE_SLIDING_BACK)
		return;

	if (slide->state == SLIDE_STATE_SLIDING_OUT)
		slide->request = SLIDE_REQUEST_BACK;
	else
		slide_back(slide);
}

/* mostly copied from weston's desktop-shell/shell.c */
static struct weston_view *
shell_curtain_create_view(struct shell_helper *helper,
			  struct weston_surface *surface)
{
	struct weston_view *view;

	if (!surface)
		return NULL;

	view = weston_view_create(surface);
	if (!view) {
		return NULL;
	}

	weston_view_set_position(view, 0, 0);
	weston_surface_set_color(surface, 0.0, 0.0, 0.0, 0.7);
	weston_layer_entry_insert(&helper->curtain_layer.view_list,
				  &view->layer_link);
	pixman_region32_init_rect(&surface->input, 0, 0,
	                          surface->width,
	                          surface->height);

	return view;
}

static void
curtain_done_hide(struct weston_view_animation *animation,
		  void *data);

static void
curtain_fade_done(struct weston_view_animation *animation,
		  void *data)
{
	struct shell_helper *helper = data;

	if (!helper->curtain_show)
		wl_list_remove(&helper->curtain_layer.link);

	helper->curtain_animation = NULL;
}

static void
shell_helper_curtain(struct wl_client *client,
		     struct wl_resource *resource,
		     struct wl_resource *surface_resource,
		     int32_t show)
{
	struct shell_helper *helper = wl_resource_get_user_data(resource);
	struct weston_surface *surface =
		wl_resource_get_user_data(surface_resource);

	helper->curtain_show = show;

	if (show) {
		if (helper->curtain_animation) {
			weston_fade_update(helper->curtain_animation, 0.7);
			return;
		}

		if (!helper->curtain_view) {
			weston_layer_init(&helper->curtain_layer,
					  &helper->panel_layer->link);

			helper->curtain_view = shell_curtain_create_view(helper, surface);

			/* we need to assign an output to the view before we can
			* fade it in */
			weston_view_geometry_dirty(helper->curtain_view);
			weston_view_update_transform(helper->curtain_view);
		} else {
			wl_list_insert(&helper->panel_layer->link, &helper->curtain_layer.link);
		}

		helper->curtain_animation = weston_fade_run(
			helper->curtain_view,
			0.0, 0.7, 400,
			curtain_fade_done, helper);

	} else {
		if (helper->curtain_animation) {
			weston_fade_update(helper->curtain_animation, 0.0);
			return;
		}

		/* should never happen in theory */
		if (!helper->curtain_view)
			return;

		helper->curtain_animation = weston_fade_run(
			helper->curtain_view,
			0.7, 0.0, 400,
			curtain_fade_done, helper);
	}
}

static const struct shell_helper_interface helper_implementation = {
	shell_helper_move_surface,
	shell_helper_add_surface_to_layer,
	shell_helper_set_panel,
	shell_helper_slide_surface,
	shell_helper_slide_surface_back,
	shell_helper_curtain
};

static void
bind_helper(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct shell_helper *helper = data;
	struct wl_resource *resource;

	resource = wl_resource_create(client, &shell_helper_interface, 1, id);
	if (resource)
		wl_resource_set_implementation(resource, &helper_implementation,
					       helper, NULL);
}

static void
helper_destroy(struct wl_listener *listener, void *data)
{
	struct shell_helper *helper =
		container_of(listener, struct shell_helper, destroy_listener);

	free(helper);
}

WL_EXPORT int
module_init(struct weston_compositor *ec,
	    int *argc, char *argv[])
{
	struct shell_helper *helper;

	helper = zalloc(sizeof *helper);
	if (helper == NULL)
		return -1;

	helper->compositor = ec;
	helper->panel_layer = NULL;
	helper->curtain_view = NULL;
	helper->curtain_show = 0;

	wl_list_init(&helper->slide_list);

	helper->destroy_listener.notify = helper_destroy;
	wl_signal_add(&ec->destroy_signal, &helper->destroy_listener);

	if (wl_global_create(ec->wl_display, &shell_helper_interface, 1,
			     helper, bind_helper) == NULL)
		return -1;

	return 0;
}
