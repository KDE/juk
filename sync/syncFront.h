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

#ifndef SYNCFRONT_H
#define SYNCFRONT_H

#include <QLabel>
#include <QPointer>
#include <QStackedWidget>

#include <KListWidget>
#include <KUrl>
#include <KVBox>
#include <KDebug>

#include <Solid/Block>
#include <Solid/Device>
#include <Solid/OpticalDisc>
#include <Solid/StorageAccess>
#include <Solid/StorageVolume>
#include <Solid/PortableMediaPlayer>
#include <Solid/DeviceNotifier>

#include "sync/syncFront.h"
#include "sync/syncEngine.h"
#include "actioncollection.h"
#include "filehandle.h"
#include "playlist.h"
#include "playlistbox.h"
#include "playermanager.h"
#include "playlistitem.h"
#include "playlistinterface.h"

class QNetworkAccessManager;
class QNetworkReply;

/*
class SyncFrontItem : public PlaylistItem
{

};
*/
/*
 * Create and maintain list showing available players
 *
 */
class SyncFront : public QWidget //PlaylistBox
{
    Q_OBJECT

public:
    explicit SyncFront (QWidget *parent=0);
    virtual ~SyncFront();
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
    void callCopy(){ listDevices(); m_selected->sync_in_stub(); }
    void setupSelected(); //Setup Selected Device

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
    SyncEngine *m_selected;

    //temp
    PlayerManager *player;
    QStackedWidget *stack;
};


#endif//SYNCFRONT_H
