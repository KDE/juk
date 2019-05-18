/**
 * Copyright (C) 2012 Martin Sandsmark <martin.sandsmark@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
    virtual void showEvent(QShowEvent*) override;

private:
    void makeLyricsRequest();

private Q_SLOTS:
    void receiveListReply(QNetworkReply*);
    void receiveLyricsReply(QNetworkReply*);
    void saveConfig();

private:
    FileHandle m_playingFile;
    QNetworkAccessManager *m_networkAccessManager;
    QString m_title;
    bool m_lyricsCurrent;
};

#endif//LYRICSWIDGET_H
