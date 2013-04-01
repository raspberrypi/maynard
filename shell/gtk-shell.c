#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkwayland.h>

#include "desktop-shell-client-protocol.h"

gchar *filename = "background.jpg";

struct desktop {
	struct wl_display *display;
	struct wl_registry *registry;
	struct desktop_shell *shell;
	struct wl_output *output;
	struct wl_surface *surface;

	GdkDisplay *gdk_display;

	/* Background */
	GtkWidget *window;
	GdkPixbuf *background;
};

static void
desktop_shell_configure(void *data,
		struct desktop_shell *desktop_shell,
		uint32_t edges,
		struct wl_surface *surface,
		int32_t width, int32_t height)
{
	struct desktop *desktop = wl_surface_get_user_data(surface);

	gtk_widget_set_size_request (desktop->window, width, height);
}

static void
desktop_shell_prepare_lock_surface(void *data,
		struct desktop_shell *desktop_shell)
{
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

/* Expose callback for the drawing area */
static gboolean
draw_cb (GtkWidget *widget, cairo_t *cr, gpointer data)
{
	struct desktop *desktop = data;

	gdk_cairo_set_source_pixbuf (cr, desktop->background, 0, 0);
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

	/* TODO: get the "right" directory */
	desktop->background = gdk_pixbuf_new_from_file (filename, NULL);
	if (!desktop->background) {
		g_message ("Could not load background.");
		exit (EXIT_FAILURE);
	}

	desktop->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	g_signal_connect (desktop->window, "destroy",
			  G_CALLBACK (destroy_cb), NULL);

	g_signal_connect (desktop->window, "draw",
			  G_CALLBACK (draw_cb), desktop);

	gtk_window_set_title(GTK_WINDOW(desktop->window), "gtk shell");
	gtk_window_set_decorated(GTK_WINDOW(desktop->window), FALSE);
	gtk_widget_realize(desktop->window);

	gdk_window = gtk_widget_get_window(desktop->window);
	gdk_wayland_window_set_use_custom_surface(gdk_window);

	desktop->surface = gdk_wayland_window_get_wl_surface(gdk_window);
	desktop_shell_set_background(desktop->shell, desktop->output,
		desktop->surface);

	gtk_widget_show_all(desktop->window);
}

static void
registry_handle_global(void *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version)
{
	struct desktop *d = data;

	if (!strcmp(interface, "desktop_shell")) {
		d->shell = wl_registry_bind(registry, name,
				&desktop_shell_interface, 1);
		desktop_shell_add_listener(d->shell, &listener, d);
	} else if (!strcmp(interface, "wl_output")) {

		/* TODO: create multiple outputs */
		d->output = wl_registry_bind(registry, name,
					     &wl_output_interface, 1);
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

	gtk_init(&argc, &argv);

	desktop = malloc(sizeof *desktop);
	desktop->output = NULL;
	desktop->shell = NULL;

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

	/* Wait until we have been notified about the compositor and shell
	 * objects */
	while (!desktop->output || !desktop->shell)
		wl_display_roundtrip (desktop->display);

	background_create(desktop);
	gtk_main();

        return EXIT_SUCCESS;
}
