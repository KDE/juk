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
#include <kwin.h>

#include <qvbox.h>
#include <qregexp.h>
#include <qlayout.h>
#include <qlabel.h>

#include "coverinfo.h"
#include "tag.h"

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////


CoverInfo::CoverInfo(const FileHandle &file) :
    m_file(file),
    m_hasCover(false),
    m_haveCheckedForCover(false),
    m_popup(0)
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
    QFile::remove(coverLocation(CoverInfo::FullSize));
    QFile::remove(coverLocation(CoverInfo::Thumbnail));
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

    image.save(coverLocation(CoverInfo::FullSize), "PNG");
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

void CoverInfo::popupCover(int x, int y)
{
    QPixmap largeCover = pixmap(FullSize);
    if(largeCover.isNull())
        return;

    if(!m_popup)
        m_popup = new CoverPopup(largeCover);

    m_popup->popup(x, y);
}

struct CoverPopupWidget : public QWidget
{
    CoverPopupWidget(const QPixmap &image, int x, int y) : QWidget(0, 0, WX11BypassWM)
    {
        QHBoxLayout *layout = new QHBoxLayout(this);
        QLabel *label = new QLabel(this);

        layout->addWidget(label);
        label->setFrameStyle(QFrame::Box | QFrame::Raised);
        label->setLineWidth(1);
        label->setPixmap(image);

        setGeometry(x - 10, y - 10, label->width(), label->height());
        show();
    }
};

void CoverPopup::popup(int x, int y)
{
    if(m_popupWidget)
        delete m_popupWidget;

    m_popupWidget = new CoverPopupWidget(m_pixmap, x, y);

    m_popupWidget->installEventFilter(this);
}

bool CoverPopup::eventFilter(QObject *object, QEvent *event)
{
    if(object == m_popupWidget && (event->type() == QEvent::MouseButtonPress ||
                                   event->type() == QEvent::Leave))
    {
        delete m_popupWidget;
        m_popupWidget = 0;
        return true;
    }

    return QLabel::eventFilter(object, event);
}

// vim: set et sw=4 ts=8:
