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
#include <QtDBus/QDBusConnection>

#include "collectionadaptor.h"
#include "playlistcollection.h"

DBusCollectionProxy::DBusCollectionProxy (QObject *parent, PlaylistCollection *collection) :
    QObject(parent), m_collection(collection)
{
    setObjectName("DBusCollectionProxy");

    new CollectionAdaptor(this);
    QDBusConnection::sessionBus().registerObject("/Collection",this);
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
