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

#include <kde/gstplay/play.h>

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
    void slotSetPosition(long long d) { m_positionNs = d; }
    void slotSetDuration(long long d) { m_durationNs = d; }
    void stopIfNotPlaying();

private:
    void setupPlayer();

    KDE::GSTPlay::Play *m_player;
    unsigned long long m_duration;

    long long m_positionNs; // in ns
    long long m_durationNs; // in ns

    QString m_currentFile;
    float m_currentVolume;
};

#endif

#endif
