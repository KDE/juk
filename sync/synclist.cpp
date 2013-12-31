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

#include <KDebug>

#include <Solid/Block>
#include <Solid/Device>
#include <Solid/OpticalDisc>
#include <Solid/StorageAccess>
#include <Solid/StorageVolume>
#include <Solid/PortableMediaPlayer>
#include <Solid/DeviceNotifier>

#include "sync/synclist.h"
#include "sync/syncplayer.h"
#include "actioncollection.h"
#include "filehandle.h"
#include "playlist.h"
#include "playlistbox.h"
#include "playermanager.h"
#include "playlistitem.h"
#include "playlistinterface.h"
#include <QStackedWidget>

SyncList::SyncList(QWidget* parent)//: PlaylistBox(player,parent,stack)
{
    listDevices();
/*
    QStackedWidget *stack = new QStackedWidget();
    PlayerManager *playerManager = new PlayerManager();
    PlaylistCollection *collection = new PlaylistCollection(playerManager,stack);
    Playlist *playlist = new Playlist(collection,"YOYOYO");
    PlaylistBox *player = new PlaylistBox(playerManager,this,stack);
    //PlaylistBox::Item *itemParent = PlaylistBox::Item::collectionItem();
    PlaylistBox::Item *item;
    item = new PlaylistBox::Item(player,"media-optical-audio", i18n("Chaudhary"));
*/

}

SyncList::~SyncList()
{
}

void SyncList::setUrl(const KUrl &url){
    qDebug()<<url;
}

void SyncList::setUdi(const QString& udi)
{
    //setDataValue("udi", udi);
}

QString SyncList::udi() const
{
    //return dataValue("udi").toString();
    return 0;
}


/*
 * Return the selected device
 */
Solid::Device SyncList::device() const
{
    return m_device;
}

/*
 * Find out all the devices available on the system
 */
void SyncList::listDevices(){
    //Solid::DeviceNotifier *notifierObj= Solid::DeviceNotifier::instance();

    //Full Device List
    //foreach(Solid::Device device, Solid::Device::allDevices()){

    //Portable Media Player List
    foreach(Solid::Device device, Solid::Device::listFromType(Solid::DeviceInterface::StorageDrive,QString())){
        qDebug() << device.udi()<< "Product" << device.product() << "Vendor" << device.vendor() << device.description();
        Solid::Block *blk = device.as<Solid::Block>();
        qDebug() << "Blk Device: " << blk->device();
    }
    //Portable Media Player List
    foreach(Solid::Device device, Solid::Device::listFromType(Solid::DeviceInterface::PortableMediaPlayer,QString())){
        qDebug() << device.udi()<< "Product" << device.product() << "Vendor" << device.vendor() << device.description();
        Solid::PortableMediaPlayer *player = device.as<Solid::PortableMediaPlayer>();
        qDebug() << "PMediaP Supported Protocols: " << player->supportedProtocols();
    }

    //temporary
    QList<Solid::Device> list = Solid::Device::listFromType(Solid::DeviceInterface::PortableMediaPlayer,QString());

    m_player = new SyncPlayer(this,list.first().udi());
}

/*
 * Show the available devices in UI
 */
void SyncList::placeDevices(){
    //
}

void SyncList::initializeDevice(const QString& udi)
{
    m_device = Solid::Device(udi);
    if (!m_device.isValid()) {
        return;
    }

    m_access = m_device.as<Solid::StorageAccess>();
    m_volume = m_device.as<Solid::StorageVolume>();
    m_disc = m_device.as<Solid::OpticalDisc>();
    m_mtp = m_device.as<Solid::PortableMediaPlayer>();

    //setText(m_device.description());
    //setIcon(m_device.icon());
    //setIconOverlays(m_device.emblems());
    //setUdi(udi);

    if (m_access) {
        setUrl(m_access->filePath());
        //QObject::connect(m_access, SIGNAL(accessibilityChanged(bool,QString)),
        //                 m_signalHandler, SLOT(onAccessibilityChanged()));
    } else if (m_disc) {
        Solid::Block *block = m_device.as<Solid::Block>();
        if (block) {
            const QString device = block->device();
            setUrl(QString("audiocd:/?device=%1").arg(device));
        } else {
            setUrl(QString("audiocd:/"));
        }
    } else if (m_mtp) {
        setUrl(QString("mtp:udi=%1").arg(m_device.udi()));
    }
}

// vim: set et sw=4 tw=0 sta:
