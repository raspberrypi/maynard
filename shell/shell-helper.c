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

#include <weston/compositor.h>

#include "shell-helper-server-protocol.h"

struct shell_helper {
	struct weston_compositor *compositor;

	struct wl_listener destroy_listener;

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

static const struct shell_helper_interface helper_implementation = {
	shell_helper_move_surface,
	shell_helper_slide_surface,
	shell_helper_slide_surface_back
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

	wl_list_init(&helper->slide_list);

	helper->destroy_listener.notify = helper_destroy;
	wl_signal_add(&ec->destroy_signal, &helper->destroy_listener);

	if (wl_global_create(ec->wl_display, &shell_helper_interface, 1,
			     helper, bind_helper) == NULL)
		return -1;

	return 0;
}
