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

#include "mediafiles.h"

#include <kfiledialog.h>
#include <kdebug.h>
#include <klocale.h>
#include <kurl.h>
#include <kio/netaccess.h>

#include <QtGui/QWidget>
#include <QtCore/QFile>

#include <taglib.h>
#include <taglib_config.h>
#include <tag.h>
#include <mpegfile.h>
#include <vorbisfile.h>
#include <flacfile.h>
#include <xiphcomment.h>
#include <oggflacfile.h>
#include <mpcfile.h>

#include "config-juk.h"
#if TAGLIB_HAS_OPUSFILE
# include <opusfile.h>
#endif

#ifdef TAGLIB_WITH_ASF
#include <asffile.h>
#endif

#ifdef TAGLIB_WITH_MP4
#include <mp4file.h>
#endif

namespace MediaFiles {
    static QStringList savedMimeTypes;

    static const char mp3Type[]  = "audio/mpeg";
    static const char oggType[]  = "audio/ogg";
    static const char flacType[] = "audio/x-flac";
    static const char mpcType[]  = "audio/x-musepack";
    static const char m3uType[]  = "audio/x-mpegurl";

    static const char vorbisType[]  = "audio/x-vorbis+ogg";
    static const char oggflacType[] = "audio/x-flac+ogg";

#ifdef TAGLIB_WITH_ASF
    static const char asfType[] = "video/x-ms-asf";
#endif

#ifdef TAGLIB_WITH_MP4
    static const char mp4Type[] = "audio/mp4";
    static const char mp4AudiobookType[] = "audio/x-m4b";
#endif

    static const char *const mediaTypes[] = {
        mp3Type, oggType, flacType, mpcType, vorbisType, oggflacType
#ifdef TAGLIB_WITH_ASF
        ,asfType
#endif
#ifdef TAGLIB_WITH_MP4
        ,mp4Type
        ,mp4AudiobookType
#endif
    };

    static const char playlistExtension[] = ".m3u";
}

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(arr[0]))

QStringList MediaFiles::openDialog(QWidget *parent)
{
    KFileDialog *dialog = new KFileDialog(KUrl(), QString(), parent);

    dialog->setOperationMode(KFileDialog::Opening);

    dialog->setCaption(i18nc("open audio file", "Open"));
    dialog->setMode(KFile::Files | KFile::LocalOnly);
    // dialog.ops->clearHistory();
    dialog->setMimeFilter(mimeTypes());

    dialog->exec();

    // Only local files included in this list.
    QStringList selectedFiles = dialog->selectedFiles();

    delete dialog;

    return selectedFiles;
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

TagLib::File *MediaFiles::fileFactoryByType(const QString &fileName)
{
    KMimeType::Ptr result = KMimeType::findByPath(fileName);
    if(!result->isValid())
        return 0;

    TagLib::File *file(0);
    QByteArray encodedFileName(QFile::encodeName(fileName));

    if(result->is(mp3Type))
        file = new TagLib::MPEG::File(encodedFileName.constData());
    else if(result->is(flacType))
        file = new TagLib::FLAC::File(encodedFileName.constData());
    else if(result->is(vorbisType))
        file = new TagLib::Vorbis::File(encodedFileName.constData());
#ifdef TAGLIB_WITH_ASF
    else if(result->is(asfType))
        file = new TagLib::ASF::File(encodedFileName.constData());
#endif
#ifdef TAGLIB_WITH_MP4
    else if(result->is(mp4Type) || result->is(mp4AudiobookType))
        file = new TagLib::MP4::File(encodedFileName.constData());
#endif
    else if(result->is(mpcType))
        file = new TagLib::MPC::File(encodedFileName.constData());
    else if(result->is(oggflacType))
        file = new TagLib::Ogg::FLAC::File(encodedFileName.constData());
#if TAGLIB_HAS_OPUSFILE
    else if(result->is(oggType) && fileName.endsWith(QLatin1String(".opus")))
        file = new TagLib::Ogg::Opus::File(encodedFileName.constData());
#endif

    return file;
}

bool MediaFiles::isMediaFile(const QString &fileName)
{
    KMimeType::Ptr result = KMimeType::findByPath(fileName);
    if(!result->isValid())
        return false;

    // Search through our table of media types for a match
    for(unsigned i = 0; i < ARRAY_SIZE(mediaTypes); ++i) {
        if(result->is(mediaTypes[i]))
            return true;
    }

    return false;
}

bool MediaFiles::isPlaylistFile(const QString &fileName)
{
    KMimeType::Ptr result = KMimeType::findByPath(fileName);
    return result->is(m3uType);
}

bool MediaFiles::isMP3(const QString &fileName)
{
    KMimeType::Ptr result = KMimeType::findByPath(fileName);
    return result->is(mp3Type);
}

bool MediaFiles::isOgg(const QString &fileName)
{
    KMimeType::Ptr result = KMimeType::findByPath(fileName);
    return result->is(oggType);
}

bool MediaFiles::isFLAC(const QString &fileName)
{
    KMimeType::Ptr result = KMimeType::findByPath(fileName);
    return result->is(flacType);
}

bool MediaFiles::isMPC(const QString &fileName)
{
    KMimeType::Ptr result = KMimeType::findByPath(fileName);
    return result->is(mpcType);
}

bool MediaFiles::isVorbis(const QString &fileName)
{
    KMimeType::Ptr result = KMimeType::findByPath(fileName);
    return result->is(vorbisType);
}

#ifdef TAGLIB_WITH_ASF
bool MediaFiles::isASF(const QString &fileName)
{
    KMimeType::Ptr result = KMimeType::findByPath(fileName);
    return result->is(asfType);
}
#endif

#ifdef TAGLIB_WITH_MP4
bool MediaFiles::isMP4(const QString &fileName)
{
    KMimeType::Ptr result = KMimeType::findByPath(fileName);
    return result->is(mp4Type) || result->is(mp4AudiobookType);
}
#endif

bool MediaFiles::isOggFLAC(const QString &fileName)
{
    KMimeType::Ptr result = KMimeType::findByPath(fileName);
    return result->is(oggflacType);
}

QStringList MediaFiles::mimeTypes()
{
    if(!savedMimeTypes.isEmpty())
        return savedMimeTypes;

    for(unsigned i = 0; i < ARRAY_SIZE(mediaTypes); ++i) {
        savedMimeTypes << QLatin1String(mediaTypes[i]);
    }

    return savedMimeTypes;
}

QStringList MediaFiles::convertURLsToLocal(const KUrl::List &urlList, QWidget *w)
{
    QStringList result;
    KUrl localUrl;

    foreach(const KUrl &url, urlList) {
        localUrl = KIO::NetAccess::mostLocalUrl(url, w);

        if(!localUrl.isLocalFile())
            kDebug() << localUrl << " is not a local file, skipping.\n";
        else
            result.append(localUrl.path());
    }

    return result;
}

// vim: set et sw=4 tw=0 sta:
