/***************************************************************************
                          main.cpp  -  description
                             -------------------
    begin                : Mon Feb  4 23:40:41 EST 2002
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

#include <kuniqueapplication.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>

#include "juk.h"

static const char *description = I18N_NOOP("Jukebox and music manager for KDE");
static const char *scott = I18N_NOOP("Author, cheif dork and keeper of the funk");
static const char *daniel = I18N_NOOP("System tray docking, \"inline\" tag editing,\nbug fixes, evangelism, moral support");
static const char *tim = I18N_NOOP("GStreamer port");

static KCmdLineOptions options[] =
{
    { "+[file(s)]", I18N_NOOP("File(s) to open"), 0 },
    { 0, 0, 0 }
};

int main(int argc, char *argv[])
{
    KAboutData aboutData("juk", I18N_NOOP("JuK"),
                         VERSION, description, KAboutData::License_GPL,
                         "(c) 2002, 2003, Scott Wheeler", 0, "http://www.slackorama.net/oss/juk/");

    aboutData.addAuthor("Scott Wheeler", scott, "wheeler@kde.org");
    aboutData.addCredit("Daniel Molkentin", daniel, "molkentin@kde.org");
    aboutData.addCredit("Tim Jansen", tim, "tim@tjansen.de");

    KCmdLineArgs::init(argc, argv, &aboutData);
    KCmdLineArgs::addCmdLineOptions(options);

    KUniqueApplication a;
    JuK *juk = new JuK();
    a.setMainWidget(juk);
    juk->show();

    return a.exec();
}
