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

#include <qpixmap.h>
#include <qstringlist.h>
#include <qregexp.h>

#include "filehandle.h"

class KURL;

class GoogleImage
{
public:
    GoogleImage(const QString &thumbURL = QString::null,
                const QString &size = QString::null);

    QString imageURL() const { return m_imageURL; }
    QString thumbURL() const { return m_thumbURL; }
    QString size() const { return m_size; }

private:
    QString m_imageURL;
    QString m_thumbURL;
    QString m_size;
};

class GoogleFetcher
{
public:
    GoogleFetcher(const FileHandle &file);
    QPixmap pixmap();

private:
    void cancel();
    void editSearch();
    void saveCover();
    void previous();
    void next();
    void loadImageURLs();
    void displayWaitMessage();
    void buildBox();

    FileHandle m_file;
    QString m_searchString;
    QString m_loadedQuery;
    QValueList<GoogleImage> m_imageList;
    bool m_chosen;
    uint m_selectedIndex;
    QPixmap m_currentPixmap;
};
#endif
