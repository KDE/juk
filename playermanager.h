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

#ifndef PLAYERMANAGER_H
#define PLAYERMANAGER_H

#include <QObject>

#include "filehandle.h"

#include <Phonon/Global>
#include <Phonon/Path>

class KSelectAction;
class StatusLabel;
class PlaylistInterface;
class QPixmap;

namespace Phonon
{
    class AudioOutput;
    class MediaObject;
    class VolumeFaderEffect;
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
    virtual ~PlayerManager();

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

    KSelectAction* outputDeviceSelectAction();

    void setPlaylistInterface(PlaylistInterface *interface);
    void setStatusLabel(StatusLabel *label);

    QString randomPlayMode() const;

public slots:
    void play(const FileHandle &file);
    void play(const QString &file);
    void play();
    void pause();
    void stop();
    void setVolume(float volume = 1.0);
    void seek(int seekTime);
    //void seekPosition(int position);
    void seekForward();
    void seekBack();
    void playPause();
    void forward();
    void back();
    void volumeUp();
    void volumeDown();
    void setMuted(bool m);
    bool mute();

    void setRandomPlayMode(const QString &randomMode);
    void setCrossfadeEnabled(bool enableCrossfade);

signals:
    void tick(int time);
    void totalTimeChanged(int time);
    void mutedChanged(bool muted);
    void volumeChanged(float volume);
    void seeked(int newPos);
    void seekableChanged(bool muted);

    void signalPlay();
    void signalPause();
    void signalStop();
    void signalItemChanged(const FileHandle &file);

private:
    void setup();
    void crossfadeToFile(const FileHandle &newFile);
    void stopCrossfade();

private slots:
    void slotNeedNextUrl();
    void slotFinished();
    void slotLength(qint64);
    void slotTick(qint64);
    void slotStateChanged(Phonon::State, Phonon::State);
    /// Updates the GUI to reflect stopped playback if we're stopped at this point.
    void slotUpdateGuiIfStopped();
    void slotSeekableChanged(bool);
    void slotMutedChanged(bool);
    void slotVolumeChanged(qreal);

private:
    FileHandle m_file;
    PlaylistInterface *m_playlistInterface;
    StatusLabel *m_statusLabel;
    bool m_muted;
    bool m_setup;
    bool m_crossfadeTracks;

    static const int m_pollInterval = 800;

    int m_curOutputPath; ///< Either 0 or 1 depending on which output path is in use.
    Phonon::AudioOutput *m_output[2];
    Phonon::Path m_audioPath[2];
    Phonon::MediaObject *m_media[2];
    Phonon::VolumeFaderEffect *m_fader[2];
};

#endif

// vim: set et sw=4 tw=0 sta:
