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

#include <kdialog.h>

#include "webimagefetcher.h"
#include "filehandle.h"

#include <QPixmap>
#include <QStandardItem>
#include <QByteArray>
#include <QPointer>

class KUrl;

class QListView;

class WebImageFetcherDialog : public KDialog
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
    void slotActivated(const QModelIndex &);
    void slotOk();
    void slotCancel();
    void showCreditURL(const QString &url);
    void selectedItemIsBad();

private:
    QPixmap fetchedImage(int index) const;
    QPixmap pixmapFromURL(const KUrl &url) const;

    QPixmap m_pixmap;
    WebImageList m_imageList;
    QListView *m_iconWidget;
    bool m_takeIt;
    bool m_newSearch;
    FileHandle m_file;
};

namespace KIO
{
    class StoredTransferJob;
    class Job;
}

class KJob;

class CoverIconViewItem : public QObject, public QStandardItem
{
    Q_OBJECT

public:
    CoverIconViewItem(QWidget *parent, const WebImage &image);
    ~CoverIconViewItem();

private slots:
    void imageResult(KJob *job);

private:
    QByteArray m_buffer;
    QPointer<KIO::StoredTransferJob> m_job;
};

#endif

// vim: set et sw=4 tw=0 sta:
