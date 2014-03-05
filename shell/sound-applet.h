/*
 * Copyright (C) 2013 Collabora Ltd.
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
