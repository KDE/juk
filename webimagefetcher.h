/***************************************************************************
    copyright            : (C) 2004 Nathan Toone
    email                : nathan@toonetown.com
    copyright            : (C) 2007 Michael Pyne
    email                : michael.pyne@kdemail.net
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
#include <QString>

// Predeclare some classes.


template<class T>
class QList;

class KJob;

class FileHandle;

class WebImage
{
public:
    WebImage();

    WebImage(const QString &imageURL,
                const QString &thumbURL,
                int width, int height);

    QString imageURL() const { return m_imageURL; }
    QString thumbURL() const { return m_thumbURL; }
    QString size() const { return m_size; }

private:
    QString m_imageURL;
    QString m_thumbURL;
    QString m_size;
};

typedef QList<WebImage> WebImageList;

class WebImageFetcher : public QObject
{
    Q_OBJECT

public:
    WebImageFetcher(QObject *parent);
    ~WebImageFetcher();

    void setFile(const FileHandle &file);
    void chooseCover();

public slots:
    void abortSearch();

signals:
    void signalNewSearch(WebImageList &images);
    void signalCoverChanged(int coverId);

private:
    void displayWaitMessage();
    void requestNewSearchTerms(bool noResults = false);

private slots:
    void slotLoadImageURLs();
    void slotWebRequestFinished(KJob *job);
    void slotCoverChosen();
    void slotNewSearch();

private:
    class Private;
    Private *d;
};
#endif

// vim: set et sw=4 tw=0 sta:
