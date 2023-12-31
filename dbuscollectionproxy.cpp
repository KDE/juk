/**
 * Copyright (C) 2009 Michael Pyne <mpyne@kde.org>
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

#include "dbuscollectionproxy.h"

#include <QStringList>
#include <QFile>
#include <QPixmap>
#include <QDBusConnection>
#include <QTemporaryFile>

#include "collectionadaptor.h"
#include "playlistcollection.h"
#include "collectionlist.h"
#include "coverinfo.h"
#include "filehandle.h"
#include "juk_debug.h"

DBusCollectionProxy::DBusCollectionProxy (QObject *parent, PlaylistCollection *collection) :
    QObject(parent), m_collection(collection)
{
    setObjectName( QLatin1String("DBusCollectionProxy" ));

    new CollectionAdaptor(this);
    QDBusConnection::sessionBus().registerObject("/Collection",this);
}

DBusCollectionProxy::~DBusCollectionProxy()
{
    // Clean's the way to be
    if(!m_lastCover.isEmpty())
        QFile::remove(m_lastCover);
}

void DBusCollectionProxy::openFile(const QString &file)
{
    m_collection->open(QStringList(file));
}

void DBusCollectionProxy::openFile(const QStringList &files)
{
    m_collection->open(files);
}

void DBusCollectionProxy::openFile(const QString &playlist, const QString &file)
{
    m_collection->open(playlist, QStringList(file));
}

void DBusCollectionProxy::openFile(const QString &playlist, const QStringList &files)
{
    m_collection->open(playlist, files);
}

QString DBusCollectionProxy::visiblePlaylist()
{
    return m_collection->playlist();
}

QString DBusCollectionProxy::playingPlaylist()
{
    return m_collection->playingPlaylist();
}

QStringList DBusCollectionProxy::playlists()
{
    return m_collection->playlists();
}

QStringList DBusCollectionProxy::playlistTracks(const QString &playlist)
{
    return m_collection->playlistTracks(playlist);
}

QString DBusCollectionProxy::trackProperty(const QString &file, const QString &property)
{
    return m_collection->trackProperty(file, property);
}

void DBusCollectionProxy::createPlaylist(const QString &name)
{
    m_collection->createPlaylist(name);
}

void DBusCollectionProxy::setPlaylist(const QString &name)
{
    m_collection->setPlaylist(name);
}

void DBusCollectionProxy::remove()
{
    m_collection->remove();
}

void DBusCollectionProxy::removeTrack(const QString &playlist, const QStringList &files)
{
    m_collection->removeTrack(playlist, files);
}

QString DBusCollectionProxy::trackCover(const QString &track)
{
    // Let's see if a cover is embedded.
    CollectionListItem *collectionItem = CollectionList::instance()->lookup(track);

    if(!collectionItem)
        return QString();

    CoverInfo *coverInfo = collectionItem->file().coverInfo();
    if(!coverInfo)
        return QString();

    QPixmap cover = coverInfo->pixmap(CoverInfo::FullSize);
    if(cover.isNull())
        return QString();

    // We have a cover, extract it and save it to a temporary file.
    QTemporaryFile tempFile;

    tempFile.setFileTemplate(QStringLiteral("juk_cover_XXXXXX.png"));
    tempFile.setAutoRemove(false);

    if(!tempFile.open()) {
        qCCritical(JUK_LOG) << "Unable to open temporary file for embedded cover art.";
        return QString();
    }

    // Save last file name cover, remove it if it's there so that we don't fill
    // the temp directory with pixmaps.
    if(!m_lastCover.isEmpty())
        QFile::remove(m_lastCover);
    m_lastCover = tempFile.fileName();

    cover.save(&tempFile, "PNG");
    return tempFile.fileName();
}

// vim: set et sw=4 tw=0 sta:
