/**
 * Copyright (C) 2004 Scott Wheeler <wheeler@kde.org>
 * Copyright (C) 2007 Matthias Kretz <kretz@kde.org>
 * Copyright (C) 2008, 2009, 2018 Michael Pyne <mpyne@kde.org>
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

#include "playermanager.h"

#include <kmessagebox.h>
#include <kactioncollection.h>
#include <kselectaction.h>
#include <ktoggleaction.h>
#include <KLocalizedString>

#include <QAudioOutput>
#include <QPixmap>
#include <QUrl>

#include <algorithm>

#include "playlistinterface.h"
#include "playeradaptor.h"
#include "slideraction.h"
#include "statuslabel.h"
#include "actioncollection.h"
#include "collectionlist.h"
#include "coverinfo.h"
#include "juktag.h"
#include "scrobbler.h"
#include "juk.h"
#include "juk_debug.h"

using namespace ActionCollection;

enum PlayerManagerStatus { StatusStopped = -1, StatusPaused = 1, StatusPlaying = 2 };

////////////////////////////////////////////////////////////////////////////////
// static functions
////////////////////////////////////////////////////////////////////////////////
static void updateWindowTitle(const FileHandle &file)
{
    JuK::JuKInstance()->setWindowTitle(i18nc(
        "%1 is the artist and %2 is the title of the currently playing track.", 
        "%1 - %2 :: JuK",
        file.tag()->artist(),
        file.tag()->title()));
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

PlayerManager::PlayerManager() :
    QObject(),
    m_playlistInterface(nullptr),
    m_output(new QAudioOutput(this)),
    m_media(new QMediaPlayer(this))
{
    m_media->setAudioOutput(m_output);
    setupAudio();
    new PlayerAdaptor(this);
    QDBusConnection::sessionBus().registerObject("/Player", this);
}

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

bool PlayerManager::playing() const
{
    return m_media->playbackState() == QMediaPlayer::PlayingState;
}

bool PlayerManager::paused() const
{
    return m_media->playbackState() == QMediaPlayer::PausedState;
}

bool PlayerManager::muted() const
{
    return m_output->isMuted();
}

float PlayerManager::volume() const
{
    return m_output->volume();
}

int PlayerManager::status() const
{
    if(paused())
        return StatusPaused;

    if(playing())
        return StatusPlaying;

    return StatusStopped;
}

int PlayerManager::totalTime() const
{
    return totalTimeMSecs() / 1000;
}

int PlayerManager::currentTime() const
{
    return currentTimeMSecs() / 1000;
}

int PlayerManager::totalTimeMSecs() const
{
    return m_media->duration();
}

int PlayerManager::currentTimeMSecs() const
{
    return m_media->position();
}

bool PlayerManager::seekable() const
{
    return m_media->isSeekable();
}

QStringList PlayerManager::trackProperties()
{
    return FileHandle::properties();
}

QString PlayerManager::trackProperty(const QString &property) const
{
    if(!playing() && !paused())
        return QString();

    return m_file.property(property);
}

QPixmap PlayerManager::trackCover(const QString &size) const
{
    if(!playing() && !paused())
        return QPixmap();

    if(size.toLower() == "small")
        return m_file.coverInfo()->pixmap(CoverInfo::Thumbnail);
    if(size.toLower() == "large")
        return m_file.coverInfo()->pixmap(CoverInfo::FullSize);

    return QPixmap();
}

FileHandle PlayerManager::playingFile() const
{
    return m_file;
}

QString PlayerManager::playingString() const
{
    if(!playing() || m_file.isNull())
        return QString();

    return m_file.tag()->playingString();
}

void PlayerManager::setPlaylistInterface(PlaylistInterface *interface)
{
    m_playlistInterface = interface;
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void PlayerManager::play(const FileHandle &file)
{
    if(!m_media || !m_playlistInterface || file.isNull())
        return;

    if(m_file != file) {
        emit signalItemChanged(file);
        m_media->setSource(QUrl::fromLocalFile(file.absFilePath()));
        m_media->play();
    }

    m_file = file;

    // Our state changed handler will perform the follow up actions necessary
    // once we actually start playing.
}

void PlayerManager::play(const QString &file)
{
    CollectionListItem *item = CollectionList::instance()->lookup(file);
    if(item) {
        item->playlist()->setPlaying(item); // Will reentrantly call play(FileHandle)
    }
}

void PlayerManager::play()
{
    if(paused())
        m_media->play();
    else if(playing()) {
        m_media->setPosition(0);
        emit seeked(0);
    }
    else {
        // Will reentrantly call play(FileHandle)
        m_playlistInterface->playNext();
    }
}

void PlayerManager::pause()
{
    if(paused())
        return;

    action("pause")->setEnabled(false);

    m_media->pause();
}

void PlayerManager::stop()
{
    if(!m_playlistInterface)
        return;

    action("pause")->setEnabled(false);
    action("stop")->setEnabled(false);
    action("back")->setEnabled(false);
    action("forward")->setEnabled(false);
    action("forwardAlbum")->setEnabled(false);

    if(!m_file.isNull()) {
        m_file = FileHandle();
        emit signalItemChanged(m_file);
    }

    m_media->stop();
}

void PlayerManager::seek(int seekTime)
{
    if(m_media->position() == seekTime)
        return;

    m_media->setPosition(seekTime);
    emit seeked(seekTime);
}

void PlayerManager::seekForward()
{
    const qint64 total = m_media->duration();
    const qint64 newtime = m_media->position() + total / 100;
    const qint64 seekTo = qMin(total, newtime);

    m_media->setPosition(seekTo);
    emit seeked(seekTo);
}

void PlayerManager::seekBack()
{
    const qint64 total = m_media->duration();
    const qint64 newtime = m_media->position() - total / 100;
    const qint64 seekTo = qMax(qint64(0), newtime);

    m_media->setPosition(seekTo);
    emit seeked(seekTo);
}

void PlayerManager::playPause()
{
    playing() ? action("pause")->trigger() : action("play")->trigger();
}

void PlayerManager::forward()
{
    m_playlistInterface->playNext();
    FileHandle file = m_playlistInterface->currentFile();

    if(!file.isNull())
        play(file);
    else
        stop();
}

void PlayerManager::back()
{
    m_playlistInterface->playPrevious();
    FileHandle file = m_playlistInterface->currentFile();

    if(!file.isNull())
        play(file);
    else
        stop();
}

void PlayerManager::volumeUp()
{
    const auto newVolume = std::min(m_output->volume() + 0.04, 1.0);
    m_output->setVolume(newVolume); // 4% up
}

void PlayerManager::volumeDown()
{
    const auto newVolume = std::max(m_output->volume() - 0.04, 0.0);
    m_output->setVolume(newVolume); // 4% down
}

void PlayerManager::setMuted(bool m)
{
    m_output->setMuted(m);
}

bool PlayerManager::mute()
{
    bool newState = !muted();
    setMuted(newState);
    return newState;
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void PlayerManager::slotFinished()
{
    // It is possible to end up in this function if a file simply fails to play or if the
    // user moves the slider all the way to the end, therefore see if we can keep playing
    // and if we can, do so.  Otherwise, stop.

    m_playlistInterface->playNext();
    play(m_playlistInterface->currentFile());
}

void PlayerManager::slotLength(qint64 msec)
{
    emit totalTimeChanged(msec);
}

void PlayerManager::slotTick(qint64 msec)
{
    emit tick(msec);
}

void PlayerManager::slotStateChanged(QMediaPlayer::PlaybackState newstate)
{
    if(m_media->error() != QMediaPlayer::NoError) {
        QString errorMessage =
            i18nc(
              "%1 will be the /path/to/file, %2 will be some string from QtMultimedia describing the error",
              "JuK is unable to play the audio file<nl/><filename>%1</filename><nl/>"
                "for the following reason:<nl/><message>%2</message>",
              m_file.absFilePath(),
              m_media->errorString()
            );

        qCWarning(JUK_LOG)
                << "QtMultimedia is in error state" << m_media->errorString()
                << "while playing" << m_file.absFilePath();

        KMessageBox::error(0, errorMessage);

        stop();
        return;
    }

    // "normal" path
    if(newstate == QMediaPlayer::StoppedState && m_file.isNull()) {
        JuK::JuKInstance()->setWindowTitle(i18n("JuK"));
        emit signalStop();
    }
    else if(newstate == QMediaPlayer::PausedState) {
        emit signalPause();
    }
    else { // PlayingState
        action("pause")->setEnabled(true);
        action("stop")->setEnabled(true);
        action("forward")->setEnabled(true);
        if(action<KToggleAction>("albumRandomPlay")->isChecked())
            action("forwardAlbum")->setEnabled(true);
        action("back")->setEnabled(true);

        updateWindowTitle(m_file);
        emit signalPlay();
    }
}

void PlayerManager::slotSeekableChanged(bool isSeekable)
{
    emit seekableChanged(isSeekable);
}

void PlayerManager::slotMutedChanged(bool muted)
{
    emit mutedChanged(muted);
}

void PlayerManager::setVolume(qreal volume)
{
    if(!qIsFinite(volume) || qFuzzyCompare((qreal)m_output->volume(), volume)) {
        return;
    }

    m_output->setVolume(volume);
    emit volumeChanged(volume);
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void PlayerManager::setupAudio()
{
    connect(m_output, &QAudioOutput::mutedChanged,
        this, &PlayerManager::slotMutedChanged);
    connect(m_output, &QAudioOutput::volumeChanged,
        this, &PlayerManager::setVolume);

    connect(m_media, &QMediaPlayer::playbackStateChanged,
        this, &PlayerManager::slotStateChanged);
    connect(m_media, &QMediaPlayer::sourceChanged,
        this, &PlayerManager::trackHasChanged);
    connect(m_media, &QMediaPlayer::durationChanged,
        this, &PlayerManager::slotLength);
    connect(m_media, &QMediaPlayer::positionChanged,
        this, &PlayerManager::slotTick);
    connect(m_media, &QMediaPlayer::mediaStatusChanged, this,
        [this](QMediaPlayer::MediaStatus newStatus) {
            if (newStatus == QMediaPlayer::EndOfMedia) {
                emit slotFinished();
            }
        });
    connect(m_media, &QMediaPlayer::seekableChanged, this,
        &PlayerManager::slotSeekableChanged);
}

QString PlayerManager::randomPlayMode() const
{
    if(action<KToggleAction>("randomPlay")->isChecked())
        return "Random";
    if(action<KToggleAction>("albumRandomPlay")->isChecked())
        return "AlbumRandom";
    return "NoRandom";
}

void PlayerManager::setRandomPlayMode(const QString &randomMode)
{
    if(randomMode.toLower() == "random")
        action<KToggleAction>("randomPlay")->setChecked(true);
    if(randomMode.toLower() == "albumrandom")
        action<KToggleAction>("albumRandomPlay")->setChecked(true);
    if(randomMode.toLower() == "norandom")
        action<KToggleAction>("disableRandomPlay")->setChecked(true);
}

void PlayerManager::trackHasChanged(const QUrl &newSource)
{
    const auto item = CollectionList::instance()->lookup(newSource.path());
    if(item) {
        const auto newFile = item->file();
        if(m_file != newFile)
            emit signalItemChanged(newFile);
        m_file = newFile;
        updateWindowTitle(m_file);
        emit signalPlay();
        emit seeked(0);
    }
}

// vim: set et sw=4 tw=0 sta:
