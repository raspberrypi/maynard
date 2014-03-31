/*
 * Copyright (c) 2013 Tiago Vignatti
 * Copyright (c) 2013-2014 Collabora Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkwayland.h>

#include "desktop-shell-client-protocol.h"
#include "shell-helper-client-protocol.h"

#include "maynard-resources.h"

#include "app-icon.h"
#include "clock.h"
#include "favorites.h"
#include "launcher.h"
#include "panel.h"
#include "vertical-clock.h"

extern char **environ; /* defined by libc */

#define DEFAULT_BACKGROUND "/usr/share/themes/Adwaita/backgrounds/morning.jpg""

struct element {
	GtkWidget *window;
	GdkPixbuf *pixbuf;
	struct wl_surface *surface;
};

struct desktop {
	struct wl_display *display;
	struct wl_registry *registry;
	struct desktop_shell *shell;
	struct wl_output *output;
	struct shell_helper *helper;

	GdkDisplay *gdk_display;

	struct element *background;
	struct element *panel;
	struct element *launcher_grid;
	struct element *clock;

	guint initial_panel_timeout_id;
	guint hide_panel_idle_id;

	gboolean grid_visible;
	gboolean volume_visible;
};

static gboolean panel_window_enter_cb (GtkWidget *widget,
				       GdkEventCrossing *event,
				       struct desktop *desktop);
static gboolean panel_window_leave_cb (GtkWidget *widget,
				       GdkEventCrossing *event,
				       struct desktop *desktop);

static gboolean
connect_enter_leave_signals (gpointer data)
{
	struct desktop *desktop = data;
	GList *l;

	g_signal_connect (desktop->panel->window, "enter-notify-event",
			  G_CALLBACK (panel_window_enter_cb), desktop);
	g_signal_connect (desktop->panel->window, "leave-notify-event",
			  G_CALLBACK (panel_window_leave_cb), desktop);

	g_signal_connect (desktop->clock->window, "enter-notify-event",
			  G_CALLBACK (panel_window_enter_cb), desktop);
	g_signal_connect (desktop->clock->window, "leave-notify-event",
			  G_CALLBACK (panel_window_leave_cb), desktop);

	g_signal_connect (desktop->launcher_grid->window, "enter-notify-event",
			  G_CALLBACK (panel_window_enter_cb), desktop);
	g_signal_connect (desktop->launcher_grid->window, "leave-notify-event",
			  G_CALLBACK (panel_window_leave_cb), desktop);

	return G_SOURCE_REMOVE;
}

static void
desktop_shell_configure(void *data,
		struct desktop_shell *desktop_shell,
		uint32_t edges,
		struct wl_surface *surface,
		int32_t width, int32_t height)
{
	struct desktop *desktop = data;
	int window_height;
	int grid_width, grid_height;

	gtk_widget_set_size_request (desktop->background->window,
				     width, height);

	/* TODO: make this height a little nicer */
	window_height = height * MAYNARD_PANEL_HEIGHT_RATIO;
	gtk_window_resize (GTK_WINDOW (desktop->panel->window),
			   MAYNARD_PANEL_WIDTH, window_height);

	maynard_launcher_calculate (MAYNARD_LAUNCHER(desktop->launcher_grid->window),
				       &grid_width, &grid_height, NULL);
	gtk_widget_set_size_request (desktop->launcher_grid->window,
				     grid_width,
				     grid_height);

	shell_helper_move_surface(desktop->helper,
				  desktop->panel->surface,
				  0, (height - window_height) / 2);

	gtk_window_resize (GTK_WINDOW (desktop->clock->window),
			   MAYNARD_CLOCK_WIDTH,
			   MAYNARD_CLOCK_HEIGHT);

	shell_helper_move_surface(desktop->helper,
				  desktop->clock->surface,
				  MAYNARD_PANEL_WIDTH,
				  (height - window_height) / 2);

	shell_helper_move_surface(desktop->helper,
				  desktop->launcher_grid->surface,
				  /* TODO: massive hack so window is always onscreen */
				  - grid_width + 1,
				  ((height - window_height) / 2) + MAYNARD_CLOCK_HEIGHT);

	desktop_shell_desktop_ready(desktop->shell);

	/* TODO: why does the panel signal leave on drawing for
	 * startup? we don't want to have to have this silly
	 * timeout. */
	g_timeout_add_seconds (1, connect_enter_leave_signals, desktop);
}

static void
desktop_shell_prepare_lock_surface(void *data,
		struct desktop_shell *desktop_shell)
{
	desktop_shell_unlock (desktop_shell);
}

static void
desktop_shell_grab_cursor(void *data, struct desktop_shell *desktop_shell,
		uint32_t cursor)
{
}

static const struct desktop_shell_listener listener = {
	desktop_shell_configure,
	desktop_shell_prepare_lock_surface,
	desktop_shell_grab_cursor
};

static void
launcher_grid_toggle (GtkWidget *widget, struct desktop *desktop)
{
	if (desktop->grid_visible) {
		shell_helper_slide_surface_back(desktop->helper,
						desktop->launcher_grid->surface);
	} else {
		int width;

		gtk_widget_get_size_request (desktop->launcher_grid->window,
					     &width, NULL);

		shell_helper_slide_surface(desktop->helper,
					   desktop->launcher_grid->surface,
					   /* TODO: massive hack so window is always onscreen */
					   width + MAYNARD_PANEL_WIDTH - 1, 0);
	}

	desktop->grid_visible = !desktop->grid_visible;
}

static void
launcher_grid_create (struct desktop *desktop)
{
	struct element *launcher_grid;
	GdkWindow *gdk_window;

	launcher_grid = malloc (sizeof *launcher_grid);
	memset (launcher_grid, 0, sizeof *launcher_grid);

	launcher_grid->window = maynard_launcher_new (desktop->background->window);
	gdk_window = gtk_widget_get_window(launcher_grid->window);
	launcher_grid->surface = gdk_wayland_window_get_wl_surface(gdk_window);

	gdk_wayland_window_set_use_custom_surface(gdk_window);
	shell_helper_add_surface_to_layer(desktop->helper,
					  launcher_grid->surface,
					  desktop->panel->surface);

	g_signal_connect(launcher_grid->window, "app-launched",
			 G_CALLBACK(launcher_grid_toggle), desktop);

	gtk_widget_show_all(launcher_grid->window);

	desktop->launcher_grid = launcher_grid;
}

static void
volume_changed_cb (MaynardClock *clock,
		   gdouble value,
		   const gchar *icon_name,
		   struct desktop *desktop)
{
	maynard_panel_set_volume_icon_name (
		MAYNARD_PANEL (desktop->panel->window), icon_name);
}

static GtkWidget *
clock_create (struct desktop *desktop)
{
	struct element *clock;
	GdkWindow *gdk_window;

	clock = malloc(sizeof *clock);
	memset(clock, 0, sizeof *clock);

	clock->window = maynard_clock_new();

	g_signal_connect (clock->window, "volume-changed",
			  G_CALLBACK (volume_changed_cb), desktop);

	gdk_window = gtk_widget_get_window(clock->window);
	clock->surface = gdk_wayland_window_get_wl_surface(gdk_window);

	gdk_wayland_window_set_use_custom_surface(gdk_window);
	shell_helper_add_surface_to_layer(desktop->helper, clock->surface,
					  desktop->panel->surface);

	gtk_widget_show_all (clock->window);

	desktop->clock = clock;
}

static void
volume_toggled_cb (GtkWidget *widget,
		   struct desktop *desktop)
{
	desktop->volume_visible = !desktop->volume_visible;

	maynard_clock_show_volume (MAYNARD_CLOCK (desktop->clock->window),
				   desktop->volume_visible);
	maynard_panel_show_volume_previous (MAYNARD_PANEL (desktop->panel->window),
					    desktop->volume_visible);
}

static gboolean
panel_window_enter_cb (GtkWidget *widget,
		       GdkEventCrossing *event,
		       struct desktop *desktop)
{
	if (desktop->initial_panel_timeout_id > 0) {
		g_source_remove (desktop->initial_panel_timeout_id);
		desktop->initial_panel_timeout_id = 0;
	}

	if (desktop->hide_panel_idle_id > 0) {
		g_source_remove (desktop->hide_panel_idle_id);
		desktop->hide_panel_idle_id = 0;
		return;
	}

	shell_helper_slide_surface_back(desktop->helper,
					desktop->panel->surface);
	shell_helper_slide_surface_back(desktop->helper,
					desktop->clock->surface);

	maynard_panel_set_expand(MAYNARD_PANEL(desktop->panel->window),
				    TRUE);

	return FALSE;
}

static gboolean
leave_panel_idle_cb (gpointer data)
{
	struct desktop *desktop = data;
	gint width;

	desktop->hide_panel_idle_id = 0;

	gtk_window_get_size (GTK_WINDOW (desktop->clock->window),
			     &width, NULL);

	shell_helper_slide_surface(desktop->helper,
				   desktop->panel->surface,
				   MAYNARD_VERTICAL_CLOCK_WIDTH - MAYNARD_PANEL_WIDTH, 0);
	shell_helper_slide_surface(desktop->helper,
				   desktop->clock->surface,
				   MAYNARD_VERTICAL_CLOCK_WIDTH - MAYNARD_PANEL_WIDTH - width, 0);

	maynard_panel_set_expand(MAYNARD_PANEL(desktop->panel->window),
				 FALSE);

	maynard_clock_show_volume (MAYNARD_CLOCK (desktop->clock->window), FALSE);
	maynard_panel_show_volume_previous (MAYNARD_PANEL (desktop->panel->window), FALSE);
	desktop->volume_visible = FALSE;

	return G_SOURCE_REMOVE;
}

static gboolean
panel_window_leave_cb (GtkWidget *widget,
		       GdkEventCrossing *event,
		       struct desktop *desktop)
{
	if (desktop->initial_panel_timeout_id > 0) {
		g_source_remove (desktop->initial_panel_timeout_id);
		desktop->initial_panel_timeout_id = 0;
	}

	if (desktop->hide_panel_idle_id > 0)
		return;

	if (desktop->grid_visible)
		return;

	desktop->hide_panel_idle_id = g_idle_add (leave_panel_idle_cb, desktop);

	return FALSE;
}

static gboolean
panel_hide_timeout_cb (gpointer data)
{
	struct desktop *desktop = data;

	panel_window_leave_cb (NULL, NULL, desktop);

	return G_SOURCE_REMOVE;
}

static void
panel_create(struct desktop *desktop)
{
	struct element *panel;
	GdkWindow *gdk_window;

	panel = malloc(sizeof *panel);
	memset(panel, 0, sizeof *panel);

	panel->window = maynard_panel_new();

	g_signal_connect(panel->window, "app-menu-toggled",
			 G_CALLBACK(launcher_grid_toggle), desktop);
	g_signal_connect(panel->window, "volume-toggled",
			 G_CALLBACK(volume_toggled_cb), desktop);

	desktop->initial_panel_timeout_id =
		g_timeout_add_seconds(2, panel_hide_timeout_cb, desktop);

	/* set it up as the panel */
	gdk_window = gtk_widget_get_window(panel->window);
	gdk_wayland_window_set_use_custom_surface(gdk_window);

	panel->surface = gdk_wayland_window_get_wl_surface(gdk_window);
	desktop_shell_set_user_data(desktop->shell, desktop);
	desktop_shell_set_panel(desktop->shell, desktop->output,
				panel->surface);
	desktop_shell_set_panel_position(desktop->shell,
					 DESKTOP_SHELL_PANEL_POSITION_LEFT);

	gtk_widget_show_all(panel->window);

	desktop->panel = panel;
}

/* Expose callback for the drawing area */
static gboolean
draw_cb (GtkWidget *widget, cairo_t *cr, gpointer data)
{
	struct desktop *desktop = data;

	gdk_cairo_set_source_pixbuf (cr, desktop->background->pixbuf, 0, 0);
	cairo_paint (cr);

	return TRUE;
}

/* Destroy handler for the window */
static void
destroy_cb (GObject *object, gpointer data)
{
	gtk_main_quit ();
}

static void
background_create(struct desktop *desktop)
{
	GdkWindow *gdk_window;
	struct element *background;
	const gchar *filename;

	background = malloc(sizeof *background);
	memset(background, 0, sizeof *background);

	filename = g_getenv("BACKGROUND");
	if (filename == NULL)
		filename = DEFAULT_BACKGROUND;
	background->pixbuf = gdk_pixbuf_new_from_file (filename, NULL);
	if (!background->pixbuf) {
		g_message ("Could not load background.");
		exit (EXIT_FAILURE);
	}

	background->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	g_signal_connect (background->window, "destroy",
			  G_CALLBACK (destroy_cb), NULL);

	g_signal_connect (background->window, "draw",
			  G_CALLBACK (draw_cb), desktop);

	gtk_window_set_title(GTK_WINDOW(background->window), "maynard");
	gtk_window_set_decorated(GTK_WINDOW(background->window), FALSE);
	gtk_widget_realize(background->window);

	gdk_window = gtk_widget_get_window(background->window);
	gdk_wayland_window_set_use_custom_surface(gdk_window);

	background->surface = gdk_wayland_window_get_wl_surface(gdk_window);
	desktop_shell_set_user_data(desktop->shell, desktop);
	desktop_shell_set_background(desktop->shell, desktop->output,
		background->surface);

	desktop->background = background;

	gtk_widget_show_all(background->window);
}

static void
css_setup(struct desktop *desktop)
{
	GtkCssProvider *provider;
	GFile *file;
	GError *error = NULL;

	provider = gtk_css_provider_new ();

	file = g_file_new_for_uri ("resource:///org/raspberry-pi/maynard/style.css");

	if (!gtk_css_provider_load_from_file (provider, file, &error)) {
		g_warning ("Failed to load CSS file: %s", error->message);
		g_clear_error (&error);
		g_object_unref (file);
		return;
	}

	gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
						   GTK_STYLE_PROVIDER (provider), 600);

	g_object_unref (file);
}

static void
registry_handle_global(void *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version)
{
	struct desktop *d = data;

	if (!strcmp(interface, "desktop_shell")) {
		d->shell = wl_registry_bind(registry, name,
				&desktop_shell_interface, 3);
		desktop_shell_add_listener(d->shell, &listener, d);
	} else if (!strcmp(interface, "wl_output")) {

		/* TODO: create multiple outputs */
		d->output = wl_registry_bind(registry, name,
					     &wl_output_interface, 1);
	} else if (!strcmp(interface, "shell_helper")) {
		d->helper = wl_registry_bind(registry, name,
					     &shell_helper_interface, 1);
	}
}

static void
registry_handle_global_remove(void *data, struct wl_registry *registry,
		uint32_t name)
{
}

static const struct wl_registry_listener registry_listener = {
	registry_handle_global,
	registry_handle_global_remove
};

int
main(int argc, char *argv[])
{
	struct desktop *desktop;

	gdk_set_allowed_backends ("wayland");

	gtk_init(&argc, &argv);

	g_resources_register (maynard_get_resource ());

	desktop = malloc(sizeof *desktop);
	desktop->output = NULL;
	desktop->shell = NULL;
	desktop->helper = NULL;

	desktop->gdk_display = gdk_display_get_default();
	desktop->display =
		gdk_wayland_display_get_wl_display(desktop->gdk_display);
	if (desktop->display == NULL) {
		fprintf(stderr, "failed to get display: %m\n");
		return -1;
	}

	desktop->registry = wl_display_get_registry(desktop->display);
	wl_registry_add_listener(desktop->registry,
			&registry_listener, desktop);

	/* Wait until we have been notified about the compositor,
	 * shell, and shell helper objects */
	while (!desktop->output || !desktop->shell || !desktop->helper)
		wl_display_roundtrip (desktop->display);

	desktop->grid_visible = FALSE;
	desktop->volume_visible = FALSE;

	css_setup(desktop);
	background_create(desktop);

	/* panel needs to be first so the clock and launcher grid can
	 * be added to its layer */
	panel_create(desktop);
	clock_create(desktop);
	launcher_grid_create (desktop);

	gtk_main();

	/* TODO cleanup */
	return EXIT_SUCCESS;
}
