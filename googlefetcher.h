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

#include <kdialogbase.h>

#include <qpixmap.h>
#include <qstringlist.h>
#include <qregexp.h>

#include "filehandle.h"

class KURL;

class GoogleImage
{
public:
    GoogleImage(QString thumbURL = QString::null, QString size = QString::null);

    QString imageURL() const { return m_imageURL; }
    QString thumbURL() const { return m_thumbURL; }
    QString size() const { return m_size; }

private:
    QString m_imageURL;
    QString m_thumbURL;
    QString m_size;
};

typedef QValueList<GoogleImage> GoogleImageList;

class GoogleFetcher : public QObject
{
    Q_OBJECT

public:
    enum ImageSize { All, Icon, Small, Medium, Large, XLarge };
    
    GoogleFetcher(const FileHandle &file);
    QPixmap pixmap();

private:
    void cancel();
    void editSearch();
    void saveCover();
    void previous();
    void next();
    void displayWaitMessage();
    void buildBox();

private slots:
    void slotLoadImageURLs(GoogleFetcher::ImageSize size = All);

signals:
    void signalNewSearch(GoogleImageList &images);

private:
    FileHandle m_file;
    QString m_searchString;
    QString m_loadedQuery;
    ImageSize m_loadedSize;
    GoogleImageList m_imageList;
    uint m_selectedIndex;
};
#endif
