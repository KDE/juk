/***************************************************************************
    begin                : Sun Feb 17 2002
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

#include <kglobal.h>
#include <kapplication.h>
#include <kstandarddirs.h>
#include <kdebug.h>
#include <kwin.h>

#include <qimage.h>
#include <qvbox.h>
#include <qregexp.h>
#include <qwidget.h>
#include <qnamespace.h>

#include "coverinfo.h"

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
        QString caption = coverInfo.m_tag.artist() + " - " + coverInfo.m_tag.album();
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


CoverInfo::CoverInfo(const Tag &tag) :
    m_tag(tag), m_popupWindow(0)
{
}

QPixmap *CoverInfo::coverPixmap() const
{
    QPixmap *coverThumb = pixmap(false);

    if(!coverThumb->isNull())
        return coverThumb;

    // If the file doesn't exist, try to create the thumbnail from
    // the large image

    QPixmap *largeCover = largeCoverPixmap();
    if(!largeCover->isNull()) {
        QImage img(largeCover->convertToImage());
        img.smoothScale(80, 80).save(coverLocation(false), "PNG");
    }

    return pixmap(false);
}

bool CoverInfo::hasCover() const
{
    return (QFile(coverLocation(false)).exists() || QFile(coverLocation(true)).exists());
}

QPixmap *CoverInfo::largeCoverPixmap() const
{
    return pixmap(true);
}

QPixmap *CoverInfo::pixmap(bool large) const
{
    if(m_tag.artist().isEmpty() || m_tag.album().isEmpty())
        return new QPixmap();

    return new QPixmap(coverLocation(large));
}

QString CoverInfo::coverLocation(bool large) const
{
    QString fileName(QFile::encodeName(m_tag.artist() + " - " + m_tag.album()));
    QRegExp maskedFileNameChars("[ /?]");

    fileName.replace(maskedFileNameChars, "_");
    fileName.append(".png");

    QString dataDir = KGlobal::dirs()->saveLocation("appdata");
    QString fileLocation = dataDir + "covers/" + (large ? "large/" : QString::null) + fileName.lower();

    return fileLocation;
}

void CoverInfo::popupLargeCover()
{
    QPixmap *largeCover = largeCoverPixmap();
    if(largeCover->isNull())
        return;

    if(!m_popupWindow)
        m_popupWindow = new CoverPopupWindow(*this, *largeCover);

    m_popupWindow->show();
    KWin::activateWindow(m_popupWindow->winId());
}

// vim: set et sw=4 ts=8:
