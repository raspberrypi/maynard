/*
 * Copyright (C) 2013 Collabora Ltd.
 *
 * Author: Emilio Pozuelo Monfort <emilio.pozuelo@collabora.co.uk>
 */

#ifndef __WESTON_GTK_SOUND_APPLET_H__
#define __WESTON_GTK_SOUND_APPLET_H__

#include <gtk/gtk.h>

#define WESTON_GTK_TYPE_SOUND_APPLET                 (weston_gtk_sound_applet_get_type ())
#define WESTON_GTK_SOUND_APPLET(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), WESTON_GTK_TYPE_SOUND_APPLET, WestonGtkSoundApplet))
#define WESTON_GTK_SOUND_APPLET_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), WESTON_GTK_TYPE_SOUND_APPLET, WestonGtkSoundAppletClass))
#define WESTON_GTK_IS_SOUND_APPLET(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WESTON_GTK_TYPE_SOUND_APPLET))
#define WESTON_GTK_IS_SOUND_APPLET_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), WESTON_GTK_TYPE_SOUND_APPLET))
#define WESTON_GTK_SOUND_APPLET_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), WESTON_GTK_TYPE_SOUND_APPLET, WestonGtkSoundAppletClass))

typedef struct WestonGtkSoundApplet WestonGtkSoundApplet;
typedef struct WestonGtkSoundAppletClass WestonGtkSoundAppletClass;
typedef struct WestonGtkSoundAppletPrivate WestonGtkSoundAppletPrivate;

struct WestonGtkSoundApplet
{
  GtkBox parent;

  WestonGtkSoundAppletPrivate *priv;
};

struct WestonGtkSoundAppletClass
{
  GtkBoxClass parent_class;
};

GType      weston_gtk_sound_applet_get_type         (void) G_GNUC_CONST;
GtkWidget *weston_gtk_sound_applet_new              (void);

#endif /* __WESTON_GTK_SOUND_APPLET_H__ */
