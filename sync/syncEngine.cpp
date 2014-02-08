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

#include "sync/syncEngine.h"

SyncEngine::SyncEngine(QWidget* parent, QString udi)
{
    m_player_udi = udi;
}

SyncEngine::~SyncEngine()
{
}

void SyncEngine::sync_in_stub(){
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

bool feasibleIn(KUrl src, KUrl dest){
    qlonglong srcSize, destFree;
    //TODO: get values
    return (destFree-srcSize);
    //TODO: Check device permissions
}

bool copy_in(Solid::Device device){
    KUrl src,dest;
    //TODO: get location
    device.udi();
    feasibleIn(src,dest);
    //TODO: Are you Sure? message

    //Start Job //copy_in_job();
    KIO::FileCopyJob *theJob = KIO::file_copy(src,dest);
    theJob->start();

    //TODO: Add the newly added songs to a folder playlist

    return theJob->error();     //theJob->percent();
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
