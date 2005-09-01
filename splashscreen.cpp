/***************************************************************************
    begin                : Sun Dec 8 2002
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

#include <kapplication.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kdebug.h>

#include <qpainter.h>

#include "splashscreen.h"

SplashScreen *SplashScreen::splash = 0;
bool SplashScreen::done = false;
int SplashScreen::count = 0;

static QString loadedText(int i)
{
    static QString loading = i18n("Loading").upper();
    return loading + ": " + QString::number(i);;
}

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
        if(( count & 63 ) == 0)
            splash->processEvents();
    }
}

void SplashScreen::update()
{
    if(splash)
        splash->processEvents();
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

SplashScreen::SplashScreen() : QLabel(0 , "splashScreen", Qt::WStyle_Splash)
{
    QPixmap background = UserIcon("splash");
    resize(background.size());
    setPaletteBackgroundPixmap(background);

    setMargin(7);
    setAlignment(AlignLeft | AlignBottom);

    setPaletteForegroundColor(QColor(107, 158, 194));

    QFont f = font();
    f.setPixelSize(10);
    setFont(f);

    setText(loadedText(0));
}

SplashScreen::~SplashScreen()
{

}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

void SplashScreen::processEvents()
{
    setText(loadedText(count));
    kapp->processEvents();
}
