/**
 * Copyright (C) 2018 Michael Pyne <mpyne@kde.org>
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

#ifndef JUK_DIRECTORYLOADER_H
#define JUK_DIRECTORYLOADER_H

#include <QObject>
#include <QDirIterator>

#include "filehandle.h"

/**
 * Loads music files and their metadata from a given directory, emitting loaded
 * files in a batch periodically. Intended for use in a separate thread as a
 * worker object.
 */
class DirectoryLoader : public QObject {
    Q_OBJECT

public:
    DirectoryLoader(const QString &dir, QObject *parent = nullptr);

public slots:
    void startLoading();

signals:
    void loadedFiles(FileHandleList files);
    void loadedPlaylist(QString fileName);

private:
    QDirIterator m_dirIterator;
};

#endif // JUK_DIRECTORYLOADER_H
