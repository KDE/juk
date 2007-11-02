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
#include <kdebug.h>
#include <klocale.h>
#include <kio/netaccess.h>

#include "mediafiles.h"

#include <taglib/tag.h>
#if (TAGLIB_MAJOR_VERSION>1) ||  \
   ((TAGLIB_MAJOR_VERSION==1) && (TAGLIB_MINOR_VERSION>=2))
#define TAGLIB_1_2
#endif
#if (TAGLIB_MAJOR_VERSION>1) ||  \
   ((TAGLIB_MAJOR_VERSION==1) && (TAGLIB_MINOR_VERSION>=3))
#define TAGLIB_1_3
#endif

namespace MediaFiles {
    QStringList mimeTypes();

    static const char mp3Type[]  = "audio/mpeg";
    static const char oggType[]  = "application/ogg";
    static const char flacType[] = "audio/x-flac";
    static const char mpcType[]  = "audio/x-musepack";
    static const char m3uType[]  = "audio/mpegurl";

    static const char vorbisType[]  = "audio/x-vorbis";
    static const char oggflacType[] = "audio/x-oggflac";

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

    return convertURLsToLocal(dialog.selectedFiles());
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

    return result->is(mp3Type) || result->is(oggType) || result->is(flacType)
#ifdef TAGLIB_1_3
                               || result->is(mpcType)
#endif
    ;
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

bool MediaFiles::isMPC(const QString &fileName)
{
    KMimeType::Ptr result = KMimeType::findByPath(fileName, 0, true);
    return result->is(mpcType);
}

bool MediaFiles::isVorbis(const QString &fileName)
{
    KMimeType::Ptr result = KMimeType::findByPath(fileName, 0, false);
    return result->is(vorbisType);
}

bool MediaFiles::isOggFLAC(const QString &fileName)
{
    KMimeType::Ptr result = KMimeType::findByPath(fileName, 0, false);
    return result->is(oggflacType);
}

QStringList MediaFiles::mimeTypes()
{
    QStringList l;

    l << mp3Type << oggType << flacType << m3uType << vorbisType
#ifdef TAGLIB_1_2
    << oggflacType
#endif
#ifdef TAGLIB_1_3
    << mpcType
#endif
    ;
    return l;
}

QStringList MediaFiles::convertURLsToLocal(const QStringList &urlList, QWidget *w)
{
    QStringList result;
    KURL localUrl;

    for(QStringList::ConstIterator it = urlList.constBegin(); it != urlList.constEnd(); ++it) {
	localUrl = KIO::NetAccess::mostLocalURL(KURL::fromPathOrURL(*it), w);

	if(!localUrl.isLocalFile())
	    kdDebug(65432) << localUrl << " is not a local file, skipping.\n";
	else
	    result.append(localUrl.path());
    }

    return result;
}
