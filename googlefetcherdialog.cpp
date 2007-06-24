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
#include <kmessagebox.h>
#include <krun.h>
#include <kiconloader.h>
#include <kurllabel.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QIcon>
#include <QEventLoop>
#include <QListView>
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

    QWidget *mainBox = new QWidget(this);
    QBoxLayout *mainLayout = new QVBoxLayout(mainBox);
    mainLayout->setMargin(0); // No extra padding needed.
    mainLayout->setSpacing(spacingHint());

    m_iconWidget = new QListView(mainBox);
    m_iconWidget->setGridSize(QSize(100, 120));
    m_iconWidget->setViewMode(QListView::IconMode);
    m_iconWidget->setResizeMode(QListView::Adjust);
    m_iconWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_iconWidget->setVerticalScrollMode(QListView::ScrollPerItem);
    m_iconWidget->setMovement(QListView::Static);
    m_iconWidget->setSelectionMode(QListView::SingleSelection);
    m_iconWidget->setIconSize(QSize(80, 80));

    mainLayout->addWidget(m_iconWidget);
    connect(m_iconWidget, SIGNAL(activated(const QModelIndex &)),
            this, SLOT(slotActivated(const QModelIndex &)));

    // Before changing the code below be sure to check the attribution terms
    // of the Yahoo Image Search API.
    // http://developer.yahoo.com/attribution/
    KUrlLabel *logoLabel = new KUrlLabel(mainBox);
    logoLabel->setUrl("http://developer.yahoo.com/about/");
    logoLabel->setPixmap(UserIcon("yahoo_credit"));
    logoLabel->setMargin(15);    // Allow large margin per attribution terms.
    logoLabel->setUseTips(true); // Show URL in tooltip.
    connect(logoLabel, SIGNAL(leftClickedUrl(const QString &)),
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

    setInitialSize(QSize(500, 480));
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

    QStandardItemModel *model = new QStandardItemModel(m_iconWidget);
    QAbstractItemModel *oldModel = m_iconWidget->model();

    foreach(GoogleImage image, m_imageList) {
        CoverIconViewItem *item = new CoverIconViewItem(m_iconWidget, image);
        model->appendRow(item);
    }

    m_iconWidget->setModel(model);
    delete oldModel;

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
    slotActivated(m_iconWidget->currentIndex());
}

void GoogleFetcherDialog::slotActivated(const QModelIndex &index)
{
    m_pixmap = pixmapFromURL(m_imageList[index.row()].imageURL());

    if(m_pixmap.isNull()) {
        KMessageBox::sorry(this,
                           i18n("The cover you have selected is unavailable. Please select another."),
                           i18n("Cover Unavailable"));

        QTimer::singleShot(0, this, SLOT(selectedItemIsBad()));
        return;
    }

    accept();
    emit coverSelected();
}

void GoogleFetcherDialog::selectedItemIsBad()
{
    QModelIndex index = m_iconWidget->currentIndex();

    QStandardItemModel *model = static_cast<QStandardItemModel *>(m_iconWidget->model());
    QStandardItem *item = model->itemFromIndex(index);
    if(!item)
        return;

    item->setIcon(DesktopIcon("dialog-error"));
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

CoverIconViewItem::CoverIconViewItem(QWidget *parent, const GoogleImage &image) :
    QObject(parent), QStandardItem(image.size()), m_job(0)
{
    // Set up the iconViewItem

    setIcon(DesktopIcon("system-search"));

    // Start downloading the image.

    m_job = KIO::storedGet(image.thumbURL());
    connect(m_job, SIGNAL(result(KJob *)), this, SLOT(imageResult(KJob *)));
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

void CoverIconViewItem::imageResult(KJob *job)
{
    if(job != m_job) {
        kError() << "Wrong slot called.\n";
        setIcon(DesktopIcon("dialog-error"));
        return;
    }

    if(job->error()) {
        kError() << "Unable to grab image\n";
        setIcon(DesktopIcon("dialog-error"));
        return;
    }

    // Create thumbnail to show on icon.  At least by Qt 4.3 the standard
    // item delegate is still retarded when it comes to drawing uneven sized
    // items in a grid, as shorter items (in height) have the corresponding
    // text shoved up below meaning the text doesn't line up.  So, force every
    // icon to be the exact same size manually...

    QPixmap iconImage, realImage(80, 80);
    iconImage.loadFromData(m_job->data());
    realImage.fill(Qt::transparent);

    if(iconImage.isNull()) {
        kError() << "Thumbnail image is not of a supported format\n";
        setIcon(DesktopIcon("dialog-error"));
        return;
    }

    // Scale down if necesssary
    if(iconImage.width() > 80 || iconImage.height() > 80)
        iconImage = iconImage.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QPainter p;
    QRect targetRect(QPoint(0, 0), iconImage.size());
    p.begin(&realImage);

    // Center thumbnail in 80x80 pixmap
    targetRect.setWidth(iconImage.width());
    targetRect.moveLeft((realImage.width() - iconImage.width()) / 2);
    targetRect.setHeight(iconImage.height());
    targetRect.moveTop((realImage.height() - iconImage.height()) / 2);

    p.drawPixmap(targetRect, iconImage, iconImage.rect());
    p.end();

    setIcon(realImage);
}

#include "googlefetcherdialog.moc"

// vim: set et sw=4 tw=0 sta:
