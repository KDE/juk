/***************************************************************************
    copyright            : (C) 2004 Scott Wheeler
    email                : wheeler@kde.org
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef GSTREAMERPLAYER_H
#define GSTREAMERPLAYER_H

#include "config.h"

#if HAVE_GSTREAMER

#include <gst/gst.h>

#include <qstring.h>

#include "player.h"

class GStreamerPlayer : public Player
{
    Q_OBJECT

public:
    GStreamerPlayer();
    virtual ~GStreamerPlayer();

    virtual void play(const FileHandle &file = FileHandle::null());

    virtual void setVolume(float volume = 1.0);
    virtual float volume() const;

    virtual bool playing() const;
    virtual bool paused() const;

    virtual int totalTime() const;
    virtual int currentTime() const;
    virtual int position() const; // in this case not really the percent

    virtual void seek(int seekTime);
    virtual void seekPosition(int position);

public slots:
    void pause();
    void stop();

private:
    long long time(GstQueryType type) const;

    GstElement *m_pipeline;
    GstElement *m_source;
    GstElement *m_decoder;
    GstElement *m_volume;
    GstElement *m_sink;
};

#endif
#endif
