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
#include <tag.h>
#include <tdebuglistener.h>
#include <mpegfile.h>
#include <vorbisfile.h>
#include <flacfile.h>
#include <xiphcomment.h>
#include <oggflacfile.h>
#include <mpcfile.h>
#include <opusfile.h>
#include <asffile.h>
#include <mp4file.h>

#include "juk_debug.h"

// Intercepts Taglib debugging messages to make part of JuK's debug output
// instead, so that we can pair messages to filenames.
struct TaglibDebugHook final : public TagLib::DebugListener
{
    TaglibDebugHook() : TagLib::DebugListener()
    {
        qCDebug(JUK_LOG) << "Setting Taglib debug listener";
        TagLib::setDebugListener(this);
    }

    virtual ~TaglibDebugHook() override
    {
        TagLib::setDebugListener(nullptr);
    }

    virtual void printMessage(const TagLib::String &msg) override
    {
        qCDebug(JUK_LOG) << m_filename << TStringToQString(msg);
    }

    void setCurrentFile(const QString &filename)
    {
        m_filename = filename;
    }

private:
    QString m_filename;
};

Q_GLOBAL_STATIC(TaglibDebugHook, debugHook);

namespace MediaFiles {
    static QStringList savedMimeTypes;

    static const char mp3Type[]  = "audio/mpeg";
    static const char oggType[]  = "audio/ogg";
    static const char flacType[] = "audio/x-flac";
    static const char mpcType[]  = "audio/x-musepack";
    static const char m3uType[]  = "audio/x-mpegurl";

    static const char vorbisType[]  = "audio/x-vorbis+ogg";
    static const char oggflacType[] = "audio/x-flac+ogg";
    static const char oggopusType[] = "audio/x-opus+ogg";

    static const char asfType[] = "video/x-ms-asf";

    static const char mp4Type[] = "audio/mp4";
    static const char mp4AudiobookType[] = "audio/x-m4b";

    static const char *const mediaTypes[] = {
        mp3Type, oggType, flacType, mpcType, vorbisType, oggflacType
        ,asfType
        ,mp4Type
        ,mp4AudiobookType
        ,oggopusType
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
    dialog.setWindowTitle(i18nc("@title:window, open audio file", "Open"));

    if(dialog.exec()) {
        return dialog.selectedFiles();
    }

    return QStringList();
}

QString MediaFiles::savePlaylistDialog(const QString &playlistName, QWidget *parent)
{
    QString fileName = QFileDialog::getSaveFileName(
        parent, i18nc("@title:window", "Save Playlist %1", playlistName),
        getMusicDir(),
        i18nc("For save dialog, %1 is always .m3u", "Playlists (*%1)", playlistExtension));
    return fileName;
}

TagLib::File *MediaFiles::fileFactoryByType(const QString &fileName)
{
    QMimeDatabase db;
    QMimeType result = db.mimeTypeForFile(fileName);
    if(!result.isValid())
        return nullptr;

    TagLib::File *file(nullptr);
    debugHook->setCurrentFile(fileName);
    QByteArray encodedFileName(QFile::encodeName(fileName));

    if(result.inherits(QLatin1String(mp3Type)))
        file = new TagLib::MPEG::File(encodedFileName.constData());
    else if(result.inherits(QLatin1String(flacType)))
        file = new TagLib::FLAC::File(encodedFileName.constData());
    else if(result.inherits(QLatin1String(vorbisType)))
        file = new TagLib::Vorbis::File(encodedFileName.constData());
    else if(result.inherits(QLatin1String(asfType)))
        file = new TagLib::ASF::File(encodedFileName.constData());
    else if(result.inherits(QLatin1String(mp4Type)) || result.inherits(QLatin1String(mp4AudiobookType)))
        file = new TagLib::MP4::File(encodedFileName.constData());
    else if(result.inherits(QLatin1String(mpcType)))
        file = new TagLib::MPC::File(encodedFileName.constData());
    else if(result.inherits(QLatin1String(oggflacType)))
        file = new TagLib::Ogg::FLAC::File(encodedFileName.constData());
    else if(result.inherits(QLatin1String(oggopusType)) ||
            (result.inherits(QLatin1String(oggType)) && fileName.endsWith(QLatin1String(".opus")))
            )
    {
        file = new TagLib::Ogg::Opus::File(encodedFileName.constData());
    }

    return file;
}

bool MediaFiles::isMediaFile(const QString &fileName)
{
    QMimeDatabase db;
    QMimeType result = db.mimeTypeForFile(fileName);
    if(!result.isValid())
        return false;

    // Search through our table of media types for a match
    const auto validMimeTypes(mimeTypes());
    for(const auto &mimeType : validMimeTypes) {
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

bool MediaFiles::isASF(const QString &fileName)
{
    return isFileOfMimeType(fileName, asfType);
}

bool MediaFiles::isMP4(const QString &fileName)
{
    return isFileOfMimeType(fileName, mp4Type) || isFileOfMimeType(fileName, mp4AudiobookType);
}

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
