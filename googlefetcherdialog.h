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
#include <kiconview.h>
#include <kio/job.h>
#include <kio/jobclasses.h>

#include <qpixmap.h>

#include "filehandle.h"
#include "googlefetcher.h"

class KURL;

class GoogleFetcherDialog : public KDialogBase
{
    Q_OBJECT

public:
    GoogleFetcherDialog(const QString &name,
                        const QValueList<GoogleImage> &urlList,
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
    void slotUser1();

private:
    void setLayout();

    QPixmap m_pixmap;
    QPixmap fetchedImage(uint index) const;
    QPixmap pixmapFromURL(const KURL &url) const;

    QValueList<GoogleImage> m_imageList;
    KIconView *m_iconWidget;
    bool m_takeIt;
    bool m_newSearch;
    uint m_index;
    FileHandle m_file;
};

class CoverIconViewItem : public QObject
{
    Q_OBJECT

    public:
        CoverIconViewItem(QIconView *parent, GoogleImage image);
        ~CoverIconViewItem();

    private slots:
        void imageData(KIO::Job* job, const QByteArray& data);
        void imageResult(KIO::Job* job);

    private:
        static const uint BUFFER_SIZE = 2000000;

        KIconViewItem *m_iconViewItem;
        uchar* m_buffer;
        uint m_bufferIndex;
};

#endif
