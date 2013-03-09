/***************************************************************************
    begin                : Wed May 9 2012
    copyright            : (C) 2012 by Martin Sandsmark
    email                : martin.sandsmark@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef LYRICSWIDGET_H
#define LYRICSWIDGET_H

#include <QTextBrowser>
#include "filehandle.h"

class QNetworkAccessManager;
class QNetworkReply;


class LyricsWidget : public QTextBrowser
{
    Q_OBJECT

public:
    explicit LyricsWidget(QWidget *parent);

    virtual ~LyricsWidget();

    QSize minimumSize() const { return QSize(100, 0); }

public Q_SLOTS:
    void playing(const FileHandle &file);

protected:
    virtual void hideEvent(QHideEvent*);

private Q_SLOTS:
    void receiveListReply(QNetworkReply*);
    void receiveLyricsReply(QNetworkReply*);
    void saveConfig();


private:
    QNetworkAccessManager *m_networkAccessManager;
    QString m_title;
};


#endif//LYRICSWIDGET_H
