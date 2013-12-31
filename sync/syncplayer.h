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

#ifndef SYNCPLAYER_H
#define SYNCPLAYER_H

#include <KListWidget>

#include <KAction>
#include <KActionCollection>
#include <KConfigGroup>
#include <KDebug>
#include <KFileDialog>
#include <KIO/Job>
#include <KLocalizedString>
#include <KToggleAction>
#include <KSqueezedTextLabel>

#include <kpushbutton.h>
#include <kiconloader.h>

#include <QLabel>
#include <QFrame>
#include <QVBoxLayout>

#include "actioncollection.h"
#include "filehandle.h"
#include "playlist.h"
#include "playlistitem.h"
#include "playlistinterface.h"

class QNetworkAccessManager;
class QNetworkReply;

/*
 * Perform operation on the selected player
 *
 */
class SyncPlayer: public QWidget
{
    Q_OBJECT

public:
    explicit SyncPlayer (QWidget *parent, QString udi);

    virtual ~SyncPlayer();

    QSize minimumSize() const { return QSize(100, 0); }
    KUrl* getSrc();
    KUrl* getDest();
    bool checkDestSize(KUrl dest);
    int copyToDevice(QString udi);
    void sync_in(PlaylistItem);
    bool copy_in(KUrl,KUrl);
    bool copy_in(KUrl::List, KUrl);

public Q_SLOTS:
    void sync_in_stub();

protected:

private:
    //QString m_currentplayername;  //get player name from syncList
    QString m_player_udi;
};


#endif//SYNCPLAYER_H
