/***************************************************************************
                          playermanager.h
                             -------------------
    begin                : Sat Feb 14 2004
    copyright            : (C) 2004 by Scott Wheeler
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

#ifndef PLAYERMANAGER_H
#define PLAYERMANAGER_H

#include <qobject.h>

#include "player.h"

class QTimer;
class SliderAction;

/**
 * This is a simple interface that should be implemented by objects used by
 * the PlayerManager.
 */

class PlaylistInterface
{
public:
    virtual QString nextFile() const = 0;
    virtual QString previousFile() const = 0;
};

/**
 * This class serves as a proxy to the Player interface and handles managing
 * the actions from the top-level mainwindow.
 */

class PlayerManager : public QObject, public Player
{
    Q_OBJECT

protected:
    PlayerManager();
    virtual ~PlayerManager();

public:
    static PlayerManager *instance();

    virtual bool playing() const;
    virtual bool paused() const;
    virtual float volume() const;
    virtual long totalTime() const;
    virtual long currentTime() const;
    virtual int position() const;

    void setPlaylistInterface(const PlaylistInterface *interface);

public slots:

    virtual void play(const QString &fileName = QString::null);
    virtual void pause();
    virtual void stop();
    virtual void setVolume(float volume = 1.0);
    virtual void seek(long seekTime);
    virtual void seekPosition(int position);

private:
    void setup();

private slots:
    void slotPollPlay();

private:
    static PlayerManager *m_instance;

    KActionCollection *m_actionCollection;
    SliderAction *m_sliderAction;
    const PlaylistInterface *m_playlistInterface;
    Player *m_player;
    QTimer *m_timer;
    bool m_noSeek;

    static const int m_pollInterval = 800;
};

#endif
