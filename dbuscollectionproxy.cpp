/***************************************************************************
    copyright            : (C) 2009 by Michael Pyne
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

#include "dbuscollectionproxy.h"

#include <QtCore/QStringList>
#include <QtCore/QFile>
#include <QtGui/QPixmap>
#include <QtDBus/QDBusConnection>

#include <KTemporaryFile>
#include <kdebug.h>

#include "collectionadaptor.h"
#include "playlistcollection.h"
#include "covermanager.h"
#include "collectionlist.h"
#include "coverinfo.h"
#include "filehandle.h"

DBusCollectionProxy::DBusCollectionProxy (QObject *parent, PlaylistCollection *collection) :
    QObject(parent), m_collection(collection)
{
    setObjectName("DBusCollectionProxy");

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
    coverKey id = CoverManager::idForTrack(track);
    if(id != CoverManager::NoMatch) {
        CoverDataPtr coverData = CoverManager::coverInfo(id);
        return coverData->path;
    }

    // No cover, let's see if one is embedded.
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
    KTemporaryFile tempFile;

    tempFile.setSuffix(".png");
    tempFile.setAutoRemove(false);

    if(!tempFile.open()) {
        kError() << "Unable to open temporary file for embedded cover art.";
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
