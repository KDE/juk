/***************************************************************************
    begin                : Wed Oct 16 2013
    copyright            : (C) 2013 by Shubham Chaudhary
    email                : shubhamchaudhary92@gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <KListWidget>

#include <KAction>
#include <KLocalizedString>
#include <KActionCollection>
#include <KToggleAction>
#include <KConfigGroup>
#include <KDebug>

#include "syncplayer.h"
#include "actioncollection.h"

SyncPlayer::SyncPlayer(QWidget* parent): KListWidget(parent)
{
    setMinimumWidth(200);

    KToggleAction *show = new KToggleAction(KIcon(QLatin1String("view-media-players")),
                                            i18n("Show &Players"), this);
    ActionCollection::actions()->addAction("showPlayers", show);
    connect(show, SIGNAL(toggled(bool)), this, SLOT(setShown(bool)));

    KConfigGroup config(KGlobal::config(), "SyncPlayer");
    bool shown = config.readEntry("Show", true);
    show->setChecked(shown);
    setVisible(shown);
}

SyncPlayer::~SyncPlayer()
{
    saveConfig();
}

void SyncPlayer::saveConfig()
{
    KConfigGroup config(KGlobal::config(), "SyncPlayer");
    config.writeEntry("Show", ActionCollection::action<KToggleAction>("showPlayers")->isChecked());
}

