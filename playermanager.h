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

class SliderAction;

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

public slots:

    // Implementation of the Player interface

    virtual void play(const QString &fileName = QString::null);
    virtual void pause();
    virtual void stop();

    virtual void setVolume(float volume = 1.0);
    virtual float volume() const;

    virtual bool playing() const;
    virtual bool paused() const;

    virtual long totalTime() const;
    virtual long currentTime() const;
    virtual int position() const;

    virtual void seek(long seekTime);
    virtual void seekPosition(int position);

private:
    void setup();
    static PlayerManager *m_instance;

    KActionCollection *m_actionCollection;
    SliderAction *m_sliderAction;
    Player *m_player;
};

#endif
