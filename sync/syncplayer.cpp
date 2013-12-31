/***************************************************************************
    begin                : Wed Oct 16 2013
    copyright            : (C) 2013 by Shubham Chaudhary
    email                : shubham.chaudhary@kdemail.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "sync/syncplayer.h"

SyncPlayer::SyncPlayer(QWidget* parent, QString udi)
{
    m_player_udi = udi;
}

SyncPlayer::~SyncPlayer()
{
}

void SyncPlayer::sync_in_stub(){
    //PlaylistItem *playingItem = Playlist::playingItem();
    //FileHandle playingFile = playingItem->file();
    //kDebug()<<"File path: "<<playingFile.absFilePath();
    KUrl::List srcList = KFileDialog::getOpenUrls();
    KUrl dest = KUrl("file:///tmp/myJuK");

    qDebug()<<"Calling through Stub";
}

/*
 * Sync Into the computer from device
 */
bool sync_in(PlaylistItem *items){

    //    foreach(PlaylistItem *item, items) {
    //        urls << KUrl::fromPath(item->file().absFilePath());
    //    }
    return true;
}

bool copy_in(KUrl src, KUrl dest){
    //qDebug() << "src= " << src << "\ndest=" << dest;
    KIO::FileCopyJob *theJob = KIO::file_copy(src,dest);
    theJob->start();
//    foreach(PlaylistItem *item, items) {
//        urls << KUrl::fromPath(item->file().absFilePath());
//    }
    return true;
}

bool copy_in(KUrl::List srcList, KUrl dest){
    //qDebug() << "src= " << src << "\ndest=" << dest;
    foreach (KUrl src, srcList) {
        qDebug()<<"Copying: "<<src;
        KIO::FileCopyJob *theJob = KIO::file_copy(src,dest);
        theJob->start();
    }
//    foreach(PlaylistItem *item, items) {
//        urls << KUrl::fromPath(item->file().absFilePath());
//    }
    return true;
}

// vim: set et sw=4 tw=0 sta:
