/***************************************************************************
    copyright            : (C) 2004 Nathan Toone
    email                : nathan@toonetown.com
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef GOOGLEFETCHER_H
#define GOOGLEFETCHER_H

#include <QObject>
#include <QString>

// Predeclare some classes.

class QPixmap;

template<class T>
class QList;

namespace DOM {
    class HTMLDocument;
}

class FileHandle;

class GoogleImage
{
public:
    explicit GoogleImage(QString thumbURL = QString(), QString size = QString());

    QString imageURL() const { return m_imageURL; }
    QString thumbURL() const { return m_thumbURL; }
    QString size() const { return m_size; }

private:
    QString m_imageURL;
    QString m_thumbURL;
    QString m_size;
};

typedef QList<GoogleImage> GoogleImageList;

class GoogleFetcher : public QObject
{
    Q_OBJECT

public:
    enum ImageSize { All, Icon, Small, Medium, Large, XLarge };

    GoogleFetcher(const FileHandle &file);
    QPixmap pixmap();

signals:
    void signalNewSearch(GoogleImageList &images);

private:
    void displayWaitMessage();
    bool requestNewSearchTerms(bool noResults = false);

    // Returns true if there are results in the search, otherwise returns false.
    bool hasImageResults(DOM::HTMLDocument &search);

private slots:
    void slotLoadImageURLs(GoogleFetcher::ImageSize size = All);

private:
    class Private;
    Private *d;
};
#endif

// vim: set et sw=4 tw=0 sta:
