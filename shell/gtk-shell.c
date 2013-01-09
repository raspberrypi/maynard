#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkwayland.h>

#include "desktop-shell-client-protocol.h"

gchar *filename = "notfound.png";

struct desktop {
	struct wl_display *display;
	struct wl_registry *registry;
	struct desktop_shell *shell;
	struct wl_output *output;
	struct wl_surface *surface;

	GdkDisplay *gdk_display;
	GdkWindow *gdk_window;
	GtkWidget *widget;
};

static void
desktop_shell_configure(void *data,
		struct desktop_shell *desktop_shell,
		uint32_t edges,
		struct wl_surface *surface,
		int32_t width, int32_t height)
{
	struct desktop *desktop = wl_surface_get_user_data(surface);

	gtk_window_resize(GTK_WINDOW(desktop->widget), width, height);
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

static void
background_create(struct desktop *desktop)
{
	GtkWidget *image;

	desktop->widget = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	/* XXX: this is actually not working */
	image = gtk_image_new_from_file(filename);

	gtk_container_add (GTK_CONTAINER (desktop->widget), image);

	gtk_window_set_title(GTK_WINDOW(desktop->widget), "gtk shell");
	gtk_window_set_decorated(GTK_WINDOW(desktop->widget), FALSE);
	gtk_widget_realize(desktop->widget);
	desktop->gdk_window = gtk_widget_get_window(desktop->widget);

	gdk_wayland_window_set_use_custom_surface(desktop->gdk_window);

	desktop->surface =
		gdk_wayland_window_get_wl_surface(desktop->gdk_window);

	wl_surface_set_user_data(desktop->surface, desktop);
	desktop_shell_set_background(desktop->shell, desktop->output,
		desktop->surface);

	gtk_widget_show_all(desktop->widget);
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
