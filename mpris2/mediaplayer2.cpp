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

#include <KAboutData>
#include <KApplication>
#include <KCmdLineArgs>
#include <KProtocolInfo>
#include <KService>
#include <KWindowSystem>

#include <QWidget>

MediaPlayer2::MediaPlayer2(QObject* parent) : QDBusAbstractAdaptor(parent)
{
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
    JuK::JuKInstance()->raise();
    KWindowSystem::activateWindow(JuK::JuKInstance()->effectiveWinId());
}

bool MediaPlayer2::CanQuit() const
{
    return true;
}

void MediaPlayer2::Quit() const
{
    kapp->closeAllWindows();
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
    return QLatin1String("juk");
}

QStringList MediaPlayer2::SupportedUriSchemes() const
{
    return QStringList(QLatin1String("file"));
}

QStringList MediaPlayer2::SupportedMimeTypes() const
{
    KService::Ptr app = KService::serviceByDesktopName(QLatin1String("juk"));

    if (app) {
        return app->mimeTypes();
    }

    return QStringList();
}
