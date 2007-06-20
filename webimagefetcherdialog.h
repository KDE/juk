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

#ifndef WEBIMAGEFETCHERDIALOG_H
#define WEBIMAGEFETCHERDIALOG_H

#include <kiconview.h>
#include <kio/job.h>

#include "webimagefetcher.h"

class KURL;

class WebImageFetcherDialog : public KDialogBase
{
    Q_OBJECT

public:
    WebImageFetcherDialog(const WebImageList &urlList,
                        const FileHandle &file,
                        QWidget *parent = 0);

    virtual ~WebImageFetcherDialog();

    QPixmap result() const { return m_pixmap; }

    void setLayout();
    void setImageList(const WebImageList &urlList);
    void setFile(const FileHandle &file);

signals:
    void coverSelected();
    void newSearchRequested();

public slots:
    int exec();
    void refreshScreen(WebImageList &list);

protected slots:
    void slotOk();
    void slotCancel();
    void slotUser1();
    void showCreditURL(const QString &url);

private:
    QPixmap fetchedImage(uint index) const;
    QPixmap pixmapFromURL(const KURL &url) const;

    QPixmap m_pixmap;
    WebImageList m_imageList;
    KIconView *m_iconWidget;
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
    CoverIconViewItem(QIconView *parent, const WebImage &image);
    ~CoverIconViewItem();

private slots:
    void imageData(KIO::Job *job, const QByteArray &data);
    void imageResult(KIO::Job* job);

private:
    QByteArray m_buffer;
    QGuardedPtr<KIO::TransferJob> m_job;
};

#endif
