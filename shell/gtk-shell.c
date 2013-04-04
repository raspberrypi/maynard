#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkwayland.h>

#include "desktop-shell-client-protocol.h"

extern char **environ; /* defined by libc */

gchar *filename = "background.jpg";
gchar *terminal_path = "/home/tiago/git/weston/clients/weston-terminal";

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

	GdkDisplay *gdk_display;

	struct element *background;
	struct element *panel;
};

static void
desktop_shell_configure(void *data,
		struct desktop_shell *desktop_shell,
		uint32_t edges,
		struct wl_surface *surface,
		int32_t width, int32_t height)
{
	struct desktop *desktop = data;

	gtk_widget_set_size_request (desktop->background->window,
				     width, height);

	gtk_widget_set_size_request (desktop->panel->window, width, 32);
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
launch_terminal (GtkWidget *widget, gpointer   data)
{
	char *argv[] = {NULL, NULL};
	pid_t pid;

	pid = fork();
	if (pid < 0) {
		fprintf(stderr, "fork failed: %m\n");
		return;
	}

	if (pid)
		return;

	argv[0] = terminal_path;
	if (execve(terminal_path, argv, environ) < 0) {
		fprintf(stderr, "execl '%s' failed: %m\n", terminal_path);
		exit(1);
	}
}

static void
panel_create(struct desktop *desktop)
{
	GdkWindow *gdk_window;
	struct element *panel;
	GtkWidget *box1, *button;

	panel = malloc(sizeof *panel);
	memset(panel, 0, sizeof *panel);

	panel->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	gtk_window_set_title(GTK_WINDOW(panel->window), "gtk shell");
	gtk_window_set_decorated(GTK_WINDOW(panel->window), FALSE);
	gtk_widget_realize(panel->window);

	box1 = gtk_box_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (panel->window), box1);

	button = gtk_button_new_with_label ("launch terminal");
	g_signal_connect (button, "clicked",
			  G_CALLBACK (launch_terminal), NULL);
	gtk_box_pack_start (GTK_BOX(box1), button, TRUE, TRUE, 0);
	gtk_widget_show (button);

	gtk_widget_show (box1);

	gdk_window = gtk_widget_get_window(panel->window);
	gdk_wayland_window_set_use_custom_surface(gdk_window);

	panel->surface = gdk_wayland_window_get_wl_surface(gdk_window);
	desktop_shell_set_user_data(desktop->shell, desktop);
	desktop_shell_set_panel(desktop->shell, desktop->output,
				panel->surface);

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

	panel_create(desktop);
	background_create(desktop);
	gtk_main();

	/* TODO cleanup */
	return EXIT_SUCCESS;
}
