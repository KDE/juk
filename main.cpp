/***************************************************************************
                          main.cpp  -  description
                             -------------------
    begin                : Mon Feb  4 23:40:41 EST 2002
    copyright            : (C) 2002 by Scott Wheeler
    email                : scott@slackorama.net
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>

#include "juk.h"

static const char *description =
I18N_NOOP("JuK is a jukebox and tagger for KDE. \n"
          "It supports id3v1 and id3v2 tags.\n"
          "It is based on QTagger 0.2, also by\n"
          "Scott Wheeler.  It makes use of id3lib\n"
          "to handle the low level aspects of tagging.");

static KCmdLineOptions options[] =
{
    { 0, 0, 0 }
    // INSERT YOUR COMMANDLINE OPTIONS HERE
};

int main(int argc, char *argv[])
{

    KAboutData aboutData("juk", I18N_NOOP("JuK"),
                         VERSION, description, KAboutData::License_GPL,
                         "(c) 2002, Scott Wheeler", 0, 0, "scott@slackorama.net");
    aboutData.addAuthor("Scott Wheeler", 0, "scott@slackorama.net");
    KCmdLineArgs::init( argc, argv, &aboutData );
    KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.

    KApplication a;
    JuK *juk = new JuK();
    a.setMainWidget(juk);
    juk->show();

    return a.exec();
}
