/***************************************************************************
                          filehandle.h
                             -------------------
    begin                : Sun Feb 29 2004
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

#ifndef JUK_FILEHANDLE_H
#define JUK_FILEHANLDE_H

class Tag;

/**
 * An value based, explicitly shared wrapper around file related information
 * used in JuK's playlists.
 */

class FileHandle
{
public:
    FileHandle();
    FileHandle(const FileHandle &f);
    FileHandle(const QFileInfo &info, const QString &path = QString::null);
    explicit FileHandle(const QString &path);
    ~FileHandle();

    /**
     * Forces the FileHandle to reread its information from the disk.
     */
    void refresh();
    void setFile(const QString &path);

    Tag *tag() const;
    QString absFilePath() const;
    const QFileInfo &fileInfo() const;

    bool current() const;
    const QDateTime &lastModified() const;

    FileHandle &operator=(const FileHandle &f);
    bool operator==(const FileHandle &f) const;

private:
    class FileHandlePrivate;
    FileHandlePrivate *d;
};

#endif
