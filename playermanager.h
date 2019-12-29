/**
 * Copyright (C) 2004 Scott Wheeler <wheeler@kde.org>
 * Copyright (C) 2007 Matthias Kretz <kretz@kde.org>
 * Copyright (C) 2008 Michael Pyne <mpyne@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef JUK_PLAYERMANAGER_H
#define JUK_PLAYERMANAGER_H

#include <QObject>

#include "filehandle.h"

#include <Phonon/Global>
#include <Phonon/Path>

class PlaylistInterface;
class QPixmap;

namespace Phonon
{
    class AudioOutput;
    class MediaObject;
    class MediaSource;
}

/**
 * This class serves as a proxy to the Player interface and handles managing
 * the actions from the top-level mainwindow.
 */

class PlayerManager : public QObject
{
    Q_OBJECT

public:
    PlayerManager();

    bool playing() const;
    bool paused() const;
    bool muted() const;
    float volume() const;
    int status() const;

    // These two have been part of the prior public DBus interface so they have
    // been retained. You should use the MSecs versions below. These return in units
    // of seconds instead.
    int totalTime() const;
    int currentTime() const;

    int totalTimeMSecs() const;
    int currentTimeMSecs() const;

    bool seekable() const;
    //int position() const;

    QStringList trackProperties();
    QString trackProperty(const QString &property) const;
    QPixmap trackCover(const QString &size) const;

    FileHandle playingFile() const;
    QString playingString() const;

    void setPlaylistInterface(PlaylistInterface *interface);

    QString randomPlayMode() const;

public slots:
    void play(const FileHandle &file);
    void play(const QString &file);
    void play(); // start or restart playback
    void pause();
    void stop();
    void seek(int seekTime);
    //void seekPosition(int position);
    void seekForward();
    void seekBack();
    void playPause();
    void forward();
    void back();
    void setVolume(qreal);
    void volumeUp();
    void volumeDown();
    void setMuted(bool m);
    bool mute();

    void trackHasChanged(const Phonon::MediaSource &newSource);
    void trackAboutToFinish();

    void setRandomPlayMode(const QString &randomMode);

signals:
    void tick(qint64 /* time_msec */);
    void totalTimeChanged(qint64 /* time_msec */);
    void mutedChanged(bool muted);
    void volumeChanged(float volume);
    void seeked(int newPos);
    void seekableChanged(bool muted);

    void signalPlay();
    void signalPause();
    void signalStop();
    void signalItemChanged(const FileHandle &file);

private:
    void setupAudio();

private slots:
    void slotFinished();
    void slotLength(qint64);
    void slotTick(qint64);
    void slotStateChanged(Phonon::State, Phonon::State);
    void slotSeekableChanged(bool);
    void slotMutedChanged(bool);

private:
    FileHandle m_file;
    PlaylistInterface *m_playlistInterface;
    bool m_muted;
    bool m_setup;

    static const int m_pollInterval = 800;

    Phonon::AudioOutput *m_output;
    Phonon::MediaObject *m_media;
    Phonon::Path m_audioPath;
};

#endif

// vim: set et sw=4 tw=0 sta:
