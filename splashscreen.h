/***************************************************************************
                          splashscreen.h  -  description
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

#ifndef SPLASHSCREEN_H
#define SPLASHSCREEN_H

#include <qhbox.h>

class QLabel;

/**
 * Well, all of this session restoration sure is fun, but it's starting to take
 * a while, especially say, if you're building KDE and indexing your file system
 * in the background.  ;-)  So, despite my general hate of splashscreens I 
 * thought on appropriate here.
 *
 * As in other places, this is a singleton.  That makes it relatively easy to 
 * handle the updates from whichever class seems appropriate through static
 * methods.
 */

class SplashScreen : public QHBox 
{
public: 
    static SplashScreen *instance();
    static void finishedLoading();
    static void increment();

protected:
    SplashScreen();
    virtual ~SplashScreen();

private:
    static SplashScreen *splash;
    static bool done;
    static int count;

    QLabel *countLabel;
};

#endif
