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


#include "player.h"

class QTimer;
class SliderAction;
class StatusLabel;
class PlaylistInterface;

/**
 * This class serves as a proxy to the Player interface and handles managing
 * the actions from the top-level mainwindow.
 */

class PlayerManager : public QObject
{
    Q_OBJECT

protected:
    PlayerManager();
    virtual ~PlayerManager();

public:
    static PlayerManager *instance();

    bool playing() const;
    bool paused() const;
    float volume() const;
    long totalTime() const;
    long currentTime() const;
    int position() const;

    void setPlaylistInterface(PlaylistInterface *interface);
    void setStatusLabel(StatusLabel *label);

public slots:

    void play(const QString &fileName = QString::null);
    void pause();
    void stop();
    void setVolume(float volume = 1.0);
    void seek(long seekTime);
    void seekPosition(int position);
    void seekForward();
    void seekBack();
    void playPause();
    void forward();
    void back();
    void volumeUp();
    void volumeDown();
    void mute();

private:
    Player *player() const;
    void setup();

private slots:
    void slotPollPlay();
    void slotUpdateTime(int position);
    void slotSetOutput(int system);
    void slotSetVolume(int volume); // TODO: make private

private:
    static PlayerManager *m_instance;

    SliderAction *m_sliderAction;
    PlaylistInterface *m_playlistInterface;
    StatusLabel *m_statusLabel;
    Player *m_player;
    QTimer *m_timer;
    bool m_noSeek;
    bool m_muted;

    static const int m_pollInterval = 800;
};

#endif
