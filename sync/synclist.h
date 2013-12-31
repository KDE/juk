/***************************************************************************
    begin                : Web Oct 16 2013
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

#ifndef SYNCLIST_H
#define SYNCLIST_H

#include <QLabel>
#include <QPointer>
#include <Solid/Device>
#include <Solid/OpticalDisc>
#include <Solid/StorageAccess>
#include <Solid/StorageVolume>
#include <Solid/PortableMediaPlayer>

#include <KListWidget>
#include <KUrl>
#include <KVBox>

#include <Solid/Device>
#include <Solid/OpticalDisc>
#include <Solid/StorageAccess>
#include <Solid/StorageVolume>
#include <Solid/PortableMediaPlayer>

#include "sync/syncplayer.h"
#include "playlistbox.h"

class QNetworkAccessManager;
class QNetworkReply;

/*
class SyncListItem : public PlaylistItem
{

};
*/
/*
 * Create and maintain list showing available players
 *
 */
class SyncList : public QWidget //PlaylistBox
{
    Q_OBJECT

public:
    explicit SyncList (QWidget *parent=0);
    virtual ~SyncList();
    QSize minimumSize() const { return QSize(100, 0); }
    void displayList();
    void setUrl(const KUrl& url);
    KUrl url() const;
    void setUdi(const QString& udi);
    QString udi() const;
    Solid::Device device() const;   //return m_device

public Q_SLOTS:
    //void newDevice();
    //void showFiles(QString udi);
    void callCopy(){ listDevices(); m_player->sync_in_stub(); }

protected:

private:
    void listDevices();
    void placeDevices();
    void initializeDevice(const QString& udi);
    void togglePlayer(bool show);

private:

    Solid::Device m_device;
    QPointer<Solid::StorageAccess> m_access;
    QPointer<Solid::StorageVolume> m_volume;
    QPointer<Solid::OpticalDisc> m_disc;
    QPointer<Solid::PortableMediaPlayer> m_mtp;
    SyncPlayer *m_player;

    //temp
    PlayerManager *player;
    QStackedWidget *stack;
};


#endif//SYNCLIST_H
