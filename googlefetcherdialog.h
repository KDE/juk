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

#include <kiconview.h>
#include <kio/job.h>


#include "googlefetcher.h"
//Added by qt3to4:
#include <QPixmap>

class KUrl;

class GoogleFetcherDialog : public KDialogBase
{
    Q_OBJECT

public:
    GoogleFetcherDialog(const QString &name,
                        const GoogleImageList &urlList,
                        const FileHandle &file,
                        QWidget *parent = 0);

    virtual ~GoogleFetcherDialog();

    QPixmap result() const { return m_pixmap; }
    bool takeIt() const { return m_takeIt; }
    bool newSearch() const { return m_newSearch; }

    void setLayout();
    void setImageList(const GoogleImageList &urlList);

public slots:
    int exec();
    void refreshScreen(GoogleImageList &list);

signals:
    void sizeChanged(GoogleFetcher::ImageSize);

protected slots:
    void slotOk();
    void slotCancel();
    void slotUser1();
    void imgSizeChanged(int index);

private:
    QPixmap fetchedImage(uint index) const;
    QPixmap pixmapFromURL(const KUrl &url) const;

    QPixmap m_pixmap;
    GoogleImageList m_imageList;
    KIconView *m_iconWidget;
    bool m_takeIt;
    bool m_newSearch;
    FileHandle m_file;
};

namespace KIO
{
    class TransferJob;
}

class CoverIconViewItem : public QObject, public KIconViewItem
{
    Q_OBJECT

public:
    CoverIconViewItem(Q3IconView *parent, const GoogleImage &image);
    ~CoverIconViewItem();

private slots:
    void imageData(KIO::Job *job, const QByteArray &data);
    void imageResult(KIO::Job* job);

private:
    QByteArray m_buffer;
    QPointer<KIO::TransferJob> m_job;
};

#endif
