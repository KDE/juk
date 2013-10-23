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
#include <KActionCollection>
#include <KConfigGroup>
#include <KDebug>
#include <KLocalizedString>
#include <KToggleAction>
#include <KSqueezedTextLabel>

#include <kpushbutton.h>
#include <kiconloader.h>

#include <QLabel>
#include <QFrame>
#include <QVBoxLayout>

#include "syncplayer.h"
#include "actioncollection.h"
#include "filehandle.h"
#include "playlist.h"
#include "playlistitem.h"
#include "playlistinterface.h"

SyncPlayer::SyncPlayer(QWidget* parent): KVBox(parent)
{


}

SyncPlayer::~SyncPlayer()
{
    saveConfig();
}

void SyncPlayer::saveConfig()
{
    KConfigGroup config(KGlobal::config(), "ShowPlayers");
    config.writeEntry("ShowPlayers", ActionCollection::action<KToggleAction>("showPlayers")->isChecked());
}

void SyncPlayer::togglePlayer(bool show)
{
    if(show)
    {
        ActionCollection::action<KToggleAction>("showPlayers")->setChecked(true);
    }
}

void SyncPlayer::copyPlayingToTmp(){
    //PlaylistItem *playingItem = Playlist::playingItem();
    //FileHandle playingFile = playingItem->file();
    //kDebug()<<"File path: "<<playingFile.absFilePath();
    qDebug()<<"Called me";
}

// vim: set et sw=4 tw=0 sta:
