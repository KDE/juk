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

#include "googlefetcherdialog.h"
#include "tag.h"

#include <kapplication.h>
#include <kio/netaccess.h>
#include <kio/job.h>
#include <klocale.h>
#include <kdebug.h>
#include <k3iconview.h>
#include <kmessagebox.h>
#include <krun.h>
#include <kcombobox.h>
#include <kiconloader.h>
#include <kurllabel.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QPushButton>
#include <QEventLoop>
#include <QPixmap>

GoogleFetcherDialog::GoogleFetcherDialog(const GoogleImageList &imageList,
                                         const FileHandle &file,
                                         QWidget *parent) :
    KDialog(parent),
    m_pixmap(QPixmap()),
    m_imageList(imageList),
    m_file(file)
{
    setObjectName("internet_image_fetcher");
    setModal(true);
    setButtons(Ok | Cancel | User1);
    setDefaultButton(NoDefault);
    showButtonSeparator(true);

#ifdef __GNUC__
    #warning KDE4 How to port this?
#endif
    //disableResize();

    QWidget *mainBox = new QWidget(this);
    QBoxLayout *mainLayout = new QVBoxLayout(mainBox);

    m_iconWidget = new K3IconView(mainBox);
    m_iconWidget->setResizeMode(Q3IconView::Adjust);
    m_iconWidget->setSpacing(10);
    m_iconWidget->setFixedSize(500,550);
    m_iconWidget->arrangeItemsInGrid();
    m_iconWidget->setItemsMovable(false);
    mainLayout->addWidget(m_iconWidget);
    connect(m_iconWidget, SIGNAL(executed(Q3IconViewItem *)),
            this, SLOT(slotOk()));

    // Before changing the code below be sure to check the attribution terms
    // of the Yahoo Image Search API.
    // http://developer.yahoo.com/attribution/
    KUrlLabel *logoLabel = new KUrlLabel(mainBox);
    logoLabel->setUrl("http://developer.yahoo.com/about/");
    logoLabel->setPixmap(UserIcon("yahoo_credit"));
    logoLabel->setMargin(15);    // Allow large margin per attribution terms.
    logoLabel->setUseTips(true); // Show URL in tooltip.
    connect(logoLabel, SIGNAL(leftClickedURL(const QString &)),
                       SLOT(showCreditURL(const QString &)));

    QBoxLayout *creditLayout = new QHBoxLayout;
    mainLayout->addLayout(creditLayout);

    creditLayout->addStretch(); // Left spacer
    creditLayout->addWidget(logoLabel);
    creditLayout->addStretch(); // Right spacer

    setMainWidget(mainBox);
    setButtonText(User1, i18n("New Search"));

    connect(this, SIGNAL(user1Clicked()),  SIGNAL(newSearchRequested()));
    connect(this, SIGNAL(okClicked()),     SLOT(slotOk()));
    connect(this, SIGNAL(cancelClicked()), SLOT(slotCancel()));
}

GoogleFetcherDialog::~GoogleFetcherDialog()
{
}

void GoogleFetcherDialog::showCreditURL(const QString &url)
{
    // Don't use static member since I'm sure that someday knowing my luck
    // Yahoo will change their mimetype they serve.
    (void) new KRun(KUrl(url), topLevelWidget());
}

void GoogleFetcherDialog::setLayout()
{
    setCaption(QString("%1 - %2 (%3)")
              .arg(m_file.tag()->artist())
              .arg(m_file.tag()->album())
              .arg(m_imageList.size()));

    m_iconWidget->clear();
    for(int i = 0; i < m_imageList.size(); i++)
        new CoverIconViewItem(m_iconWidget, m_imageList[i]);

    adjustSize();
}

void GoogleFetcherDialog::setImageList(const GoogleImageList &imageList)
{
    m_imageList = imageList;
}

void GoogleFetcherDialog::setFile(const FileHandle &file)
{
    m_file = file;
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void GoogleFetcherDialog::refreshScreen(GoogleImageList &imageList)
{
    setImageList(imageList);
    setLayout();
}

int GoogleFetcherDialog::exec()
{
    setLayout();
    return KDialog::exec();
}

void GoogleFetcherDialog::slotOk()
{
    int selectedIndex = m_iconWidget->index(m_iconWidget->currentItem());
    m_pixmap = pixmapFromURL(m_imageList[selectedIndex].imageURL());

    if(m_pixmap.isNull()) {
        KMessageBox::sorry(this,
                           i18n("The cover you have selected is unavailable. Please select another."),
                           i18n("Cover Unavailable"));
        QPixmap blankPix(80,80);
        blankPix.fill();
        m_iconWidget->currentItem()->setPixmap(blankPix, true, true);
        return;
    }

    accept();
    emit coverSelected();
}

void GoogleFetcherDialog::slotCancel()
{
    m_pixmap = QPixmap();
    reject();
}

QPixmap GoogleFetcherDialog::fetchedImage(int index) const
{
    return (index > m_imageList.count()) ? QPixmap() : pixmapFromURL(m_imageList[index].imageURL());
}

QPixmap GoogleFetcherDialog::pixmapFromURL(const KUrl &url) const
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

CoverIconViewItem::CoverIconViewItem(Q3IconView *parent, const GoogleImage &image) :
    QObject(parent), K3IconViewItem(parent, parent->lastItem(), image.size()), m_job(0)
{
    // Set up the iconViewItem

    QPixmap mainMap(80,80);
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
        kapp->processEvents(QEventLoop::ExcludeUserInput);

        delete m_job;
    }
}

void CoverIconViewItem::imageData(KIO::Job *, const QByteArray &data)
{
    int currentSize = m_buffer.size();
    m_buffer.resize(currentSize + data.size());
    memcpy(&(m_buffer.data()[currentSize]), data.data(), data.size());
}

void CoverIconViewItem::imageResult(KIO::Job *job)
{
    if(job->error())
        return;

    QPixmap iconImage(m_buffer);
    setPixmap(iconImage.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

#include "googlefetcherdialog.moc"

// vim: set et sw=4 tw=0 sta:
