/***************************************************************************
    begin                : Sat Feb 14 2004
    copyright            : (C) 2004 by Scott Wheeler
    email                : wheeler@kde.org

    copyright            : (C) 2007 by Matthias Kretz
    email                : kretz@kde.org

    copyright            : (C) 2008 by Michael Pyne
    email                : michael.pyne@kdemail.net
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
    int totalTime() const;
    int currentTime() const;
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
