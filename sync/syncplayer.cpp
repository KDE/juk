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
    //setMinimumWidth(200);
/*
    KToggleAction *showPlayers = new KToggleAction(KIcon(QLatin1String("view-media-players")),
                                            i18n("Show &Players"), this);
    ActionCollection::actions()->addAction("showPlayers", showPlayers);
    connect(showPlayers, SIGNAL(toggled(bool)), this, SLOT(setShown(bool)));

    KConfigGroup config(KGlobal::config(), "showPlayers");
    bool shown = config.readEntry("showPlayers", true);
    showPlayers->setChecked(shown);
    setVisible(shown);
*/

    QFrame *deviceFrame = new QFrame(this);
    deviceFrame->setFrameStyle(Box | Sunken);
    deviceFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);


    // Make sure that we have enough of a margin to suffice for the borders,
    // hence the "lineWidth() * 2"
    QVBoxLayout *deviceLayout = new QVBoxLayout( deviceFrame );
    deviceLayout->setMargin( deviceFrame->lineWidth() * 2 );
    deviceLayout->setSpacing( 5 );
    deviceLayout->setObjectName( QLatin1String( "deviceFrame" ));
    deviceLayout->addSpacing(5);

    m_deviceLabel = new KSqueezedTextLabel(deviceFrame);
    deviceLayout->addWidget(m_deviceLabel);
    m_deviceLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_deviceLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_deviceLabel->setTextFormat(Qt::PlainText);

    deviceLayout->addSpacing(5);

    KVBox *deviceBox = new KVBox(this);
    deviceBox->setFrameStyle(Box | Sunken);
    deviceBox->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);

    QPushButton *deviceButton = new QPushButton(deviceBox);
    deviceButton->setIcon(SmallIcon("go-home"));
    deviceButton->setFlat(true);

    deviceButton->setToolTip( i18n("Call copyPlayingToTmp"));
    connect(deviceButton, SIGNAL(clicked()), this, SLOT(callCopy()));


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
