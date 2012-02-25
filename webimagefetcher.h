/***************************************************************************
    copyright            : (C) 2004 Nathan Toone
    email                : nathan@toonetown.com
    copyright            : (C) 2007 Michael Pyne
    email                : michael.pyne@kdemail.net
    copyright            : (C) 2012 Martin Sandsmark
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

#ifndef WEBIMAGEFETCHER_H
#define WEBIMAGEFETCHER_H

#include <QObject>

// Predeclare some classes.


template<class T>
class QList;

class KJob;
class KUrl;

class FileHandle;


class WebImageFetcher : public QObject
{
    Q_OBJECT

public:
    WebImageFetcher(QObject *parent);
    ~WebImageFetcher();

    void setFile(const FileHandle &file);

public slots:
    void abortSearch();
    void searchCover();


signals:
    void signalCoverChanged(int coverId);

private slots:
    void slotWebRequestFinished(KJob *job);
    void slotImageFetched(KJob *job);
    void slotCoverChosen();

private:
    class Private;
    Private *d;
};
#endif

// vim: set et sw=4 tw=0 sta:
