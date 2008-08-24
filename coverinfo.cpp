/***************************************************************************
    copyright            : (C) 2004 Nathan Toone
                         : (C) 2005, 2008 Michael Pyne <michael.pyne@kdemail.net>
    email                : nathan@toonetown.com
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "coverinfo.h"

#include <kglobal.h>
#include <kapplication.h>
#include <kstandarddirs.h>
#include <kdebug.h>

#include <QRegExp>
#include <QLabel>
#include <QCursor>
#include <QPixmap>
#include <QMouseEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QDesktopWidget>
#include <QImage>

#include <taglib/mpegfile.h>
#include <taglib/tstring.h>
#include <taglib/id3v2tag.h>
#include <taglib/attachedpictureframe.h>

#include "collectionlist.h"
#include "playlistsearch.h"
#include "playlistitem.h"
#include "tag.h"

struct CoverPopup : public QWidget
{
    CoverPopup(const QPixmap &image, const QPoint &p) :
        QWidget(0, Qt::WDestructiveClose | Qt::WX11BypassWM)
    {
        QHBoxLayout *layout = new QHBoxLayout(this);
        QLabel *label = new QLabel(this);

        layout->addWidget(label);
        label->setFrameStyle(QFrame::Box | QFrame::Raised);
        label->setLineWidth(1);
        label->setPixmap(image);

        setGeometry(p.x(), p.y(), label->width(), label->height());
        show();
    }
    virtual void leaveEvent(QEvent *) { close(); }
    virtual void mouseReleaseEvent(QMouseEvent *) { close(); }
};

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////


CoverInfo::CoverInfo(const FileHandle &file) :
    m_file(file),
    m_hasCover(false),
    m_hasAttachedCover(false),
    m_haveCheckedForCover(false),
    m_coverKey(CoverManager::NoMatch)
{

}

bool CoverInfo::hasCover() const
{
    if(m_haveCheckedForCover)
        return m_hasCover || m_hasAttachedCover;

    m_haveCheckedForCover = true;

    // Check for new-style covers.  First let's determine what our coverKey is
    // if it's not already set, as that's also tracked by the CoverManager.
    if(m_coverKey == CoverManager::NoMatch)
        m_coverKey = CoverManager::idForTrack(m_file.absFilePath());

    // We were assigned a key, let's see if we already have a cover.  Notice
    // that due to the way the CoverManager is structured, we should have a
    // cover if we have a cover key.  If we don't then either there's a logic
    // error, or the user has been mucking around where they shouldn't.
    if(m_coverKey != CoverManager::NoMatch)
        m_hasCover = CoverManager::hasCover(m_coverKey);

    // Check if it's embedded in the file itself.

    QByteArray filePath = QFile::encodeName(m_file.absFilePath());
    TagLib::MPEG::File mpegFile(filePath.constData(), true, TagLib::AudioProperties::Accurate);
    TagLib::ID3v2::Tag *id3tag = mpegFile.ID3v2Tag(false);

    if(!id3tag)
        return m_hasCover;

    // Look for attached picture frames.
    TagLib::ID3v2::FrameList frames = id3tag->frameListMap()["APIC"];
    m_hasAttachedCover = !frames.isEmpty();

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

    // We don't need to call removeCover because the CoverManager will
    // automatically unlink the cover if we were the last track to use it.
    CoverManager::setIdForTrack(m_file.absFilePath(), CoverManager::NoMatch);
    m_coverKey = CoverManager::NoMatch;
}

void CoverInfo::setCover(const QImage &image)
{
    if(image.isNull())
        return;

    m_haveCheckedForCover = true;
    m_hasCover = true;

    QPixmap cover = QPixmap::fromImage(image);

    // If we use replaceCover we'll change the cover for every other track
    // with the same coverKey, which we don't want since that case will be
    // handled by Playlist.  Instead just replace this track's cover.
    m_coverKey = CoverManager::addCover(cover, m_file.tag()->artist(), m_file.tag()->album());
    if(m_coverKey != CoverManager::NoMatch)
        CoverManager::setIdForTrack(m_file.absFilePath(), m_coverKey);
}

void CoverInfo::setCoverId(coverKey id)
{
    m_coverKey = id;
    m_haveCheckedForCover = true;
    m_hasCover = id != CoverManager::NoMatch;

    // Inform CoverManager of the change.
    CoverManager::setIdForTrack(m_file.absFilePath(), m_coverKey);
}

void CoverInfo::applyCoverToWholeAlbum(bool overwriteExistingCovers) const
{
    QString artist = m_file.tag()->artist();
    QString album = m_file.tag()->album();
    PlaylistSearch::ComponentList components;
    ColumnList columns;

    columns.append(PlaylistItem::ArtistColumn);
    components.append(PlaylistSearch::Component(artist, false, columns, PlaylistSearch::Component::Exact));

    columns.clear();
    columns.append(PlaylistItem::AlbumColumn);
    components.append(PlaylistSearch::Component(album, false, columns, PlaylistSearch::Component::Exact));

    PlaylistList playlists;
    playlists.append(CollectionList::instance());

    PlaylistSearch search(playlists, components, PlaylistSearch::MatchAll);

    // Search done, iterate through results.

    PlaylistItemList results = search.matchedItems();
    PlaylistItemList::ConstIterator it = results.constBegin();
    for(; it != results.constEnd(); ++it) {

        // Don't worry about files that somehow already have a tag,
        // unless the conversion is forced.
        if(!overwriteExistingCovers && (*it)->file().coverInfo()->coverId() != CoverManager::NoMatch)
            continue;

        (*it)->file().coverInfo()->setCoverId(m_coverKey);
    }
}

coverKey CoverInfo::coverId() const
{
    if(m_coverKey == CoverManager::NoMatch)
        m_coverKey = CoverManager::idForTrack(m_file.absFilePath());

    return m_coverKey;
}

QPixmap CoverInfo::pixmap(CoverSize size) const
{
    if(hasCover() && m_coverKey != CoverManager::NoMatch) {
        return CoverManager::coverFromId(m_coverKey,
            size == Thumbnail
               ? CoverManager::Thumbnail
               : CoverManager::FullSize);
    }

    // If m_hasCover is still true we must have a directory cover image.
    if(m_hasCover) {
        QString fileName = m_file.fileInfo().absolutePath() + "/cover.jpg";

        QImage cover;
        if(!cover.load(fileName)) {
            fileName = m_file.fileInfo().absolutePath() + "/cover.png";

            if(!cover.load(fileName))
                return QPixmap();
        }

        if(size == Thumbnail)
            cover = scaleCoverToThumbnail(cover);

        return QPixmap::fromImage(cover);
    }

    // If we get here, see if there is an embedded cover.

    QByteArray filePath = QFile::encodeName(m_file.absFilePath());
    TagLib::MPEG::File mpegFile(filePath.constData(), true, TagLib::AudioProperties::Accurate);
    TagLib::ID3v2::Tag *id3tag = mpegFile.ID3v2Tag(false);

    if(!id3tag)
        return QPixmap();

    // Look for attached picture frames.
    TagLib::ID3v2::FrameList frames = id3tag->frameListMap()["APIC"];

    if(frames.isEmpty())
        return QPixmap();

    // According to the spec attached picture frames have different types.
    // So we should look for the corresponding picture depending on what
    // type of image (i.e. front cover, file info) we want.  If only 1
    // frame, just return that (scaled if necessary).

    TagLib::ID3v2::AttachedPictureFrame *selectedFrame = 0;

    if(frames.size() != 1) {
        TagLib::ID3v2::FrameList::Iterator it = frames.begin();
        for(; it != frames.end(); ++it) {

            // This must be dynamic_cast<>, TagLib will return UnknownFrame in APIC for
            // encrypted frames.
            TagLib::ID3v2::AttachedPictureFrame *frame =
                dynamic_cast<TagLib::ID3v2::AttachedPictureFrame *>(*it);

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
        return QPixmap();

    QByteArray pictureData = QByteArray(selectedFrame->picture().data(),
                                        selectedFrame->picture().size());
    QImage attachedImage = QImage::fromData(pictureData);

    kDebug() << "Embedded cover art of size" << attachedImage.size();

    if(size == Thumbnail)
        attachedImage = scaleCoverToThumbnail(attachedImage);

    return QPixmap::fromImage(attachedImage);
}

void CoverInfo::popup() const
{
    QPixmap image = pixmap(FullSize);
    QPoint mouse  = QCursor::pos();
    QRect desktop = QApplication::desktop()->screenGeometry(mouse);

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

QImage CoverInfo::scaleCoverToThumbnail(const QImage &image) const
{
    return image.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

// vim: set et sw=4 tw=0 sta:
