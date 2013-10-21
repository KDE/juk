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

#ifndef SYNCPLAYER_H
#define SYNCPLAYER_H

#include <QLabel>

#include <KListWidget>

#include <kurl.h>
#include <KVBox>

class QNetworkAccessManager;
class QNetworkReply;


class SyncPlayer : public KVBox
{
    Q_OBJECT

public:
    explicit SyncPlayer (QWidget *parent=0);

    virtual ~SyncPlayer();

    QSize minimumSize() const { return QSize(100, 0); }
    void copyPlayingToTmp();

public Q_SLOTS:
    void callCopy(){ copyPlayingToTmp(); }

protected:


private:
    void togglePlayer(bool show);

private Q_SLOTS:
    void saveConfig();

private:
    QString m_mediaplayername;  //get player name from Solid
    QLabel *m_deviceLabel;
};


#endif//SYNCPLAYER_H
