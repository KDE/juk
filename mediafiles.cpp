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

#include <kfiledialog.h>
#include <klocale.h>


#include "mediafiles.h"

namespace MediaFiles {
    QStringList mimeTypes();

    static const char mp3Type[]  = "audio/x-mp3";
    static const char oggType[]  = "application/ogg";
    static const char flacType[] = "audio/x-flac";
    static const char m3uType[]  = "audio/mpegurl";

    static const char playlistExtension[] = ".m3u";
}

QStringList MediaFiles::openDialog(QWidget *parent)
{
    KFileDialog dialog(QString::null, QString::null, parent, "filedialog", true);
    dialog.setOperationMode(KFileDialog::Opening);

    dialog.setCaption(i18n("Open"));
    dialog.setMode(KFile::Files | KFile::LocalOnly);
    // dialog.ops->clearHistory();
    dialog.setMimeFilter(mimeTypes());

    dialog.exec();

    return dialog.selectedFiles();
}

QString MediaFiles::savePlaylistDialog(const QString &playlistName, QWidget *parent)
{
    QString fileName = KFileDialog::getSaveFileName(playlistName + playlistExtension,
                                                    QString("*").append(playlistExtension),
                                                    parent,
                                                    i18n("Playlists"));
    if(!fileName.isEmpty() && !fileName.endsWith(playlistExtension))
       fileName.append(playlistExtension);

    return fileName;
}

bool MediaFiles::isMediaFile(const QString &fileName)
{
    KMimeType::Ptr result = KMimeType::findByPath(fileName, 0, true);

    return result->is(mp3Type) || result->is(oggType) || result->is(flacType);
}

bool MediaFiles::isPlaylistFile(const QString &fileName)
{
    KMimeType::Ptr result = KMimeType::findByPath(fileName, 0, true);
    return result->is(m3uType);
}

bool MediaFiles::isMP3(const QString &fileName)
{
    KMimeType::Ptr result = KMimeType::findByPath(fileName, 0, true);
    return result->is(mp3Type);
}

bool MediaFiles::isOgg(const QString &fileName)
{
    KMimeType::Ptr result = KMimeType::findByPath(fileName, 0, true);
    return result->is(oggType);
}

bool MediaFiles::isFLAC(const QString &fileName)
{
    KMimeType::Ptr result = KMimeType::findByPath(fileName, 0, true);
    return result->is(flacType);
}

QStringList MediaFiles::mimeTypes()
{
    QStringList l;

    l << mp3Type << oggType << flacType << m3uType;

    return l;
}
