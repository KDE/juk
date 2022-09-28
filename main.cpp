/**
 * Copyright (C) 2002-2007 Scott Wheeler <wheeler@kde.org>
 * Copyright (C) 2004-2021 Michael Pyne  <mpyne@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <KAboutData>
#include <KConfig>
#include <KConfigGroup>
#include <KCrash>
#include <KDBusService>
#include <KLocalizedString>
#include <KNotification>
#include <KSharedConfig>

#include <QApplication>
#include <QCommandLineParser>

#include "juk.h"
#include <config-juk.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    KLocalizedString::setApplicationDomain("juk");

    KAboutData aboutData(QStringLiteral("juk"), i18n("JuK"),
                         QStringLiteral(JUK_VERSION), i18n("Jukebox and music manager by the KDE community"), KAboutLicense::GPL,
                         i18n("Copyright © 2002–2021, Scott Wheeler, Michael Pyne, and others"),
                         QLatin1String(""),
                         QStringLiteral("https://juk.kde.org/"));

    aboutData.addAuthor(i18n("Scott Wheeler"), i18n("Author, chief dork and keeper of the funk"), "wheeler@kde.org");
    aboutData.addAuthor(i18n("Michael Pyne"), i18n("Assistant superhero, fixer of many things"), "mpyne@kde.org");
    aboutData.addCredit(i18n("Kacper Kasper"), i18n("Porting to KDE Frameworks 5 when no one else was around"), "kacperkasper@gmail.com", "http://kacperkasper.pl/");
    aboutData.addCredit(i18n("Eike Hein"), i18n("MPRIS2 Interface implementation."), "hein@kde.org");
    aboutData.addCredit(i18n("Martin Sandsmark"), i18n("Last.fm scrobbling support, lyrics, prepping for KDE Frameworks 5."), "martin.sandsmark@kde.org");
    aboutData.addCredit(i18n("Γιώργος Κυλάφας (Giorgos Kylafas)"), i18n("Badly-needed tag editor bugfixes."), "gekylafas@gmail.com");
    aboutData.addCredit(i18n("Georg Grabler"), i18n("More KDE Platform 4 porting efforts"), "georg@grabler.net");
    aboutData.addCredit(i18n("Laurent Montel"), i18n("Porting to KDE Platform 4 when no one else was around"), "montel@kde.org");
    aboutData.addCredit(i18n("Nathan Toone"), i18n("Album cover manager"), "nathan@toonetown.com");
    aboutData.addCredit(i18n("Matthias Kretz"), i18n("Friendly, neighborhood aRts guru"), "kretz@kde.org");
    aboutData.addCredit(i18n("Daniel Molkentin"), i18n("System tray docking, \"inline\" tag editing,\nbug fixes, evangelism, moral support"), "molkentin@kde.org");
    aboutData.addCredit(i18n("Tim Jansen"), i18n("GStreamer port"), "tim@tjansen.de");
    aboutData.addCredit(i18n("Stefan Asserhäll"), i18n("Global keybindings support"), "stefan.asserhall@telia.com");
    aboutData.addCredit(i18n("Stephen Douglas"), i18n("Track announcement popups"), "stephen_douglas@yahoo.com");
    aboutData.addCredit(i18n("Frerich Raabe"), i18n("Automagic track data guessing, bugfixes"), "raabe@kde.org");
    aboutData.addCredit(i18n("Zack Rusin"), i18n("More automagical things, now using MusicBrainz"), "zack@kde.org");
    aboutData.addCredit(i18n("Adam Treat"), i18n("Co-conspirator in MusicBrainz wizardry"), "manyoso@yahoo.com");
    aboutData.addCredit(i18n("Maks Orlovich"), i18n("Making JuK friendlier to people with terabytes of music"), "maksim@kde.org");
    aboutData.addCredit(i18n("Antonio Larrosa Jimenez"), i18n("DCOP interface"), "larrosa@kde.org");
    aboutData.addCredit(i18n("Allan Sandfeld Jensen"), i18n("FLAC and MPC support"), "kde@carewolf.com");
    aboutData.addCredit(i18n("Pascal Klein"), i18n("Gimper of splash screen"), "4pascal@tpg.com.au");
    KAboutData::setApplicationData(aboutData);

    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);
    parser.addPositionalArgument(QLatin1String("[file(s)]"), i18n("File(s) to open"));
    parser.process(a);
    aboutData.processCommandLine(&parser);

    KCrash::initialize();

    // Create the main window and such
    JuK *juk = new JuK(parser.positionalArguments());

    if(a.isSessionRestored() && KMainWindow::canBeRestored(1))
        juk->restore(1, false /* don't show */);

    KConfigGroup config(KSharedConfig::openConfig(), "Settings");
    if(!config.readEntry("StartDocked", false) || !config.readEntry("DockInSystemTray", false)) {
        juk->show();
    }
    else if(!a.isSessionRestored()) {
        QString message = i18n("JuK running in docked mode\nUse context menu in system tray to restore.");
        KNotification::event("dock_mode",i18n("JuK Docked"), message);
    }

    a.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    a.setApplicationName("juk");
    a.setOrganizationDomain("kde.org");
    // Limit to only one instance
    KDBusService service(KDBusService::Unique);

    return a.exec();
}

// vim: set et sw=4 tw=0 sta fileencoding=utf8:
