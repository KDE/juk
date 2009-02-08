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

#ifndef DBUS_COLLECTION_PROXY_H
#define DBUS_COLLECTION_PROXY_H

#include <QtCore/QObject>
#include <QtCore/QStringList> // Required for Q_CLASSINFO ?

class PlaylistCollection;

class DBusCollectionProxy : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.juk.collection")

public:
    DBusCollectionProxy (QObject *parent, PlaylistCollection *collection);
    ~DBusCollectionProxy();

public slots: // Expose to D-Bus
    void openFile(const QString &file);
    void openFile(const QStringList &files);
    void openFile(const QString &playlist, const QString &file);
    void openFile(const QString &playlist, const QStringList &files);

    QString visiblePlaylist();
    QString playingPlaylist();
    QStringList playlists();
    QStringList playlistTracks(const QString &playlist);
    QString trackProperty(const QString &file, const QString &property);

    void createPlaylist(const QString &name);
    void setPlaylist(const QString &name);
    void remove();
    void removeTrack(const QString &playlist, const QStringList &files);

    /**
     * Returns the path to the cover art for the given file.  Returns the empty
     * string if the track has no cover art.  Some tracks have embedded cover
     * art -- in this case JuK returns the path to a temporary file with the
     * extracted cover art.
     */
    QString trackCover(const QString &track);

private:
    PlaylistCollection *m_collection;
    QString m_lastCover;
};

#endif /* DBUS_COLLECTION_PROXY_H */

// vim: set et sw=4 tw=0 sta:
