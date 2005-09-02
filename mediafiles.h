/***************************************************************************
    begin                : Fri Sep 13 2002
    copyright            : (C) 2002 - 2004 by Scott Wheeler
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
     * Returns true if fileName is a mp3 file.
     */
    bool isMP3(const QString &fileName);

    /**
     * Returns true if fileName is a mpc (aka musepack) file.
     */
    bool isMPC(const QString &fileName);

    /**
     * Returns true if fileName is an Ogg file.
     */
    bool isOgg(const QString &fileName);

    /**
     * Returns true if fileName is a FLAC file.
     */
    bool isFLAC(const QString &fileName);

    /**
     * Returns true if fileName is an Ogg/Vorbis file.
     */
    bool isVorbis(const QString &fileName);

    /**
     * Returns true if fileName is an Ogg/FLAC file.
     */
    bool isOggFLAC(const QString &fileName);

    /**
     * Returns a list of absolute local filenames, mapped from \p urlList.
     * Any URLs in urlList that aren't really local files will be stripped
     * from the result (so result.size() may be < urlList.size()).
     *
     * @param urlList list of file names or URLs to convert.
     * @param w KIO may need the widget to handle user interaction.
     * @return list of all local files in urlList, converted to absolute paths.
     */
    QStringList convertURLsToLocal(const QStringList &urlList, QWidget *w = 0);
}

#endif
