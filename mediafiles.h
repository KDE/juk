/***************************************************************************
                          mediafiles.h
                             -------------------
    begin                : Fri Sep 13 2002
    copyright            : (C) 2002 by Scott Wheeler
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

#ifndef MEDIAFILES_H
#define MEDIAFILES_H

#include <qstringlist.h>

/**
 * A namespace for file JuK's file related functions.  The goal is to hide
 * all specific knowledge of mimetypes and file extensions here.
 */
namespace MediaFiles
{
    /**
     * Creates a JuK specific KFileDialog with the specified parent.
     */
    QStringList openDialog(QWidget *parent = 0);

    /**
     * Creates a JuK specific KFileDialog for saving a playlist with the name
     * playlistName and the specified parent and returns the file name.
     */
    QString savePlaylistDialog(const QString &playlistName, QWidget *parent = 0);

    /**
     * Returns true if fileName is a supported media file.
     */
    bool isMediaFile(const QString &fileName);

    /**
     * Returns true if fileName is a supported playlist file.
     */
    bool isPlaylistFile(const QString &fileName);

    /**
     * Returns true if fileName is an mp3 file.
     */
    bool isMP3(const QString &fileName);

    /**
     * Returns true if fileName is an Ogg file.
     */
    bool isOgg(const QString &fileName);
};

#endif
