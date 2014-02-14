#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkwayland.h>

#include "desktop-shell-client-protocol.h"
#include "shell-helper-client-protocol.h"

#include "weston-gtk-shell-resources.h"

#include "app-icon.h"
#include "clock.h"
#include "favorites.h"
#include "launcher-grid.h"
#include "sound-applet.h"

extern char **environ; /* defined by libc */

gchar *filename = "/usr/share/themes/Adwaita/backgrounds/morning.jpg";

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

	guint initial_panel_timeout_id;
	guint hide_panel_idle_id;
};

/* TODO: guessed from the mockups, it'd be nice to have this in stone
 * somewhere. */
#define PANEL_HEIGHT_RATIO 0.73

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

	g_signal_connect (desktop->panel->window, "enter-notify-event",
			  G_CALLBACK (panel_window_enter_cb), desktop);
	g_signal_connect (desktop->panel->window, "leave-notify-event",
			  G_CALLBACK (panel_window_leave_cb), desktop);

	return FALSE;
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

	gtk_widget_set_size_request (desktop->background->window,
				     width, height);

	/* Not sure where this '28' comes from, but it seems to work...
	 * The issue here is that the bottom of the grid (after scrolling
	 * all the way down) gets cut. That is because the window is a
	 * bit bigger than it should, but logic tells me we should only
	 * substract 16px from the height (those we're giving to the panel),
	 * but maybe I'm missing something...
	 */
	gtk_widget_set_size_request (desktop->launcher_grid->window,
				     width, height - 28);

	/* TODO: make this height a little nicer */
	window_height = height * PANEL_HEIGHT_RATIO;
	gtk_window_resize (GTK_WINDOW (desktop->panel->window), 56, window_height);

	shell_helper_move_surface(desktop->helper,
				  desktop->panel->surface,
				  0, (height - window_height) / 2);

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
launcher_grid_create (struct desktop *desktop)
{
	struct element *launcher_grid;
	GtkWidget *grid;

	launcher_grid = malloc (sizeof *launcher_grid);
	memset (launcher_grid, 0, sizeof *launcher_grid);

	launcher_grid->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

	gtk_window_set_title(GTK_WINDOW(launcher_grid->window), "gtk shell");
	gtk_window_set_decorated(GTK_WINDOW(launcher_grid->window), FALSE);
	gtk_widget_realize(launcher_grid->window);

	grid = launcher_grid_new ();

	gtk_container_add (GTK_CONTAINER (launcher_grid->window), grid);

	gtk_widget_show_all (grid);

	desktop->launcher_grid = launcher_grid;
}

static void
launcher_grid_toggle (GtkWidget *widget, struct desktop *desktop)
{
	gboolean grid_visible;

	grid_visible = gtk_widget_is_visible (desktop->launcher_grid->window);
	gtk_widget_set_visible (desktop->launcher_grid->window, !grid_visible);
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
				   -31, 0);

	return FALSE;
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

	desktop->hide_panel_idle_id = g_idle_add (leave_panel_idle_cb, desktop);

	return FALSE;
}

static gboolean
panel_hide_timeout_cb (gpointer data)
{
	struct desktop *desktop = data;

	panel_window_leave_cb (NULL, NULL, desktop);
}

static void
panel_create(struct desktop *desktop)
{
	GdkWindow *gdk_window;
	struct element *panel;
	GtkWidget *box1, *button;
	GtkWidget *image; /* TODO */

	panel = malloc(sizeof *panel);
	memset(panel, 0, sizeof *panel);

	panel->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	gtk_window_set_title(GTK_WINDOW(panel->window), "gtk shell");
	gtk_window_set_decorated(GTK_WINDOW(panel->window), FALSE);
	gtk_widget_realize(panel->window);

	desktop->initial_panel_timeout_id = g_timeout_add_seconds(2,
								  panel_hide_timeout_cb,
								  desktop);

	gtk_style_context_add_class (gtk_widget_get_style_context (panel->window),
				     "wgs-panel");

	box1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add (GTK_CONTAINER (panel->window), box1);

	/* TODO: clock */

	/* TODO: wifi */
	image = gtk_image_new_from_icon_name ("network-wireless-signal-excellent-symbolic",
					      GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_style_context_add_class (gtk_widget_get_style_context (image),
				     "wgs-wifi");
	gtk_box_pack_start (GTK_BOX (box1), image, FALSE, FALSE, 0);

	/* TODO: sound */
	image = gtk_image_new_from_icon_name ("audio-volume-high-symbolic",
					      GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_style_context_add_class (gtk_widget_get_style_context (image),
				     "wgs-audio");
	gtk_box_pack_start (GTK_BOX (box1), image, FALSE, FALSE, 0);

	/* favourites */
	gtk_box_pack_start (GTK_BOX (box1), weston_gtk_favorites_new (), FALSE, FALSE, 0);

	/* menu */
	button = weston_gtk_app_icon_new ("view-grid-symbolic");
	g_signal_connect (button, "clicked",
			  G_CALLBACK (launcher_grid_toggle), desktop);
	gtk_box_pack_end (GTK_BOX (box1), button, FALSE, FALSE, 0);

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

	background = malloc(sizeof *background);
	memset(background, 0, sizeof *background);

	/* TODO: get the "right" directory */
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

	gtk_window_set_title(GTK_WINDOW(background->window), "gtk shell");
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

	file = g_file_new_for_uri ("resource:///org/raspberry-pi/weston-gtk-shell/style.css");

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

	g_resources_register (weston_gtk_shell_get_resource ());

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

	css_setup(desktop);
	panel_create(desktop);
	launcher_grid_create (desktop);
	background_create(desktop);

	gtk_main();

	/* TODO cleanup */
	return EXIT_SUCCESS;
}
