/***************************************************************************
    begin                : Sat Feb 14 2004
    copyright            : (C) 2004 by Scott Wheeler
    email                : wheeler@kde.org

    copyright            : (C) 2007 by Matthias Kretz
    email                : kretz@kde.org

    copyright            : (C) 2008, 2009 by Michael Pyne
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

#include "playermanager.h"

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

#include <QPixmap>
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
    m_muted(false),
    m_setup(false),
    m_crossfadeTracks(true),
    m_curOutputPath(0)
{
// This class is the first thing constructed during program startup, and
// therefore has no access to the widgets needed by the setup() method.
// Since the setup() method will be called indirectly by the player() method
// later, just disable it here. -- mpyne
//    setup();
    new PlayerAdaptor( this );
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
    if(!m_setup)
        return false;

    Phonon::State state = m_media[m_curOutputPath]->state();
    return (state == Phonon::PlayingState || state == Phonon::BufferingState);
}

bool PlayerManager::paused() const
{
    if(!m_setup)
        return false;

    return m_media[m_curOutputPath]->state() == Phonon::PausedState;
}

float PlayerManager::volume() const
{
    if(!m_setup)
        return 1.0;

    return m_output[m_curOutputPath]->volume();
}

int PlayerManager::status() const
{
    if(!m_setup)
        return StatusStopped;

    if(paused())
        return StatusPaused;

    if(playing())
        return StatusPlaying;

    return 0;
}

int PlayerManager::totalTime() const
{
    if(!m_setup)
        return 0;

    return m_media[m_curOutputPath]->totalTime() / 1000;
}

int PlayerManager::currentTime() const
{
    if(!m_setup)
        return 0;

    return m_media[m_curOutputPath]->currentTime() / 1000;
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
    if(!m_setup)
        setup();

    if(!m_media[0] || !m_media[1] || !m_playlistInterface)
        return;

    stopCrossfade();

    // The "currently playing" media object.
    Phonon::MediaObject *mediaObject = m_media[m_curOutputPath];

    if(file.isNull()) {
        if(paused())
            mediaObject->play();
        else if(playing()) {
            mediaObject->seek(0);
        }
        else {
            m_playlistInterface->playNext();
            m_file = m_playlistInterface->currentFile();

            if(!m_file.isNull())
            {
                mediaObject->setCurrentSource(KUrl::fromPath(m_file.absFilePath()));
                mediaObject->play();
            }
        }
    }
    else {
        m_file = file;

        mediaObject->setCurrentSource(KUrl::fromPath(m_file.absFilePath()));
        mediaObject->play();
    }

    // Our state changed handler will perform the follow up actions necessary
    // once we actually start playing.
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
    if(!m_setup)
        return;

    if(paused()) {
        play();
        return;
    }

    action("pause")->setEnabled(false);

    m_media[m_curOutputPath]->pause();

    emit signalPause();
}

void PlayerManager::stop()
{
    if(!m_setup || !m_playlistInterface)
        return;

    action("pause")->setEnabled(false);
    action("stop")->setEnabled(false);
    action("back")->setEnabled(false);
    action("forward")->setEnabled(false);
    action("forwardAlbum")->setEnabled(false);

    // Fading out playback is for chumps.
    stopCrossfade();
    m_media[0]->stop();
    m_media[1]->stop();
}

void PlayerManager::setVolume(float volume)
{
    if(!m_setup)
        setup();

    m_output[0]->setVolume(volume);
    m_output[1]->setVolume(volume);
}

void PlayerManager::seek(int seekTime)
{
    if(!m_setup || m_media[m_curOutputPath]->currentTime() == seekTime)
        return;

    kDebug() << "Stopping crossfade to seek from" << m_media[m_curOutputPath]->currentTime()
             << "to" << seekTime;
    stopCrossfade();
    m_media[m_curOutputPath]->seek(seekTime);
}

void PlayerManager::seekForward()
{
    Phonon::MediaObject *mediaObject = m_media[m_curOutputPath];
    const qint64 total = mediaObject->totalTime();
    const qint64 newtime = mediaObject->currentTime() + total / 100;

    stopCrossfade();
    mediaObject->seek(qMin(total, newtime));
}

void PlayerManager::seekBack()
{
    Phonon::MediaObject *mediaObject = m_media[m_curOutputPath];
    const qint64 total = mediaObject->totalTime();
    const qint64 newtime = mediaObject->currentTime() - total / 100;

    stopCrossfade();
    mediaObject->seek(qMax(qint64(0), newtime));
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
    if(!m_setup)
        return;

    setVolume(volume() + 0.04); // 4% up
}

void PlayerManager::volumeDown()
{
    if(!m_output)
        return;

    setVolume(volume() - 0.04); // 4% down
}

void PlayerManager::mute()
{
    if(!m_setup)
        return;

    m_output[m_curOutputPath]->setMuted(!m_output[m_curOutputPath]->isMuted());
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void PlayerManager::slotNeedNextUrl()
{
    if(m_file.isNull() || !m_crossfadeTracks)
        return;

    m_playlistInterface->playNext();
    FileHandle nextFile = m_playlistInterface->currentFile();

    if(!nextFile.isNull()) {
        m_file = nextFile;
        crossfadeToFile(m_file);
    }
}

void PlayerManager::slotFinished()
{
    // It is possible to end up in this function if a file simply fails to play or if the
    // user moves the slider all the way to the end, therefore see if we can keep playing
    // and if we can, do so.  Otherwise, stop.  Note that this slot should
    // only be called by the currently "main" output path (i.e. not from the
    // crossfading one).  However life isn't always so nice apparently, so do some
    // sanity-checking.

    Phonon::MediaObject *mediaObject = qobject_cast<Phonon::MediaObject *>(sender());
    if(mediaObject != m_media[m_curOutputPath])
        return;

    m_playlistInterface->playNext();
    m_file = m_playlistInterface->currentFile();

    if(m_file.isNull()) {
        stop();
    }
    else {
        m_media[m_curOutputPath]->setCurrentSource(m_file.absFilePath());
        m_media[m_curOutputPath]->play();
    }
}

void PlayerManager::slotLength(qint64 msec)
{
    m_statusLabel->setItemTotalTime(msec / 1000);
}

void PlayerManager::slotTick(qint64 msec)
{
    if(!m_setup || !m_playlistInterface)
        return;

    if(m_statusLabel)
        m_statusLabel->setItemCurrentTime(msec / 1000);
}

void PlayerManager::slotStateChanged(Phonon::State newstate, Phonon::State oldstate)
{
    // Use sender() since either media object may have sent the signal.
    Phonon::MediaObject *mediaObject = qobject_cast<Phonon::MediaObject *>(sender());
    if(!mediaObject)
        return;

    // Handle errors for either media object
    if(newstate == Phonon::ErrorState) {
        QString errorMessage =
            i18nc(
              "%1 will be the /path/to/file, %2 will be some string from Phonon describing the error",
              "JuK is unable to play the audio file<nl><filename>%1</filename><nl>"
                "for the following reason:<nl><message>%2</message>",
              m_file.absFilePath(),
              mediaObject->errorString()
            );

        switch(mediaObject->errorType()) {
            case Phonon::NoError:
                kDebug() << "received a state change to ErrorState but errorType is NoError!?";
                break;

            case Phonon::NormalError:
                forward();
                KMessageBox::information(0, errorMessage);
                break;

            case Phonon::FatalError:
                stop();
                KMessageBox::sorry(0, errorMessage);
                break;
        }
    }

    // Now bail out if we're not dealing with the currently playing media
    // object.

    if(mediaObject != m_media[m_curOutputPath])
        return;

    // Handle state changes for the playing media object.
    if(newstate == Phonon::StoppedState && oldstate != Phonon::LoadingState) {
        // If this occurs it should be due to a transitory shift (i.e. playing a different
        // song when one is playing now), since it didn't occur in the error handler.  Just
        // in case we really did abruptly stop, handle that case in a couple of seconds.
        QTimer::singleShot(2000, this, SLOT(slotUpdateGuiIfStopped()));
        emit signalStop();
    }
    else if(newstate == Phonon::PlayingState) {
        action("pause")->setEnabled(true);
        action("stop")->setEnabled(true);
        action("forward")->setEnabled(true);
        if(action<KToggleAction>("albumRandomPlay")->isChecked())
            action("forwardAlbum")->setEnabled(true);
        action("back")->setEnabled(true);

        emit signalPlay();
    }
}

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
        kWarning() << "Could not find all of the required actions.";
        return;
    }

    if(m_setup)
        return;
    m_setup = true;

    // We use two audio paths at all times to make cross fading easier (and to also easily
    // support not using cross fading with the same code).  The currently playing audio
    // path is controlled using m_curOutputPath.

    for(int i = 0; i < 2; ++i) {
        m_output[i] = new Phonon::AudioOutput(Phonon::MusicCategory, this);

        m_media[i] = new Phonon::MediaObject(this);
        m_audioPath[i] = Phonon::createPath(m_media[i], m_output[i]);
        m_media[i]->setTickInterval(200);
        m_media[i]->setPrefinishMark(2000);

        // Pre-cache a volume fader object
        m_fader[i] = new Phonon::VolumeFaderEffect(m_media[i]);
        m_audioPath[i].insertEffect(m_fader[i]);
        m_fader[i]->setVolume(1.0f);

        connect(m_media[i], SIGNAL(stateChanged(Phonon::State, Phonon::State)), SLOT(slotStateChanged(Phonon::State, Phonon::State)));
        connect(m_media[i], SIGNAL(prefinishMarkReached(qint32)), SLOT(slotNeedNextUrl()));
        connect(m_media[i], SIGNAL(totalTimeChanged(qint64)), SLOT(slotLength(qint64)));
        connect(m_media[i], SIGNAL(tick(qint64)), SLOT(slotTick(qint64)));
        connect(m_media[i], SIGNAL(finished()), SLOT(slotFinished()));
    }

    // initialize action states

    action("pause")->setEnabled(false);
    action("stop")->setEnabled(false);
    action("back")->setEnabled(false);
    action("forward")->setEnabled(false);
    action("forwardAlbum")->setEnabled(false);

    // setup sliders (a separate slot is used to switch as needed)

    m_sliderAction = action<SliderAction>("trackPositionAction");

    if(m_sliderAction->trackPositionSlider())
        m_sliderAction->trackPositionSlider()->setMediaObject(m_media[0]);

    if(m_sliderAction->volumeSlider())
        m_sliderAction->volumeSlider()->setAudioOutput(m_output[0]);

    QDBusConnection::sessionBus().registerObject("/Player", this);
}

void PlayerManager::slotUpdateSliders()
{
    m_sliderAction->trackPositionSlider()->setMediaObject(m_media[m_curOutputPath]);
    m_sliderAction->volumeSlider()->setAudioOutput(m_output[m_curOutputPath]);

    disconnect(m_media[1 - m_curOutputPath], 0, this, SLOT(slotUpdateSliders()));
    connect(m_media[1 - m_curOutputPath], SIGNAL(finished()), SLOT(slotFinished()));
}

void PlayerManager::slotUpdateGuiIfStopped()
{
    if(m_media[0]->state() == Phonon::StoppedState && m_media[1]->state() == Phonon::StoppedState)
        stop();
}

void PlayerManager::crossfadeToFile(const FileHandle &newFile)
{
    int nextOutputPath = 1 - m_curOutputPath;

    // Don't need this anymore
    disconnect(m_media[m_curOutputPath], SIGNAL(finished()), this, 0);
    connect(m_media[nextOutputPath], SIGNAL(finished()), SLOT(slotFinished()));

    // Wait a couple of seconds and switch slider objects.  (We would simply
    // handle this when finished() is emitted but phonon-gst is buggy).
    QTimer::singleShot(2000, this, SLOT(slotUpdateSliders()));

    m_fader[nextOutputPath]->setVolume(0.0f);

    m_media[nextOutputPath]->setCurrentSource(newFile.absFilePath());
    m_media[nextOutputPath]->play();

    m_fader[m_curOutputPath]->setVolume(1.0f);
    m_fader[m_curOutputPath]->fadeTo(0.0f, 2000);

    m_fader[nextOutputPath]->fadeTo(1.0f, 2000);

    m_curOutputPath = nextOutputPath;
}

void PlayerManager::stopCrossfade()
{
    // According to the Phonon docs, setVolume immediately takes effect,
    // which is "good enough for government work" ;)

    // 1 - curOutputPath is the other output path...
    m_fader[m_curOutputPath]->setVolume(1.0f);
    m_fader[1 - m_curOutputPath]->setVolume(0.0f);

    // We don't actually need to physically stop crossfading as the playback
    // code will call ->play() when necessary anyways.  If we hit stop()
    // here instead of pause() then we will trick our stateChanged handler
    // into thinking Phonon had a spurious stop and we'll switch tracks
    // unnecessarily.  (This isn't a problem after crossfade completes due to
    // the signals being disconnected).

    m_media[1 - m_curOutputPath]->pause();
}

QString PlayerManager::randomPlayMode() const
{
    if(action<KToggleAction>("randomPlay")->isChecked())
        return "Random";
    if(action<KToggleAction>("albumRandomPlay")->isChecked())
        return "AlbumRandom";
    return "NoRandom";
}

void PlayerManager::setCrossfadeEnabled(bool crossfadeEnabled)
{
    m_crossfadeTracks = crossfadeEnabled;
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
