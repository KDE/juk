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

#include <qimage.h>
#include <qvbox.h>
#include <qwidget.h>
#include <qnamespace.h>

#include "coverinfo.h"

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////


CoverInfo::CoverInfo(const Tag &tag) :
    m_tag(tag)
{

}

QPixmap *CoverInfo::coverPixmap()
{

    QPixmap *coverThumb = getPixmap(false);

    if (!coverThumb->isNull())
        return coverThumb;

    // If the file doesn't exists, try to create the thumbnail from
    // the large image

    QPixmap *largeCover = largeCoverPixmap();
    if (!largeCover->isNull()) {
        QImage img(largeCover->convertToImage());
        img.smoothScale(80, 80).save(coverLocation(false), "PNG");
    }

    return getPixmap(false);
}

QPixmap *CoverInfo::largeCoverPixmap()
{
    return getPixmap(true);
}

QPixmap *CoverInfo::getPixmap(bool large)
{
    if (m_tag.artist().isEmpty() || m_tag.album().isEmpty())
        return new QPixmap();

    return new QPixmap(coverLocation(large));
}

QString CoverInfo::coverLocation(bool large)
{
    QString fileName (QFile::encodeName(m_tag.artist() + " - " + m_tag.album()));
    fileName.replace(" ", "_").replace("/", "_").replace("?", "_").append(".png");
    QString fileLocation = KGlobal::dirs()->saveLocation("data", kapp->instanceName() + "/") +
					"covers/" +
					(large ? "large/" : "") +
					fileName.lower();
    return fileLocation;
}

void CoverInfo::popupLargeCover()
{
    QPixmap *largeCover = largeCoverPixmap();;
    if (largeCover->isNull()){
        return;
    }
    QVBox *container = new QVBox(0, 0, Qt::WDestructiveClose);
    container->setCaption(kapp->makeStdCaption(m_tag.artist() + " - " +m_tag.album()));
    QWidget *widget = new QWidget(container);
    widget->setPaletteBackgroundPixmap(*largeCover);
    widget->setFixedSize(largeCover->size());
    container->adjustSize();
    container->setFixedSize(container->size());
    container->show();
}

