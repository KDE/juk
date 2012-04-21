/***********************************************************************
 * Copyright 2012  Eike Hein <hein@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************/

#include "mpris2/mediaplayer2.h"
#include "juk.h"
#include "mediafiles.h"

#include <KAboutData>
#include <KApplication>
#include <KCmdLineArgs>
#include <KProtocolInfo>
#include <KWindowSystem>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QWidget>

#include <Phonon/BackendCapabilities>

MediaPlayer2::MediaPlayer2(QObject* parent) : QDBusAbstractAdaptor(parent)
{
    connect(Phonon::BackendCapabilities::notifier(), SIGNAL(capabilitiesChanged()),
            this, SLOT(backendCapabilitiesChanged()));
}

MediaPlayer2::~MediaPlayer2()
{
}

bool MediaPlayer2::CanRaise() const
{
    return true;
}

void MediaPlayer2::Raise() const
{
    JuK::JuKInstance()->show();
    KWindowSystem::forceActiveWindow(JuK::JuKInstance()->effectiveWinId());
}

bool MediaPlayer2::CanQuit() const
{
    return true;
}

void MediaPlayer2::Quit() const
{
    kapp->quit();
}

bool MediaPlayer2::CanSetFullscreen() const
{
    return false;
}

bool MediaPlayer2::Fullscreen() const
{
    return false;
}

bool MediaPlayer2::HasTrackList() const
{
    return false;
}

QString MediaPlayer2::Identity() const
{
    return KCmdLineArgs::aboutData()->programName();
}

QString MediaPlayer2::DesktopEntry() const
{
    return QLatin1String("kde4-juk");
}

QStringList MediaPlayer2::SupportedUriSchemes() const
{
    return QStringList(QLatin1String("file"));
}

QStringList MediaPlayer2::SupportedMimeTypes() const
{
    QStringList mimeTable = MediaFiles::mimeTypes();

    // Add whitelist hacks

    // technically, "audio/flac" is not a valid mimetype (not on IANA list), but some things expect it
    if( mimeTable.contains( "audio/x-flac" ) && !mimeTable.contains( "audio/flac" ) )
        mimeTable << "audio/flac";

    bool canPlayMp3 = mimeTable.contains( "audio/mpeg" ) || mimeTable.contains( "audio/x-mp3" );
    // We special case this, as otherwise the users would hate us
    // Again, "audio/mp3" is not a valid mimetype, but is widely used
    // (the proper one is "audio/mpeg", but that is also for .mp1 and .mp2 files)
    if( canPlayMp3 && !mimeTable.contains( "audio/mp3" ) )
        mimeTable << "audio/mp3";
    if( canPlayMp3 && !mimeTable.contains( "audio/x-mp3" ) )
        mimeTable << "audio/x-mp3";

    return mimeTable;
}

void MediaPlayer2::backendCapabilitiesChanged()
{
    QDBusMessage msg = QDBusMessage::createSignal("/org/mpris/MediaPlayer2",
        "org.freedesktop.DBus.Properties", "PropertiesChanged" );

    QVariantList args;
    args << "org.mpris.MediaPlayer2";
    args << QVariantMap();
    args << QStringList("SupportedMimeTypes");

    msg.setArguments(args);

    QDBusConnection::sessionBus().send(msg);
}

