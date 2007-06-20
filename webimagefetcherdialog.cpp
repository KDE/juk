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

#include <kapplication.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <krun.h>
#include <kcombobox.h>
#include <kiconloader.h>
#include <kurllabel.h>

#include <qvbox.h>
#include <qlayout.h>
#include <qimage.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qeventloop.h>

#include "webimagefetcherdialog.h"
#include "tag.h"

WebImageFetcherDialog::WebImageFetcherDialog(const WebImageList &imageList,
                                         const FileHandle &file,
                                         QWidget *parent) :
    KDialogBase(parent, "internet_image_fetcher", true, QString::null,
                Ok | Cancel | User1 , NoDefault, true),
    m_pixmap(QPixmap()),
    m_imageList(imageList),
    m_file(file)
{
    disableResize();

    QWidget *mainBox = new QWidget(this);
    QBoxLayout *mainLayout = new QVBoxLayout(mainBox);

    m_iconWidget = new KIconView(mainBox);
    m_iconWidget->setResizeMode(QIconView::Adjust);
    m_iconWidget->setSpacing(10);
    m_iconWidget->setFixedSize(500,550);
    m_iconWidget->arrangeItemsInGrid();
    m_iconWidget->setItemsMovable(false);
    mainLayout->addWidget(m_iconWidget);
    connect(m_iconWidget, SIGNAL(executed(QIconViewItem *)),
	    this, SLOT(slotOk()));

    // Before changing the code below be sure to check the attribution terms
    // of the Yahoo Image Search API.
    // http://developer.yahoo.com/attribution/
    KURLLabel *logoLabel = new KURLLabel(mainBox);
    logoLabel->setURL("http://developer.yahoo.com/about/");
    logoLabel->setPixmap(UserIcon("yahoo_credit"));
    logoLabel->setMargin(15);    // Allow large margin per attribution terms.
    logoLabel->setUseTips(true); // Show URL in tooltip.
    connect(logoLabel, SIGNAL(leftClickedURL(const QString &)),
                       SLOT(showCreditURL(const QString &)));

    QBoxLayout *creditLayout = new QHBoxLayout(mainLayout);
    creditLayout->addStretch(); // Left spacer
    creditLayout->addWidget(logoLabel);
    creditLayout->addStretch(); // Right spacer

    setMainWidget(mainBox);
    setButtonText(User1, i18n("New Search"));
}

WebImageFetcherDialog::~WebImageFetcherDialog()
{
}

void WebImageFetcherDialog::showCreditURL(const QString &url)
{
    // Don't use static member since I'm sure that someday knowing my luck
    // Yahoo will change their mimetype they serve.
    (void) new KRun(KURL(url), topLevelWidget());
}

void WebImageFetcherDialog::setLayout()
{
    setCaption(QString("%1 - %2 (%3)")
              .arg(m_file.tag()->artist())
              .arg(m_file.tag()->album())
              .arg(m_imageList.size()));

    m_iconWidget->clear();
    for(uint i = 0; i < m_imageList.size(); i++)
        new CoverIconViewItem(m_iconWidget, m_imageList[i]);

    adjustSize();
}

void WebImageFetcherDialog::setImageList(const WebImageList &imageList)
{
    m_imageList = imageList;
}

void WebImageFetcherDialog::setFile(const FileHandle &file)
{
    m_file = file;
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void WebImageFetcherDialog::refreshScreen(WebImageList &imageList)
{
    setImageList(imageList);
    setLayout();
}

int WebImageFetcherDialog::exec()
{
    setLayout();
    return KDialogBase::exec();
}

void WebImageFetcherDialog::slotOk()
{
    uint selectedIndex = m_iconWidget->index(m_iconWidget->currentItem());
    m_pixmap = pixmapFromURL(m_imageList[selectedIndex].imageURL());

    if(m_pixmap.isNull()) {
        KMessageBox::sorry(this,
                           i18n("The cover you have selected is unavailable. Please select another."),
                           i18n("Cover Unavailable"));
        QPixmap blankPix;
        blankPix.resize(80, 80);
        blankPix.fill();
        m_iconWidget->currentItem()->setPixmap(blankPix, true, true);
        return;
    }

    accept();
    emit coverSelected();
}

void WebImageFetcherDialog::slotCancel()
{
    m_pixmap = QPixmap();
    reject();
}

void WebImageFetcherDialog::slotUser1()
{
    m_pixmap = QPixmap();
    accept();
    emit newSearchRequested();
}

QPixmap WebImageFetcherDialog::fetchedImage(uint index) const
{
    return (index > m_imageList.count()) ? QPixmap() : pixmapFromURL(m_imageList[index].imageURL());
}

QPixmap WebImageFetcherDialog::pixmapFromURL(const KURL &url) const
{
    QString file;

    if(KIO::NetAccess::download(url, file, 0)) {
        QPixmap pixmap = QPixmap(file);
        KIO::NetAccess::removeTempFile(file);
        return pixmap;
    }
    KIO::NetAccess::removeTempFile(file);
    return QPixmap();
}

////////////////////////////////////////////////////////////////////////////////
// CoverIconViewItem
////////////////////////////////////////////////////////////////////////////////

CoverIconViewItem::CoverIconViewItem(QIconView *parent, const WebImage &image) :
    QObject(parent), KIconViewItem(parent, parent->lastItem(), image.size()), m_job(0)
{
    // Set up the iconViewItem

    QPixmap mainMap;
    mainMap.resize(80, 80);
    mainMap.fill();
    setPixmap(mainMap, true, true);

    // Start downloading the image.

    m_job = KIO::get(image.thumbURL(), false, false);
    connect(m_job, SIGNAL(result(KIO::Job *)), this, SLOT(imageResult(KIO::Job *)));
    connect(m_job, SIGNAL(data(KIO::Job *, const QByteArray &)),
            this, SLOT(imageData(KIO::Job *, const QByteArray &)));
}

CoverIconViewItem::~CoverIconViewItem()
{
    if(m_job) {
        m_job->kill();

        // Drain results issued by KIO before being deleted,
        // and before deleting the job.
        kapp->eventLoop()->processEvents(QEventLoop::ExcludeUserInput);

        delete m_job;
    }
}

void CoverIconViewItem::imageData(KIO::Job *, const QByteArray &data)
{
    int currentSize = m_buffer.size();
    m_buffer.resize(currentSize + data.size(), QGArray::SpeedOptim);
    memcpy(&(m_buffer.data()[currentSize]), data.data(), data.size());
}

void CoverIconViewItem::imageResult(KIO::Job *job)
{
    if(job->error())
        return;

    QPixmap iconImage(m_buffer);
    iconImage = QImage(iconImage.convertToImage()).smoothScale(80, 80, QImage::ScaleMin);
    setPixmap(iconImage, true, true);
}

#include "webimagefetcherdialog.moc"
