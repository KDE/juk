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

#ifndef GOOGLEFETCHERDIALOG_H
#define GOOGLEFETCHERDIALOG_H

#include <kdialog.h>
#include <k3iconview.h>

#include "googlefetcher.h"
#include "filehandle.h"

#include <QPixmap>
#include <QPointer>

class KUrl;

class GoogleFetcherDialog : public KDialog
{
    Q_OBJECT

public:
    GoogleFetcherDialog(const GoogleImageList &urlList,
                        const FileHandle &file,
                        QWidget *parent = 0);

    virtual ~GoogleFetcherDialog();

    QPixmap result() const { return m_pixmap; }

    void setLayout();
    void setImageList(const GoogleImageList &urlList);
    void setFile(const FileHandle &file);

signals:
    void coverSelected();
    void newSearchRequested();

public slots:
    int exec();
    void refreshScreen(GoogleImageList &list);

protected slots:
    void slotOk();
    void slotCancel();
    void showCreditURL(const QString &url);

private:
    QPixmap fetchedImage(int index) const;
    QPixmap pixmapFromURL(const KUrl &url) const;

    QPixmap m_pixmap;
    GoogleImageList m_imageList;
    K3IconView *m_iconWidget;
    bool m_takeIt;
    bool m_newSearch;
    FileHandle m_file;
};

namespace KIO
{
    class TransferJob;
    class Job;
}

class CoverIconViewItem : public QObject, public K3IconViewItem
{
    Q_OBJECT

public:
    CoverIconViewItem(Q3IconView *parent, const GoogleImage &image);
    ~CoverIconViewItem();

private slots:
    void imageData(KIO::Job *job, const QByteArray &data);
    void imageResult(KIO::Job *job);

private:
    QByteArray m_buffer;
    QPointer<KIO::TransferJob> m_job;
};

#endif

// vim: set et sw=4 tw=0 sta:
