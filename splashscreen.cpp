/***************************************************************************
                          splashscreen.cpp  -  description
                             -------------------
    begin                : Sun Dec 8 2002
    copyright            : (C) 2002 by Scott Wheeler
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

#include <kapplication.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kdebug.h>

#include <qlabel.h>

#include "splashscreen.h"

SplashScreen *SplashScreen::splash = 0;
bool SplashScreen::done = false;
int SplashScreen::count = 0;

////////////////////////////////////////////////////////////////////////////////
// pubic members
////////////////////////////////////////////////////////////////////////////////

SplashScreen *SplashScreen::instance()
{
    if(!splash && !done)
	splash = new SplashScreen();
    return splash;
}

void SplashScreen::finishedLoading()
{
    done = true;
    delete splash;
    splash = 0;
}

void SplashScreen::increment()
{
    if(splash) {
	count++;
	if(count % 10 == 0)
	    splash->processEvents();
    }
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

SplashScreen::SplashScreen() : QHBox(0 , "splashScreen", Qt::WStyle_Splash)
{
    setMargin(10);
    setSpacing(5);

    setLineWidth(5);
    setFrameShape(Box);
    setFrameShadow(Plain);
    
    QFont font = QWidget::font();

    if(font.pixelSize() > 0)
	font.setPixelSize(font.pixelSize() * 2);
    else
	font.setPointSize(font.pointSize() * 2);

    QLabel *iconLabel = new QLabel(this);
    iconLabel->setPixmap(DesktopIcon("juk"));

    QLabel *textLabel = new QLabel(i18n("Items loaded:"), this);
    textLabel->setFont(font);

    m_countLabel = new QLabel(this);
    m_countLabel->setText(QString::number(count));
    m_countLabel->setFont(font);
    m_countLabel->setMinimumWidth(m_countLabel->fontMetrics().width("00000"));
    
    setMaximumWidth(iconLabel->width() + textLabel->width() + m_countLabel->width() + 10);
    setMaximumHeight(QMAX(iconLabel->height(), textLabel->height()));

    QDesktopWidget *desktop = KApplication::desktop();
    QRect r = desktop->screenGeometry(desktop->primaryScreen());
    setGeometry((r.width() / 2) - (width() / 2), (r.height() / 2) - (height() / 2), width(), height());
}

SplashScreen::~SplashScreen()
{

}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

void SplashScreen::processEvents()
{
    m_countLabel->setText(QString::number(count));
    kapp->processEvents();
}
