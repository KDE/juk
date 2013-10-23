/***************************************************************************
    begin                : Web Oct 16 2013
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

#ifndef SYNCLIST_H
#define SYNCLIST_H

#include <QLabel>

#include <KListWidget>
#include <KUrl>
#include <KVBox>

#include <Solid/Device>
#include <Solid/OpticalDisc>
#include <Solid/StorageAccess>
#include <Solid/StorageVolume>
#include <Solid/PortableMediaPlayer>

class QNetworkAccessManager;
class QNetworkReply;


class SyncList : public KVBox
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
    Solid::Device device() const;

public Q_SLOTS:
    void newDevice();
    void showFiles(QString udi);

protected:

private:
    void initializeDevice(const QString& udi);
    void togglePlayer(bool show);

private Q_SLOTS:
    void saveConfig();

private:
    Solid::Device m_device;
    QPointer<Solid::StorageAccess> m_access;
    QPointer<Solid::StorageVolume> m_volume;
    QPointer<Solid::OpticalDisc> m_disc;
    QPointer<Solid::PortableMediaPlayer> m_mtp;
};


#endif//SYNCLIST_H
