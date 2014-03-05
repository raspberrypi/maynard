/*
 * Copyright (C) 2013 Collabora Ltd.
 *
 * Author: Emilio Pozuelo Monfort <emilio.pozuelo@collabora.co.uk>
 */

#ifndef __MAYNARD_SOUND_APPLET_H__
#define __MAYNARD_SOUND_APPLET_H__

#include <gtk/gtk.h>

#define MAYNARD_TYPE_SOUND_APPLET                 (maynard_sound_applet_get_type ())
#define MAYNARD_SOUND_APPLET(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAYNARD_TYPE_SOUND_APPLET, MaynardSoundApplet))
#define MAYNARD_SOUND_APPLET_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), MAYNARD_TYPE_SOUND_APPLET, MaynardSoundAppletClass))
#define MAYNARD_IS_SOUND_APPLET(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MAYNARD_TYPE_SOUND_APPLET))
#define MAYNARD_IS_SOUND_APPLET_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), MAYNARD_TYPE_SOUND_APPLET))
#define MAYNARD_SOUND_APPLET_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), MAYNARD_TYPE_SOUND_APPLET, MaynardSoundAppletClass))

typedef struct MaynardSoundApplet MaynardSoundApplet;
typedef struct MaynardSoundAppletClass MaynardSoundAppletClass;
typedef struct MaynardSoundAppletPrivate MaynardSoundAppletPrivate;

struct MaynardSoundApplet
{
  GtkBox parent;

  MaynardSoundAppletPrivate *priv;
};

struct MaynardSoundAppletClass
{
  GtkBoxClass parent_class;
};

GType      maynard_sound_applet_get_type         (void) G_GNUC_CONST;
GtkWidget *maynard_sound_applet_new              (void);

#endif /* __MAYNARD_SOUND_APPLET_H__ */
