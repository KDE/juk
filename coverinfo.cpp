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

#include <qimage.h>
#include <qvbox.h>
#include <qregexp.h>

#include "coverinfo.h"
#include "tag.h"

/**
 * QVBox subclass to show a window for the track cover, and update the parent
 * CoverInfo when we close.  It may not be the 'best' way to do it, but it
 * works.
 *
 * @author Michael Pyne <michael.pyne@kdemail.net>
 */
class CoverInfo::CoverPopupWindow : public QVBox
{
public:
    CoverPopupWindow(CoverInfo &coverInfo, const QPixmap &pixmap, QWidget *parent = 0) :
        QVBox(parent, 0, WDestructiveClose), m_coverInfo(coverInfo)
    {
        QString caption = coverInfo.m_file.tag()->artist() + " - " + coverInfo.m_file.tag()->album();
        setCaption(kapp->makeStdCaption(caption));

        QWidget *widget = new QWidget(this);
        widget->setPaletteBackgroundPixmap(pixmap);
        widget->setFixedSize(pixmap.size());

        // Trim as much as possible and keep that size.

        adjustSize();
        setFixedSize(size());
    }

    ~CoverPopupWindow()
    {
        m_coverInfo.m_popupWindow = 0;
    }

private:
    CoverInfo &m_coverInfo;
};

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////


CoverInfo::CoverInfo(const FileHandle &file) :
    m_file(file), m_popupWindow(0)
{
}

QPixmap CoverInfo::coverPixmap() const
{
    QPixmap coverThumb = pixmap(Thumbnail);

    if(!coverThumb.isNull())
        return coverThumb;

    // If the file doesn't exist, try to create the thumbnail from
    // the large image

    QPixmap largeCover = largeCoverPixmap();
    if(!largeCover.isNull()) {
        QImage image(largeCover.convertToImage());
        image.smoothScale(80, 80).save(coverLocation(Thumbnail), "PNG");
    }

    return pixmap(Thumbnail);
}

bool CoverInfo::hasCover() const
{
    return (QFile(coverLocation(Thumbnail)).exists() || QFile(coverLocation(FullSize)).exists());
}

QPixmap CoverInfo::largeCoverPixmap() const
{
    return pixmap(FullSize);
}

QPixmap CoverInfo::pixmap(int size) const
{
    if(m_file.tag()->artist().isEmpty() || m_file.tag()->album().isEmpty())
        return QPixmap();

    return QPixmap(coverLocation(size));
}

QString CoverInfo::coverLocation(int size) const
{
    QString fileName(QFile::encodeName(m_file.tag()->artist() + " - " + m_file.tag()->album()));
    QRegExp maskedFileNameChars("[ /?]");

    fileName.replace(maskedFileNameChars, "_");
    fileName.append(".png");

    QString dataDir = KGlobal::dirs()->saveLocation("appdata");
    QString subDir = QString::null;
    switch (size) {
    case FullSize:
        subDir="large/";
	break;
    default:
        subDir="";
	break;
    }
    QString fileLocation = dataDir + "covers/" + subDir + fileName.lower();

    return fileLocation;
}

void CoverInfo::popupLargeCover()
{
    QPixmap largeCover = largeCoverPixmap();
    if(largeCover.isNull())
        return;

    if(!m_popupWindow)
        m_popupWindow = new CoverPopupWindow(*this, largeCover);

    m_popupWindow->show();
    KWin::activateWindow(m_popupWindow->winId());
}

// vim: set et sw=4 ts=8:
