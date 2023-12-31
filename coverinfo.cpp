/**
 * Copyright (C) 2004 Nathan Toone <nathan@toonetown.com>
 * Copyright (C) 2005, 2008, 2018 Michael Pyne <mpyne@kde.org>
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

#include "coverinfo.h"

#include <QApplication>
#include <QLabel>
#include <QCursor>
#include <QPixmap>
#include <QMouseEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QScopedPointer>
#include <QScreen>

// Taglib includes
#include <mpegfile.h>
#include <tstring.h>
#include <id3v2tag.h>
#include <attachedpictureframe.h>
#include <flacfile.h>
#include <xiphcomment.h>
#include <mp4coverart.h>
#include <mp4file.h>
#include <mp4tag.h>
#include <mp4item.h>

#include <utility>

#include "mediafiles.h"
#include "collectionlist.h"
#include "playlistsearch.h"
#include "playlistitem.h"
#include "juktag.h"
#include "juk_debug.h"

struct CoverPopup : public QWidget
{
    CoverPopup(QPixmap &image, const QPoint &p) :
        QWidget(nullptr, Qt::WindowFlags(Qt::FramelessWindowHint))
    {
        setAttribute(Qt::WA_DeleteOnClose);

        QHBoxLayout *layout = new QHBoxLayout(this);
        QLabel *label = new QLabel(this);
        layout->addWidget(label);

        const auto pixRatio = this->devicePixelRatioF();
        QSizeF imageSize(label->width(), label->height());

        if (!qFuzzyCompare(pixRatio, 1.0)) {
            imageSize /= pixRatio;
            image.setDevicePixelRatio(pixRatio);
        }

        label->setFrameStyle(QFrame::Box | QFrame::Raised);
        label->setLineWidth(1);
        label->setPixmap(image);

        setGeometry(QRect(p, imageSize.toSize()));

        show();
    }
    virtual void leaveEvent(QEvent *) override { close(); }
    virtual void mouseReleaseEvent(QMouseEvent *) override { close(); }
};

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////


CoverInfo::CoverInfo(const FileHandle &file)
  : m_file(file)
{
}

bool CoverInfo::hasCover() const
{
    if(m_haveCheckedForCover)
        return m_hasCover || m_hasAttachedCover;

    m_haveCheckedForCover = true;

    // Check if it's embedded in the file itself.

    m_hasAttachedCover = hasEmbeddedAlbumArt();

    if(m_hasAttachedCover)
        return true;

    // Look for cover.jpg or cover.png in the directory.
    if(QFile::exists(m_file.fileInfo().absolutePath() + "/cover.jpg") ||
       QFile::exists(m_file.fileInfo().absolutePath() + "/cover.png"))
    {
        m_hasCover = true;
    }

    return m_hasCover;
}

void CoverInfo::clearCover()
{
    m_hasCover = false;
    m_hasAttachedCover = false;

    // Re-search for cover since we may still have a different type of cover.
    m_haveCheckedForCover = false;
}

void CoverInfo::setCover(const QImage &image)
{
    if(image.isNull())
        return;

    m_haveCheckedForCover = true;
    m_hasCover = true;

    QPixmap cover = QPixmap::fromImage(image);

    // TODO: Embed album art into the music track
    qCWarning(JUK_LOG) << "Unable to embed cover image for" << m_file.absFilePath() << "(unsupported)";
}

QPixmap CoverInfo::pixmap(CoverSize size) const
{
    QImage cover;

    // We either have a directory cover image or an embedded art
    if(m_hasCover) {
        QString fileName = m_file.fileInfo().absolutePath() + "/cover.jpg";

        if(!cover.load(fileName)) {
            fileName = m_file.fileInfo().absolutePath() + "/cover.png";

            if(!cover.load(fileName))
                return QPixmap();
        }
        return QPixmap::fromImage(cover);
    }

    // If we get here, see if there is an embedded cover.
    cover = embeddedAlbumArt();
    if(!cover.isNull() && size == Thumbnail)
        cover = scaleCoverToThumbnail(cover);

    if(cover.isNull()) {
        return QPixmap();
    }

    return QPixmap::fromImage(cover);
}

QString CoverInfo::localPathToCover(const QString &fallbackFileName) const
{
    if(hasEmbeddedAlbumArt()) {
        QFile albumArtFile(fallbackFileName);
        if(!albumArtFile.open(QIODevice::ReadWrite)) {
            return QString();
        }

        QImage albumArt = embeddedAlbumArt();
        albumArt.save(&albumArtFile, "PNG");
        return fallbackFileName;
    }

    QString basePath = m_file.fileInfo().absolutePath();
    if(QFile::exists(basePath + "/cover.jpg"))
        return basePath + "/cover.jpg";
    else if(QFile::exists(basePath + "/cover.png"))
        return basePath + "/cover.png";

    return QString();
}

bool CoverInfo::hasEmbeddedAlbumArt() const
{
    QScopedPointer<TagLib::File> fileTag(
            MediaFiles::fileFactoryByType(m_file.absFilePath()));

    if (!fileTag->isValid()) {
        return false;
    }

    if (TagLib::MPEG::File *mpegFile =
            dynamic_cast<TagLib::MPEG::File *>(fileTag.data()))
    {
        TagLib::ID3v2::Tag *id3tag = mpegFile->ID3v2Tag(false);

        if (!id3tag) {
            qCCritical(JUK_LOG) << m_file.absFilePath() << "seems to have invalid ID3 tag";
            return false;
        }

        // Look for attached picture frames.
        TagLib::ID3v2::FrameList frames = id3tag->frameListMap()["APIC"];
        return !frames.isEmpty();
    }
    else if (TagLib::Ogg::XiphComment *oggTag =
            dynamic_cast<TagLib::Ogg::XiphComment *>(fileTag->tag()))
    {
        return !oggTag->pictureList().isEmpty();
    }
    else if (TagLib::FLAC::File *flacFile =
            dynamic_cast<TagLib::FLAC::File *>(fileTag.data()))
    {
        // Look if images are embedded.
        return !flacFile->pictureList().isEmpty();
    }
    else if(TagLib::MP4::File *mp4File =
            dynamic_cast<TagLib::MP4::File *>(fileTag.data()))
    {
        TagLib::MP4::Tag *tag = mp4File->tag();
        if (tag) {
            return tag->contains("covr");
        }
    }

    return false;
}

static QImage embeddedMPEGAlbumArt(TagLib::ID3v2::Tag *id3tag)
{
    if(!id3tag)
        return QImage();

    // Look for attached picture frames.
    TagLib::ID3v2::FrameList frames = id3tag->frameListMap()["APIC"];

    if(frames.isEmpty())
        return QImage();

    // According to the spec attached picture frames have different types.
    // So we should look for the corresponding picture depending on what
    // type of image (i.e. front cover, file info) we want.  If only 1
    // frame, just return that (scaled if necessary).

    TagLib::ID3v2::AttachedPictureFrame *selectedFrame = nullptr;

    if(frames.size() != 1) {
        for(auto *srcFrame : frames) {
            // This must be dynamic_cast<>, TagLib will return UnknownFrame in APIC for
            // encrypted frames.
            auto *frame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame *>(srcFrame);

            // Both thumbnail and full size should use FrontCover, as
            // FileIcon may be too small even for thumbnail.
            if(frame && frame->type() != TagLib::ID3v2::AttachedPictureFrame::FrontCover)
                continue;

            selectedFrame = frame;
            break;
        }
    }

    // If we get here we failed to pick a picture, or there was only one,
    // so just use the first picture.

    if(!selectedFrame)
        selectedFrame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame *>(frames.front());

    if(!selectedFrame) // Could occur for encrypted picture frames.
        return QImage();

    TagLib::ByteVector picture = selectedFrame->picture();
    return QImage::fromData(
            reinterpret_cast<const uchar *>(picture.data()),
            picture.size());
}

static QImage embeddedFLACAlbumArt(const TagLib::List<TagLib::FLAC::Picture *> &flacPictures)
{
    if(flacPictures.isEmpty()) {
        return QImage();
    }

    // Always use first picture - even if multiple are embedded.
    TagLib::ByteVector coverData = flacPictures[0]->data();

    // Will return an image or a null image on error, works either way
    return QImage::fromData(
            reinterpret_cast<const uchar *>(coverData.data()),
            coverData.size());
}

static QImage embeddedMP4AlbumArt(TagLib::MP4::Tag *tag)
{
    if(!tag->contains("covr"))
        return QImage();

    const TagLib::MP4::CoverArtList covers = tag->item("covr").toCoverArtList();
    for(const auto &cover : covers) {
        TagLib::ByteVector coverData = cover.data();

        QImage result = QImage::fromData(
                reinterpret_cast<const uchar *>(coverData.data()),
                coverData.size());

        if(!result.isNull())
            return result;
    }

    // No appropriate image found
    return QImage();
}

void CoverInfo::popup() const
{
    QPixmap image = pixmap(FullSize);
    QPoint mouse  = QCursor::pos();
    QScreen *primaryScreen = QApplication::primaryScreen();
    QRect desktop = primaryScreen->availableGeometry();

    int x = mouse.x();
    int y = mouse.y();
    int height = image.size().height() + 4;
    int width  = image.size().width() + 4;

    // Detect the right direction to pop up (always towards the center of the
    // screen), try to pop up with the mouse pointer 10 pixels into the image in
    // both directions.  If we're too close to the screen border for this margin,
    // show it at the screen edge, accounting for the four pixels (two on each
    // side) for the window border.

    if(x - desktop.x() < desktop.width() / 2)
        x = (x - desktop.x() < 10) ? desktop.x() : (x - 10);
    else
        x = (x - desktop.x() > desktop.width() - 10) ? desktop.width() - width +desktop.x() : (x - width + 10);

    if(y - desktop.y() < desktop.height() / 2)
        y = (y - desktop.y() < 10) ? desktop.y() : (y - 10);
    else
        y = (y - desktop.y() > desktop.height() - 10) ? desktop.height() - height + desktop.y() : (y - height + 10);

    new CoverPopup(image, QPoint(x, y));
}

QImage CoverInfo::embeddedAlbumArt() const
{
    QScopedPointer<TagLib::File> fileTag(
            MediaFiles::fileFactoryByType(m_file.absFilePath()));

    if (auto *mpegFile =
            dynamic_cast<TagLib::MPEG::File *>(fileTag.data()))
    {
        return embeddedMPEGAlbumArt(mpegFile->ID3v2Tag(false));
    }
    else if (auto *oggTag =
            dynamic_cast<TagLib::Ogg::XiphComment *>(fileTag->tag()))
    {
        return embeddedFLACAlbumArt(oggTag->pictureList());
    }
    else if (auto *flacFile =
            dynamic_cast<TagLib::FLAC::File *>(fileTag.data()))
    {
        return embeddedFLACAlbumArt(flacFile->pictureList());
    }
    else if(auto *mp4File =
            dynamic_cast<TagLib::MP4::File *>(fileTag.data()))
    {
        auto *tag = mp4File->tag();
        if (tag) {
            return embeddedMP4AlbumArt(tag);
        }
    }

    return QImage();
}

QImage CoverInfo::scaleCoverToThumbnail(const QImage &image) const
{
    return image.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

// vim: set et sw=4 tw=0 sta:
