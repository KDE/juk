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
#include <dcopclient.h>

#include "juk.h"

static const char description[] = I18N_NOOP("Jukebox and music manager for KDE");
static const char scott[]       = I18N_NOOP("Author, chief dork and keeper of the funk");
static const char daniel[]      = I18N_NOOP("System tray docking, \"inline\" tag editing,\nbug fixes, evangelism, moral support");
static const char tim[]         = I18N_NOOP("GStreamer port");
static const char stefan[]      = I18N_NOOP("Global keybindings support");
static const char stephen[]     = I18N_NOOP("Track announcement popups");
static const char frerich[]     = I18N_NOOP("Automagic track data guessing, bugfixes");
static const char zack[]        = I18N_NOOP("More automagical things, now using MusicBrainz");
static const char adam[]        = I18N_NOOP("Co-conspirator in MusicBrainz wizardry");
static const char matthias[]    = I18N_NOOP("Friendly, neighborhood aRts guru");
static const char maks[]        = I18N_NOOP("Making JuK friendlier to people with\nterabytes of music");
static const char antonio[]     = I18N_NOOP("DCOP interface");

static KCmdLineOptions options[] =
{
#ifndef NO_DEBUG
    { "norestore", I18N_NOOP("Restore playlists.  Use --norestore for debugging."), 0 },
#endif
    { "+[file(s)]", I18N_NOOP("File(s) to open"), 0 },
    KCmdLineLastOption
};

int main(int argc, char *argv[])
{
    KAboutData aboutData("juk", I18N_NOOP("JuK"),
                         "2.0", description, KAboutData::License_GPL,
                         "(c) 2002, 2003, Scott Wheeler", 0,
			 "http://developer.kde.org/~wheeler/juk.html");

    aboutData.addAuthor("Scott Wheeler", scott, "wheeler@kde.org");
    aboutData.addCredit("Daniel Molkentin", daniel, "molkentin@kde.org");
    aboutData.addCredit("Tim Jansen", tim, "tim@tjansen.de");
    aboutData.addCredit("Stefan Asserhäll", stefan, "stefan.asserhall@telia.com");
    aboutData.addCredit("Stephen Douglas", stephen, "stephen_douglas@yahoo.com");
    aboutData.addCredit("Frerich Raabe", frerich, "raabe@kde.org");
    aboutData.addCredit("Zack Rusin", zack, "zack@kde.org");
    aboutData.addCredit("Adam Treat", adam, "manyoso@yahoo.com");
    aboutData.addCredit("Matthias Kretz", matthias, "kretz@kde.org");
    aboutData.addCredit("Maks Orlovich", maks, "maksim@kde.org");
    aboutData.addCredit("Antonio Larrosa Jimenez", antonio, "larrosa@kde.org");

    KCmdLineArgs::init(argc, argv, &aboutData);
    KCmdLineArgs::addCmdLineOptions(options);

    KUniqueApplication::addCmdLineOptions();

    KUniqueApplication a;

    // Here we do some DCOP locking of sorts to prevent incoming DCOP calls
    // before JuK has finished its initialization.
    
    a.dcopClient()->suspend();
    JuK *juk = new JuK();
    a.dcopClient()->resume();

    a.setMainWidget(juk);

    bool startDocked;

    KConfig *config = KGlobal::config();
    {
        KConfigGroupSaver saver(config, "Settings");
	startDocked = config->readBoolEntry("StartDocked", false);
    }
    if(!startDocked)
	juk->show();

    return a.exec();
}
