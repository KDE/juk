/**
 * Copyright (C) 2002-2004 Scott Wheeler <wheeler@kde.org>
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

#include "mediafiles.h"

#include <KLocalizedString>
#include <KIO/StatJob>
#include <KJobWidgets>

#include <QWidget>
#include <QFile>
#include <QUrl>
#include <QFileDialog>
#include <QStandardPaths>
#include <QMimeType>
#include <QMimeDatabase>

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

#include "juk_debug.h"

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

#if TAGLIB_HAS_OPUSFILE
    static const char oggopusType[] = "audio/x-opus+ogg";
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
#if TAGLIB_HAS_OPUSFILE
        ,oggopusType
#endif
    };

    static const char playlistExtension[] = ".m3u";
}

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(arr[0]))

static QString getMusicDir()
{
    const auto musicLocation =
        QStandardPaths::writableLocation(QStandardPaths::MusicLocation);

    QDir musicDir(musicLocation);
    if (Q_UNLIKELY(
        !musicDir.exists() &&
        musicDir.isAbsolute() && // safety precaution here
        !musicDir.mkpath(musicLocation)))
    {
        qCWarning(JUK_LOG) << "Failed to create music dir:" << musicLocation;
    }

    return musicLocation;
}

QStringList MediaFiles::openDialog(QWidget *parent)
{
    QFileDialog dialog(parent);

    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setMimeTypeFilters(mimeTypes());
    // limit to only file:// for now
    dialog.setSupportedSchemes(QStringList() << QStringLiteral("file"));
    dialog.setDirectory(getMusicDir());
    dialog.setWindowTitle(i18nc("open audio file", "Open"));

    if(dialog.exec()) {
        return dialog.selectedFiles();
    }

    return QStringList();
}

QString MediaFiles::savePlaylistDialog(const QString &playlistName, QWidget *parent)
{
    QString fileName = QFileDialog::getSaveFileName(
        parent,
        i18n("Save Playlist") + QStringLiteral(" ") + playlistName,
        getMusicDir(),
        QStringLiteral("Playlists (*") + playlistExtension + QStringLiteral(")")
    );
    return fileName;
}

TagLib::File *MediaFiles::fileFactoryByType(const QString &fileName)
{
    QMimeDatabase db;
    QMimeType result = db.mimeTypeForFile(fileName);
    if(!result.isValid())
        return nullptr;

    TagLib::File *file(nullptr);
    QByteArray encodedFileName(QFile::encodeName(fileName));

    if(result.inherits(QLatin1String(mp3Type)))
        file = new TagLib::MPEG::File(encodedFileName.constData());
    else if(result.inherits(QLatin1String(flacType)))
        file = new TagLib::FLAC::File(encodedFileName.constData());
    else if(result.inherits(QLatin1String(vorbisType)))
        file = new TagLib::Vorbis::File(encodedFileName.constData());
#ifdef TAGLIB_WITH_ASF
    else if(result.inherits(QLatin1String(asfType)))
        file = new TagLib::ASF::File(encodedFileName.constData());
#endif
#ifdef TAGLIB_WITH_MP4
    else if(result.inherits(QLatin1String(mp4Type)) || result.inherits(QLatin1String(mp4AudiobookType)))
        file = new TagLib::MP4::File(encodedFileName.constData());
#endif
    else if(result.inherits(QLatin1String(mpcType)))
        file = new TagLib::MPC::File(encodedFileName.constData());
    else if(result.inherits(QLatin1String(oggflacType)))
        file = new TagLib::Ogg::FLAC::File(encodedFileName.constData());
#if TAGLIB_HAS_OPUSFILE
    else if(result.inherits(QLatin1String(oggopusType)) ||
            (result.inherits(QLatin1String(oggType)) && fileName.endsWith(QLatin1String(".opus")))
            )
    {
        file = new TagLib::Ogg::Opus::File(encodedFileName.constData());
    }
#endif

    return file;
}

bool MediaFiles::isMediaFile(const QString &fileName)
{
    QMimeDatabase db;
    QMimeType result = db.mimeTypeForFile(fileName);
    if(!result.isValid())
        return false;

    // Search through our table of media types for a match
    for(const auto &mimeType : mimeTypes()) {
        if(result.inherits(mimeType))
            return true;
    }

    return false;
}

static bool isFileOfMimeType(const QString &fileName, const QString &mimeType)
{
    QMimeDatabase db;
    QMimeType result = db.mimeTypeForFile(fileName);
    return result.isValid() && result.inherits(mimeType);
}

bool MediaFiles::isPlaylistFile(const QString &fileName)
{
    return isFileOfMimeType(fileName, m3uType);
}

bool MediaFiles::isMP3(const QString &fileName)
{
    return isFileOfMimeType(fileName, mp3Type);
}

bool MediaFiles::isOgg(const QString &fileName)
{
    return isFileOfMimeType(fileName, oggType);
}

bool MediaFiles::isFLAC(const QString &fileName)
{
    return isFileOfMimeType(fileName, flacType);
}

bool MediaFiles::isMPC(const QString &fileName)
{
    return isFileOfMimeType(fileName, mpcType);
}

bool MediaFiles::isVorbis(const QString &fileName)
{
    return isFileOfMimeType(fileName, vorbisType);
}

#ifdef TAGLIB_WITH_ASF
bool MediaFiles::isASF(const QString &fileName)
{
    return isFileOfMimeType(fileName, asfType);
}
#endif

#ifdef TAGLIB_WITH_MP4
bool MediaFiles::isMP4(const QString &fileName)
{
    return isFileOfMimeType(fileName, mp4Type) || isFileOfMimeType(fileName, mp4AudiobookType);
}
#endif

bool MediaFiles::isOggFLAC(const QString &fileName)
{
    return isFileOfMimeType(fileName, oggflacType);
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

QStringList MediaFiles::convertURLsToLocal(const QList<QUrl> &urlList, QWidget *w)
{
    QStringList result;
    QUrl localUrl;

    for(const auto &url : urlList) {
        auto localizerJob = KIO::mostLocalUrl(url);
        KJobWidgets::setWindow(localizerJob, w);

        if(localizerJob->exec() && (localUrl = localizerJob->mostLocalUrl()).isLocalFile())
            result.append(localUrl.path());
        else
            qCDebug(JUK_LOG) << url << " is not a local file, skipping.";
    }

    return result;
}

// vim: set et sw=4 tw=0 sta:
