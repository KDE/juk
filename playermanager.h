/***************************************************************************
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
#include "jukIface.h"

class QTimer;
class KSelectAction;
class SliderAction;
class StatusLabel;
class PlaylistInterface;

/**
 * This class serves as a proxy to the Player interface and handles managing
 * the actions from the top-level mainwindow.
 */

class PlayerManager : public Player, public PlayerIface
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
    int status() const;
    int totalTime() const;
    int currentTime() const;
    int position() const;

    QStringList trackProperties();
    QString trackProperty(const QString &property) const;
    QPixmap trackCover(const QString &size) const;

    FileHandle playingFile() const;
    QString playingString() const;

    void setPlaylistInterface(PlaylistInterface *interface);
    void setStatusLabel(StatusLabel *label);

    QString randomPlayMode() const;

    static KSelectAction *playerSelectAction(QObject *parent);

public slots:

    void play(const FileHandle &file);
    void play(const QString &file);
    void play();
    void pause();
    void stop();
    void setVolume(float volume = 1.0);
    void seek(int seekTime);
    void seekPosition(int position);
    void seekForward();
    void seekBack();
    void playPause();
    void forward();
    void back();
    void volumeUp();
    void volumeDown();
    void mute();

    void setRandomPlayMode(const QString &randomMode);

signals:
    void signalPlay();
    void signalPause();
    void signalStop();

private:
    Player *player() const;
    void setup();
    void setOutput(const QString &);
    
private slots:
    void slotPollPlay();
    void slotUpdateTime(int position);
    void slotSetOutput(const QString &);
    void slotSetVolume(int volume);

private:
    FileHandle m_file;
    SliderAction *m_sliderAction;
    PlaylistInterface *m_playlistInterface;
    StatusLabel *m_statusLabel;
    Player *m_player;
    QTimer *m_timer;
    bool m_noSeek;
    bool m_muted;
    bool m_setup;

    static const int m_pollInterval = 800;
};

#endif
