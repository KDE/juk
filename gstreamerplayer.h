/***************************************************************************
                          gstreamerplayer.h  -  description
                             -------------------
    begin                : Sat Feb 9 2003
    copyright            : (C) 2003 by Tim Jansen
    email                : tim@tjansen.de
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

#include "../config.h"

#if HAVE_GSTREAMER

#include <qgstplay/play.h>

#include <qobject.h>
#include <qstring.h>

#include "player.h"

class GStreamerPlayer : public QObject, public Player
{
    Q_OBJECT

public:
    GStreamerPlayer();
    virtual ~GStreamerPlayer();

    virtual void play(const QString &fileName, float volume = 1.0);
    virtual void play(float volume = 1.0);

    virtual void setVolume(float volume = 1.0);
    virtual float getVolume() const;

    virtual bool playing() const;
    virtual bool paused() const;

    virtual long totalTime() const;
    virtual long currentTime() const;
    virtual int position() const; // in this case not really the percent

    virtual void seek(long seekTime);
    virtual void seekPosition(int position);

public slots:
    void pause();
    void stop();

private slots:
    void setPosition(long long d);
    void setDuration(long long d);

private:
    void setupPlayer();

    QGstPlay::Play *m_player;
    unsigned long long duration;

    long long positionNs; // in ns
    long long durationNs; // in ns

    QString m_currentFile;
    float m_currentVolume;
};

#endif

#endif
