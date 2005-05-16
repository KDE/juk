/***************************************************************************
    copyright            : (C) 2004 Nathan Toone
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

#include <kglobal.h>
#include <kapplication.h>
#include <kstandarddirs.h>
#include <kdebug.h>

#include <qregexp.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qcursor.h>

#include "coverinfo.h"
#include "tag.h"

struct CoverPopup : public QWidget
{
    CoverPopup(const QPixmap &image, const QPoint &p) :
        QWidget(0, 0, WDestructiveClose | WX11BypassWM)
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
    m_haveCheckedForCover(false),
    m_coverKey(CoverManager::NoMatch)
{

}

bool CoverInfo::hasCover()
{
    if(m_haveCheckedForCover)
        return m_hasCover;

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

    // If we don't have a cover, first check if the CoverManager
    // has one matching our album and artist.
    if(!m_hasCover) {
        QString artist = m_file.tag()->artist();
        QString album = m_file.tag()->album();
        coverKey match = CoverManager::idFromMetadata(artist, album);

        if(match != CoverManager::NoMatch) {
            m_hasCover = true;
            m_coverKey = match;

            // Make CoverManager remember this association
            CoverManager::setIdForTrack(m_file.absFilePath(), match);
        }
    }

    // We *still* don't have it?  Check the old-style covers then.
    if(!m_hasCover) {
        m_hasCover = QFile(coverLocation(FullSize)).exists();

        if(m_hasCover) {
            // Ah, old-style cover.  Let's transfer it to the new system.
            kdDebug() << "Found old style cover for " << m_file.absFilePath() << endl;

            QString artist = m_file.tag()->artist();
            QString album = m_file.tag()->album();
            m_coverKey = CoverManager::addCover(coverLocation(FullSize), artist, album);

            if(m_coverKey != CoverManager::NoMatch)
                CoverManager::setIdForTrack(m_file.absFilePath(), m_coverKey);
            else
                kdDebug() << "We were unable to replace the old style cover.\n";
        }
    }

    return m_hasCover;
}

void CoverInfo::clearCover()
{
    QFile::remove(coverLocation(FullSize));
    QFile::remove(coverLocation(Thumbnail));
    m_hasCover = false;
    m_haveCheckedForCover = false;

    CoverManager::setIdForTrack(m_file.absFilePath(), CoverManager::NoMatch);
    CoverManager::removeCover(m_coverKey);
    m_coverKey = CoverManager::NoMatch;
}

void CoverInfo::setCover(const QImage &image)
{
    m_haveCheckedForCover = false;

    if(image.isNull())
        return;

    if(m_hasCover)
        clearCover();

    QPixmap cover;
    cover.convertFromImage(image);

    if(m_coverKey != CoverManager::NoMatch)
        CoverManager::replaceCover(m_coverKey, cover);
    else {
        m_coverKey = CoverManager::addCover(cover, m_file.tag()->artist(), m_file.tag()->album());
        if(m_coverKey != CoverManager::NoMatch)
            CoverManager::setIdForTrack(m_file.absFilePath(), m_coverKey);
    }
}

QPixmap CoverInfo::pixmap(CoverSize size) const
{
    if(m_coverKey == CoverManager::NoMatch)
        return QPixmap();

    if(size == Thumbnail)
        return CoverManager::coverFromId(m_coverKey, CoverManager::Thumbnail);
    else
        return CoverManager::coverFromId(m_coverKey, CoverManager::FullSize);
}

void CoverInfo::popup() const
{
    QPixmap image = pixmap(FullSize);
    QPoint mouse  = QCursor::pos();
    QRect desktop = KApplication::desktop()->screenGeometry(mouse);
    
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

/**
 * DEPRECATED
 */
QString CoverInfo::coverLocation(CoverSize size) const
{
    QString fileName(QFile::encodeName(m_file.tag()->artist() + " - " + m_file.tag()->album()));
    QRegExp maskedFileNameChars("[ /?:\"]");

    fileName.replace(maskedFileNameChars, "_");
    fileName.append(".png");

    QString dataDir = KGlobal::dirs()->saveLocation("appdata");
    QString subDir;

    switch (size) {
    case FullSize:
        subDir = "large/";
        break;
    default:
        break;
    }
    QString fileLocation = dataDir + "covers/" + subDir + fileName.lower();

    return fileLocation;
}

// vim: set et sw=4 ts=8:
