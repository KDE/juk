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
#include <kurl.h>

#include <qpixmap.h>

#include "tag.h"

class GoogleFetcherDialog : public KDialogBase
{
    Q_OBJECT

public:
    GoogleFetcherDialog(const QString &name,
                        const QValueList<QString> &urlList,
                        uint selectedIndex,
                        const Tag *tag,
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
    QPixmap getPixmapFromURL(KURL url) const;

    QValueList<QString> m_urlList;
    QPixmap m_pixmap;
    QWidget *m_pixWidget;
    bool m_takeIt;
    bool m_newSearch;
    uint m_index;
    const Tag *m_tag;
};

#endif
