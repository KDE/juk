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

#include <kio/netaccess.h>
#include <klocale.h>
#include <kdebug.h>
#include <kmessagebox.h>

#include <qhbox.h>
#include <qimage.h>

#include "googlefetcherdialog.h"
#include "tag.h"

GoogleFetcherDialog::GoogleFetcherDialog(const QString &name,
                                         const QValueList<GoogleImage> &imageList,
                                         uint selectedIndex,
                                         const FileHandle &file,
                                         QWidget *parent) :
    KDialogBase(parent, name.latin1(), true, QString::null,
                Ok | Cancel | User1 , NoDefault, true),
    m_pixmap(QPixmap()),
    m_imageList(imageList),
    m_takeIt(false),
    m_newSearch(false),
    m_index(selectedIndex),
    m_file(file)
{
    QHBox *mainBox = new QHBox(this);
    m_iconWidget = new KIconView(mainBox);
    m_iconWidget->setResizeMode(QIconView::Adjust);
    m_iconWidget->setSelectionMode(QIconView::Extended);
    m_iconWidget->setSpacing(10);
    m_iconWidget->setMode(KIconView::Select);
    m_iconWidget->setFixedSize(500,550);
    m_iconWidget->arrangeItemsInGrid();
    m_iconWidget->setItemsMovable(FALSE);
    
    setMainWidget(mainBox);
    setButtonText(User1, i18n("New Search"));
}

GoogleFetcherDialog::~GoogleFetcherDialog()
{

}

void GoogleFetcherDialog::setLayout()
{
    setCaption(QString("%1 - %2 (%3)")
              .arg(m_file.tag()->artist())
              .arg(m_file.tag()->album())
              .arg(m_imageList.size()));
    m_iconWidget->clear();
    for(uint i = 0; i < m_imageList.size(); i++) {
        new CoverIconViewItem(m_iconWidget, m_imageList[i]);
    }

    adjustSize();
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

int GoogleFetcherDialog::exec()
{
    setLayout();
    return KDialogBase::exec();
}

void GoogleFetcherDialog::slotOk()
{
    uint selectedIndex=m_iconWidget->index(m_iconWidget->currentItem());
    m_pixmap=pixmapFromURL(m_imageList[selectedIndex].imageURL());
    if (m_pixmap.isNull()) {
        KMessageBox::sorry(this, i18n("The cover you have selected is unavailable.  Please select another."), i18n("Cover unavailable"));
        QPixmap blankPix=QPixmap();
        blankPix.resize(80,80);
        blankPix.fill();
        m_iconWidget->currentItem()->setPixmap(blankPix, TRUE, TRUE);
        return;
    }
    m_takeIt = true;
    m_newSearch = false;
    hide();
}

void GoogleFetcherDialog::slotCancel()
{
    m_takeIt = true;
    m_newSearch = false;
    m_pixmap = QPixmap();
    hide();
}

void GoogleFetcherDialog::slotUser1()
{
    m_takeIt = false;
    m_newSearch = true;
    m_pixmap = QPixmap();
    hide();
}


QPixmap GoogleFetcherDialog::fetchedImage(uint index) const
{
    return (index > m_imageList.count()) ? QPixmap() : pixmapFromURL(m_imageList[index].imageURL());
}

QPixmap GoogleFetcherDialog::pixmapFromURL(const KURL &url) const
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

CoverIconViewItem::CoverIconViewItem(QIconView *parent, GoogleImage image) : 
                   QObject(),m_buffer(0),m_bufferIndex(0)
{
    //Set up the iconViewItem
    
    m_iconViewItem=new KIconViewItem(parent, parent->lastItem(), image.size());
    QPixmap mainMap=QPixmap();
    mainMap.resize(80,80);
    mainMap.fill();
    m_iconViewItem->setPixmap(mainMap, TRUE, TRUE);    
    
    //Start downloading the image.
    
    m_buffer = new uchar[BUFFER_SIZE];

    KIO::TransferJob* job = KIO::get(image.thumbURL(), false, false);
    connect(job, SIGNAL(result(KIO::Job*)), this, SLOT(imageResult(KIO::Job*)));
    connect(job, SIGNAL(data(KIO::Job*, const QByteArray&)), this, SLOT(imageData(KIO::Job*, const QByteArray&)));
   
}

CoverIconViewItem::~CoverIconViewItem()
{
    delete m_buffer;
}

void CoverIconViewItem::imageData(KIO::Job* job, const QByteArray& data)
{
    if(m_bufferIndex + (uint) data.size() >= BUFFER_SIZE)
        return;

    memcpy(m_buffer + m_bufferIndex, data.data(), data.size());
    m_bufferIndex += data.size();
}

void CoverIconViewItem::imageResult(KIO::Job* job)
{
    if(job->error())
        return;
    
    QPixmap iconImage=QPixmap();
    iconImage.loadFromData(m_buffer, m_bufferIndex);
    iconImage=QImage(iconImage.convertToImage()).smoothScale(80,80);
    m_iconViewItem->setPixmap(iconImage, TRUE, TRUE);
}

#include "googlefetcherdialog.moc"
