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

#include "filehandle.h"

class KURL;

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
    QStringList m_urlList;
    bool m_chosen;
    uint m_selectedIndex;
    QPixmap m_currentPixmap;
};
#endif
