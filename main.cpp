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
#include <KConfigBase>
#include <KConfig>
#include <KConfigGroup>
#include <KCrash>
#include <KDBusService>
#include <KNotification>
#include <KSharedConfig>
#include <KLocalizedString>

#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QApplication>

#include "juk.h"
#include <config-juk.h>

static const char description[] = I18N_NOOP("Jukebox and music manager by the KDE community");
static const char scott[]       = I18N_NOOP("Author, chief dork and keeper of the funk");
static const char michael[]     = I18N_NOOP("Assistant superhero, fixer of many things");
static const char georg[]       = I18N_NOOP("More KDE Platform 4 porting efforts");
static const char daniel[]      = I18N_NOOP("System tray docking, \"inline\" tag editing,\nbug fixes, evangelism, moral support");
static const char tim[]         = I18N_NOOP("GStreamer port");
static const char stefan[]      = I18N_NOOP("Global keybindings support");
static const char stephen[]     = I18N_NOOP("Track announcement popups");
static const char frerich[]     = I18N_NOOP("Automagic track data guessing, bugfixes");
static const char zack[]        = I18N_NOOP("More automagical things, now using MusicBrainz");
static const char adam[]        = I18N_NOOP("Co-conspirator in MusicBrainz wizardry");
static const char matthias[]    = I18N_NOOP("Friendly, neighborhood aRts guru");
static const char maks[]        = I18N_NOOP("Making JuK friendlier to people with terabytes of music");
static const char antonio[]     = I18N_NOOP("DCOP interface");
static const char allan[]       = I18N_NOOP("FLAC and MPC support");
static const char nathan[]      = I18N_NOOP("Album cover manager");
static const char pascal[]      = I18N_NOOP("Gimper of splash screen");
static const char laurent[]     = I18N_NOOP("Porting to KDE Platform 4 when no one else was around");
static const char giorgos[]     = I18N_NOOP("Badly-needed tag editor bugfixes.");
static const char sandsmark[]   = I18N_NOOP("Last.fm scrobbling support, lyrics, prepping for KDE Frameworks 5.");
static const char sho[]         = I18N_NOOP("MPRIS2 Interface implementation.");
static const char kacper[]      = I18N_NOOP("Porting to KDE Frameworks 5 when no one else was around");

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    KLocalizedString::setApplicationDomain("juk");

    KAboutData aboutData(QStringLiteral("juk"), i18n("JuK"),
                         QStringLiteral(JUK_VERSION), i18n(description), KAboutLicense::GPL,
                         i18n("Copyright © 2002–2021, Scott Wheeler, Michael Pyne, and others"),
                         QStringLiteral(""),
                         QStringLiteral("https://juk.kde.org/"));

    aboutData.addAuthor(i18n("Scott Wheeler"), i18n(scott), "wheeler@kde.org");
    aboutData.addAuthor(i18n("Michael Pyne"), i18n(michael), "mpyne@kde.org");
    aboutData.addCredit(i18n("Kacper Kasper"), i18n(kacper), "kacperkasper@gmail.com", "http://kacperkasper.pl/");
    aboutData.addCredit(i18n("Eike Hein"), i18n(sho), "hein@kde.org");
    aboutData.addCredit(i18n("Martin Sandsmark"), i18n(sandsmark), "martin.sandsmark@kde.org");
    aboutData.addCredit(i18n("Γιώργος Κυλάφας (Giorgos Kylafas)"), i18n(giorgos), "gekylafas@gmail.com");
    aboutData.addCredit(i18n("Georg Grabler"), i18n(georg), "georg@grabler.net");
    aboutData.addCredit(i18n("Laurent Montel"), i18n(laurent), "montel@kde.org");
    aboutData.addCredit(i18n("Nathan Toone"), i18n(nathan), "nathan@toonetown.com");
    aboutData.addCredit(i18n("Matthias Kretz"), i18n(matthias), "kretz@kde.org");
    aboutData.addCredit(i18n("Daniel Molkentin"), i18n(daniel), "molkentin@kde.org");
    aboutData.addCredit(i18n("Tim Jansen"), i18n(tim), "tim@tjansen.de");
    aboutData.addCredit(i18n("Stefan Asserhäll"), i18n(stefan), "stefan.asserhall@telia.com");
    aboutData.addCredit(i18n("Stephen Douglas"), i18n(stephen), "stephen_douglas@yahoo.com");
    aboutData.addCredit(i18n("Frerich Raabe"), i18n(frerich), "raabe@kde.org");
    aboutData.addCredit(i18n("Zack Rusin"), i18n(zack), "zack@kde.org");
    aboutData.addCredit(i18n("Adam Treat"), i18n(adam), "manyoso@yahoo.com");
    aboutData.addCredit(i18n("Maks Orlovich"), i18n(maks), "maksim@kde.org");
    aboutData.addCredit(i18n("Antonio Larrosa Jimenez"), i18n(antonio), "larrosa@kde.org");
    aboutData.addCredit(i18n("Allan Sandfeld Jensen"), i18n(allan), "kde@carewolf.com");
    aboutData.addCredit(i18n("Pascal Klein"), i18n(pascal), "4pascal@tpg.com.au");
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
