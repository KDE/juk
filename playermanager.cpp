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

#include "playermanager.h"
#include <config.h>

#include <kdebug.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kactioncollection.h>
#include <kselectaction.h>
#include <ktoggleaction.h>
#include <kurl.h>

#include <Phonon/AudioOutput>
#include <Phonon/MediaObject>
#include <Phonon/VolumeFaderEffect>

#include <QSlider>
#include <QPixmap>
#include <QtDBus>
#include <QTimer>

#include <math.h>

#include "playlistinterface.h"
#include "playeradaptor.h"
#include "slideraction.h"
#include "statuslabel.h"
#include "actioncollection.h"
#include "collectionlist.h"
#include "coverinfo.h"
#include "tag.h"

using namespace ActionCollection;

enum PlayerManagerStatus { StatusStopped = -1, StatusPaused = 1, StatusPlaying = 2 };

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

PlayerManager::PlayerManager() :
    QObject(),
    m_sliderAction(0),
    m_playlistInterface(0),
    m_statusLabel(0),
    m_noSeek(false),
    m_muted(false),
    m_setup(false),
    m_output(0),
    m_media(0)
{
// This class is the first thing constructed during program startup, and
// therefore has no access to the widgets needed by the setup() method.
// Since the setup() method will be called indirectly by the player() method
// later, just disable it here. -- mpyne
//    setup();
    new PlayerAdaptor( this );
    QDBusConnection::sessionBus().registerObject("/Player", this);

}

PlayerManager::~PlayerManager()
{
}

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

PlayerManager *PlayerManager::instance() // static
{
    static PlayerManager manager;
    return &manager;
}

bool PlayerManager::playing() const
{
    if(!m_media)
        return false;

    return (m_media->state() == Phonon::PlayingState || m_media->state() == Phonon::BufferingState);
}

bool PlayerManager::paused() const
{
    if(!m_media)
        return false;

    return m_media->state() == Phonon::PausedState;
}

float PlayerManager::volume() const
{
    if(!m_output)
        return 1.0;

    return m_output->volume();
}

int PlayerManager::status() const
{
    if(!m_media)
        return StatusStopped;

    if(paused())
        return StatusPaused;

    if(playing())
        return StatusPlaying;

    return 0;
}

int PlayerManager::totalTime() const
{
    if(!m_media)
        return 0;

    return m_media->totalTime() / 1000;
}

int PlayerManager::currentTime() const
{
    if(!m_media)
        return 0;

    return m_media->currentTime() / 1000;
}

/*
int PlayerManager::position() const
{
    if(!m_media)
        return 0;

    long curr = m_media->currentTime();
    if(curr > 0)
        return static_cast<int>(static_cast<float>(curr * SliderAction::maxPosition) / m_media->totalTime() + 0.5f);
    return -1;
}
*/

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
    if(!playing())
        return QString();

    QString str = m_file.tag()->artist() + " - " + m_file.tag()->title();
    if(m_file.tag()->artist().isEmpty())
        str = m_file.tag()->title();

    return str;
}

void PlayerManager::setPlaylistInterface(PlaylistInterface *interface)
{
    m_playlistInterface = interface;
}

void PlayerManager::setStatusLabel(StatusLabel *label)
{
    m_statusLabel = label;
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void PlayerManager::play(const FileHandle &file)
{
    if(!m_media)
        setup();

    if(!m_media || !m_playlistInterface)
        return;

    if(file.isNull()) {
        if(paused())
            m_media->play();
        else if(playing()) {
            m_media->seek(0);
        }
        else {
            m_playlistInterface->playNext();
            m_file = m_playlistInterface->currentFile();

            if(!m_file.isNull())
            {
                m_media->setCurrentSource(KUrl::fromPath(m_file.absFilePath()));
                m_media->play();
            }
        }
    }
    else {
        m_file = file;
        if(m_media->state() == Phonon::PlayingState)
        {
            // do a crossfade
            Phonon::VolumeFaderEffect *fader1 = new Phonon::VolumeFaderEffect(m_media);
            m_audioPath.insertEffect(fader1);
            Phonon::MediaObject *mo = m_media;
            Phonon::AudioOutput *out = m_output;

            mo->disconnect(this);

            m_media = new Phonon::MediaObject(this);
            m_output = new Phonon::AudioOutput(Phonon::MusicCategory, this);
            Phonon::VolumeFaderEffect *fader2 = new Phonon::VolumeFaderEffect(m_media);
            m_audioPath = Phonon::createPath(m_media, m_output);
            m_audioPath.insertEffect(fader2);
            m_output->setVolume(out->volume());
            m_output->setMuted(out->isMuted());
            m_media->setTickInterval(200);

            connect(m_media, SIGNAL(stateChanged(Phonon::State, Phonon::State)), SLOT(slotStateChanged(Phonon::State)));
            connect(m_media, SIGNAL(aboutToFinish()), SLOT(slotNeedNextUrl()));
            if(m_sliderAction->trackPositionSlider())
                m_sliderAction->trackPositionSlider()->setMediaObject(m_media);
            connect(m_media, SIGNAL(totalTimeChanged(qint64)), SLOT(slotLength(qint64)));
            connect(m_media, SIGNAL(tick(qint64)), SLOT(slotTick(qint64)));
            connect(m_media, SIGNAL(finished()), SLOT(slotFinished()));
            m_media->setCurrentSource(KUrl::fromPath(m_file.absFilePath()));

            fader2->setVolume(0.0f);
            fader1->fadeOut(2000);
            fader2->fadeIn(2000);
            m_media->play();
            QTimer::singleShot(3000, mo, SLOT(deleteLater()));
            QTimer::singleShot(3000, out, SLOT(deleteLater()));
            QTimer::singleShot(3000, fader2, SLOT(deleteLater()));

            if(m_sliderAction->trackPositionSlider()) {
                m_sliderAction->trackPositionSlider()->setMediaObject(m_media);
            }
            if(m_sliderAction->volumeSlider()) {
                m_sliderAction->volumeSlider()->setAudioOutput(m_output);
            }
        }
        else
        {
            m_media->setCurrentSource(KUrl::fromPath(m_file.absFilePath()));
            m_media->play();
        }
    }

    // Make sure that the player() actually starts before doing anything.

    if(!playing()) {
        kWarning(65432) << "Unable to play " << file.absFilePath();
        stop();
        return;
    }

    action("pause")->setEnabled(true);
    action("stop")->setEnabled(true);
    action("forward")->setEnabled(true);
    if(action<KToggleAction>("albumRandomPlay")->isChecked())
        action("forwardAlbum")->setEnabled(true);
    action("back")->setEnabled(true);

    emit signalPlay();
}

void PlayerManager::play(const QString &file)
{
    CollectionListItem *item = CollectionList::instance()->lookup(file);
    if(item) {
        Playlist::setPlaying(item);
        play(item->file());
    }
}

void PlayerManager::play()
{
    play(FileHandle::null());
}

void PlayerManager::pause()
{
    if(!m_media)
        return;

    if(paused()) {
        play();
        return;
    }

    action("pause")->setEnabled(false);

    m_media->pause();

    emit signalPause();
}

void PlayerManager::stop()
{
    if(!m_media || !m_playlistInterface)
        return;

    action("pause")->setEnabled(false);
    action("stop")->setEnabled(false);
    action("back")->setEnabled(false);
    action("forward")->setEnabled(false);
    action("forwardAlbum")->setEnabled(false);

    switch(m_media->state())
    {
    case Phonon::PlayingState:
    case Phonon::BufferingState:
        {
            Phonon::VolumeFaderEffect *fader = new Phonon::VolumeFaderEffect(m_media);
            m_audioPath.insertEffect(fader);
            fader->setFadeCurve(Phonon::VolumeFaderEffect::Fade9Decibel);
            fader->fadeOut(200);
            QTimer::singleShot(1000, m_media, SLOT(stop()));
            QTimer::singleShot(1200, fader, SLOT(deleteLater()));
            break;
        }
    default:
        m_media->stop();
    }
    m_playlistInterface->stop();

    m_file = FileHandle::null();

    emit signalStop();
}

void PlayerManager::setVolume(float volume)
{
    if(!m_output)
        setup();
    m_output->setVolume(volume);
}

void PlayerManager::seek(int seekTime)
{
    if(!m_media)
        return;

    m_media->seek(seekTime);
}

/*
void PlayerManager::seekPosition(int position)
{
    if(!m_media)
        return;

    if(!playing() || m_noSeek)
        return;

    slotUpdateTime(position);
    m_media->seek(static_cast<qint64>(static_cast<float>(m_media->totalTime() * position) / SliderAction::maxPosition + 0.5f));
}
*/

void PlayerManager::seekForward()
{
    const qint64 total = m_media->totalTime();
    const qint64 newtime = m_media->currentTime() + total / 100;
    m_media->seek(qMin(total, newtime));
}

void PlayerManager::seekBack()
{
    const qint64 total = m_media->totalTime();
    const qint64 newtime = m_media->currentTime() - total / 100;
    m_media->seek(qMax(qint64(0), newtime));
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
    if(!m_output)
        return;

    float volume = m_output->volume() + 0.04; // 4% up
    m_output->setVolume(volume);
}

void PlayerManager::volumeDown()
{
    if(!m_output)
        return;

    float volume = m_output->volume() - 0.04; // 4% up
    m_output->setVolume(volume);
}

void PlayerManager::mute()
{
    if(!m_output)
        return;

    m_output->setMuted(!m_output->isMuted());
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void PlayerManager::slotNeedNextUrl()
{
    if(m_file.isNull())
    {
        return;
    }
    m_playlistInterface->playNext();
    FileHandle nextFile = m_playlistInterface->currentFile();
    if(!nextFile.isNull())
    {
        //kDebug() << m_file.absFilePath();
        m_file = nextFile;
        m_media->enqueue(KUrl::fromPath(m_file.absFilePath()));

        emit signalPlay();
    }
}

void PlayerManager::slotFinished()
{
    action("pause")->setEnabled(false);
    action("stop")->setEnabled(false);
    action("back")->setEnabled(false);
    action("forward")->setEnabled(false);
    action("forwardAlbum")->setEnabled(false);

    m_playlistInterface->stop();

    m_file = FileHandle::null();

    emit signalStop();
}

void PlayerManager::slotLength(qint64 msec)
{
    m_statusLabel->setItemTotalTime(msec / 1000);
}

void PlayerManager::slotTick(qint64 msec)
{
    if(!m_media || !m_playlistInterface)
        return;

    m_noSeek = true;

    if(m_statusLabel) {
        m_statusLabel->setItemCurrentTime(msec / 1000);
    }

    m_noSeek = false;
}

void PlayerManager::slotStateChanged(Phonon::State newstate)
{
    if(newstate == Phonon::ErrorState)
    {
        switch(m_media->errorType())
        {
            case Phonon::NoError:
                kDebug() << "received a state change to ErrorState but errorType is NoError!?";
                break;
            case Phonon::NormalError:
                forward();
                KMessageBox::information(0, m_media->errorString());
                break;
            case Phonon::FatalError:
                // stop playback
                stop();
                KMessageBox::sorry(0, m_media->errorString());
                break;
        }
    }
}

/*
void PlayerManager::slotUpdateTime(int position)
{
    if(!m_statusLabel)
        return;

    float positionFraction = float(position) / SliderAction::maxPosition;
    float totalTime = float(m_media->totalTime()) / 1000.0f;
    int seekTime = int(positionFraction * totalTime + 0.5); // "+0.5" for rounding

    m_statusLabel->setItemCurrentTime(seekTime);
}
*/

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void PlayerManager::setup()
{
    // All of the actions required by this class should be listed here.

    if(!action("pause") ||
       !action("stop") ||
       !action("back") ||
       !action("forwardAlbum") ||
       !action("forward") ||
       !action("trackPositionAction"))
    {
        kWarning(65432) << "Could not find all of the required actions.";
        return;
    }

    if(m_setup)
        return;
    m_setup = true;

    m_output = new Phonon::AudioOutput(Phonon::MusicCategory, this);

    m_media = new Phonon::MediaObject(this);
    connect(m_media, SIGNAL(stateChanged(Phonon::State, Phonon::State)), SLOT(slotStateChanged(Phonon::State)));
    connect(m_media, SIGNAL(aboutToFinish()), SLOT(slotNeedNextUrl()));
    m_audioPath = Phonon::createPath(m_media, m_output);
    m_media->setTickInterval(200);

    // initialize action states

    action("pause")->setEnabled(false);
    action("stop")->setEnabled(false);
    action("back")->setEnabled(false);
    action("forward")->setEnabled(false);
    action("forwardAlbum")->setEnabled(false);

    // setup sliders

    m_sliderAction = action<SliderAction>("trackPositionAction");

    /*
    connect(m_sliderAction, SIGNAL(signalPositionChanged(int)),
            this, SLOT(seekPosition(int)));
    connect(m_sliderAction->trackPositionSlider(), SIGNAL(valueChanged(int)),
            this, SLOT(slotUpdateTime(int)));
            */

    if(m_sliderAction->trackPositionSlider())
    {
        m_sliderAction->trackPositionSlider()->setMediaObject(m_media);
    }
    if(m_sliderAction->volumeSlider())
    {
        m_sliderAction->volumeSlider()->setAudioOutput(m_output);
    }

    connect(m_media, SIGNAL(totalTimeChanged(qint64)), SLOT(slotLength(qint64)));
    connect(m_media, SIGNAL(tick(qint64)), SLOT(slotTick(qint64)));
    connect(m_media, SIGNAL(finished()), SLOT(slotFinished()));
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

#include "playermanager.moc"

// vim: set et sw=4 tw=0 sta:
