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

#ifndef GOOGLEFETCHERDIALOG_H
#define GOOGLEFETCHERDIALOG_H

#include <kdialogbase.h>

#include <qpixmap.h>

#include "filehandle.h"

class KURL;

class GoogleFetcherDialog : public KDialogBase
{
    Q_OBJECT

public:
    GoogleFetcherDialog(const QString &name,
                        const QStringList &urlList,
                        uint selectedIndex,
			const FileHandle &file,
                        QWidget *parent = 0);

    virtual ~GoogleFetcherDialog();

    QPixmap result() const { return m_pixmap; }
    bool takeIt() const { return m_takeIt; }
    bool newSearch() const { return m_newSearch; }

public slots:
    int exec();

protected slots:
    void slotOk();
    void slotCancel();
    void slotUser3();
    void slotUser2();
    void slotUser1();

private:
    void setLayout();

    QPixmap fetchedImage(uint index) const;
    QPixmap pixmapFromURL(const KURL &url) const;

    QStringList m_urlList;
    QPixmap m_pixmap;
    QWidget *m_pixWidget;
    bool m_takeIt;
    bool m_newSearch;
    uint m_index;
    FileHandle m_file;
};

#endif
