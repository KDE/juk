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
    virtual void mousePressEvent(QMouseEvent *) { close(); }
};

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////


CoverInfo::CoverInfo(const FileHandle &file) :
    m_file(file),
    m_hasCover(false),
    m_haveCheckedForCover(false)
{

}

bool CoverInfo::hasCover()
{
    if(!m_haveCheckedForCover) {
        m_hasCover = QFile(coverLocation(FullSize)).exists();
        m_haveCheckedForCover = true;
    }

    return m_hasCover;
}

void CoverInfo::clearCover()
{
    QFile::remove(coverLocation(FullSize));
    QFile::remove(coverLocation(Thumbnail));
    m_hasCover = false;
    m_haveCheckedForCover = false;
}

void CoverInfo::setCover(const QImage &image)
{
    m_haveCheckedForCover = false;

    if(image.isNull())
        return;

    if(m_hasCover)
        clearCover();

    image.save(coverLocation(FullSize), "PNG");
}

QPixmap CoverInfo::pixmap(CoverSize size) const
{
    if(m_file.tag()->artist().isEmpty() || m_file.tag()->album().isEmpty())
        return QPixmap();

    if(size == Thumbnail && !QFile(coverLocation(Thumbnail)).exists()) {
        QPixmap large = pixmap(FullSize);
        if(!large.isNull()) {
            QImage image(large.convertToImage());
            image.smoothScale(80, 80).save(coverLocation(Thumbnail), "PNG");
        }
    }

    return QPixmap(coverLocation(size));
}

void CoverInfo::popup() const
{
    QPixmap image = pixmap(FullSize);
    QRect desktop = KApplication::desktop()->screenGeometry(kapp->mainWidget());
    QPoint mouse  = QCursor::pos();

    int x = mouse.x();
    int y = mouse.y();
    int height = image.size().height() + 4;
    int width  = image.size().width() + 4;

    // Detect the right direction to pop up (always towards the center of the
    // screen), try to pop up with the mouse pointer 10 pixels into the image in
    // both directions.  If we're too close to the screen border for this margin,
    // show it at the screen edge, accounting for the four pixels (two on each
    // side) for the window border.

    if(x < desktop.width() / 2)
        x = (x < 10) ? 0 : (x - 10);
    else
        x = (x > desktop.width() - 10) ? desktop.width() - width : (x - width + 10);

    if(y < desktop.height() / 2)
        y = (y < 10) ? 0 : (y - 10);
    else
        y = (y > desktop.height() - 10) ? desktop.height() - height : (y - height + 10);

    new CoverPopup(image, QPoint(x, y));
}

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
