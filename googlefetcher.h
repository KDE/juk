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
//Added by qt3to4:
#include <Q3ValueList>

#include "filehandle.h"

namespace DOM {
    class HTMLDocument;
}

class KUrl;

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

typedef Q3ValueList<GoogleImage> GoogleImageList;

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
    FileHandle m_file;
    QString m_searchString;
    QString m_loadedQuery;
    ImageSize m_loadedSize;
    GoogleImageList m_imageList;
    uint m_selectedIndex;
};
#endif
