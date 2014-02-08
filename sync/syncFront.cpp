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

#include "sync/syncFront.h"

SyncFront::SyncFront(QWidget* parent)//: PlaylistBox(player,parent,stack)
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

SyncFront::~SyncFront()
{
}

void SyncFront::setUrl(const KUrl &url){
    qDebug()<<url;
}

void SyncFront::setUdi(const QString& udi)
{
    //setDataValue("udi", udi);
}

QString SyncFront::udi() const
{
    //return dataValue("udi").toString();
    return 0;
}


/*
 * Return the selected device
 */
Solid::Device SyncFront::device() const
{
    return m_device;
}

/// Public Q_SLOTS
void SyncFront::setupSelected(){
    //TODO: get this udi
    QString udi;
    m_selected = new SyncEngine(this,udi);
}



// PRIVATE
/*
 * Find out all the devices available on the system
 */
void SyncFront::listDevices(){
    //Solid::DeviceNotifier *notifierObj= Solid::DeviceNotifier::instance();

    //Full Device List
    foreach(Solid::Device device, Solid::Device::allDevices()){
        m_access = device.as<Solid::StorageAccess>();
        m_volume = device.as<Solid::StorageVolume>();
        m_disc = device.as<Solid::OpticalDisc>();
        m_mtp = device.as<Solid::PortableMediaPlayer>();

        if (m_access) {
            setUrl(m_access->filePath());
            kDebug() << "FilePath: " << m_access->filePath();
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
/*
    //Portable Media Player List
    foreach(Solid::Device device, Solid::Device::listFromType(Solid::DeviceInterface::StorageDrive,QString())){
        qDebug() << device.udi()<< "Product" << device.product() << "Vendor" << device.vendor() << device.description();
        Solid::Block *blk = device.as<Solid::Block>();
        if(blk)
            kDebug() << "Blk Device: " << blk->device();
    }
    //Portable Media Player List
    foreach(Solid::Device device, Solid::Device::listFromType(Solid::DeviceInterface::PortableMediaPlayer,QString())){
        qDebug() << device.udi()<< "Product" << device.product() << "Vendor" << device.vendor() << device.description();
        Solid::PortableMediaPlayer *player = device.as<Solid::PortableMediaPlayer>();
        if(player)
            qDebug() << "PMediaP Supported Protocols: " << player->supportedProtocols();
    }
*/
    //temporary    QList<Solid::Device> list = Solid::Device::listFromType(Solid::DeviceInterface::PortableMediaPlayer,QString());

    //m_selected = new SyncEngine(this,list.first().udi());
}

/*
 * Show the available devices in UI
 */
void SyncFront::placeDevices(){
    //
}

void SyncFront::initializeDevice(const QString& udi)
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
