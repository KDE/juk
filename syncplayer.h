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

#include <KListWidget>

class QNetworkAccessManager;
class QNetworkReply;


class SyncPlayer : public KListWidget
{
    Q_OBJECT

public:
    explicit SyncPlayer (QWidget *parent);

    virtual ~SyncPlayer();

    QSize minimumSize() const { return QSize(100, 0); }

public Q_SLOTS:

protected:
    virtual void showEvent(QShowEvent*);

private:

private Q_SLOTS:
    void saveConfig();

private:
};


#endif//SYNCPLAYER_H
