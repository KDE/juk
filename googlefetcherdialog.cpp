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

#include <klocale.h>
#include <dom/html_document.h>
#include <dom/dom_string.h>
#include <dom/html_misc.h>
#include <dom/dom_node.h>
#include <kio/netaccess.h>
#include <kapplication.h>

#include <qhbox.h>
#include <qimage.h>

#include "googlefetcherdialog.h"

GoogleFetcherDialog::GoogleFetcherDialog(const QString &name,
                                         const QValueList<QString> &urlList,
                                         uint selectedIndex,
                                         const Tag *tag,
                                         QWidget *parent) :
    KDialogBase(parent, name.latin1(), true, QString::null, Ok | Cancel | User1 | User2 | User3, NoDefault, true),
    m_urlList(urlList),
    m_pixmap(QPixmap()),
    m_takeIt(false),
    m_newSearch(false),
    m_index(selectedIndex),
    m_tag(tag)
{
    QHBox *mainBox = new QHBox(this);
    m_pixWidget = new QWidget(mainBox);
    setMainWidget(mainBox);
    setButtonText(User3, i18n("Previous"));
    setButtonText(User2, i18n("Next"));
    setButtonText(User1, i18n("New Search"));
}

GoogleFetcherDialog::~GoogleFetcherDialog()
{

}

void GoogleFetcherDialog::setLayout()
{
    m_pixmap = fetchedImage(m_index);

    if(m_pixmap.size().width() > 500 || m_pixmap.size().height() > 500)
        m_pixmap = QImage(m_pixmap.convertToImage()).smoothScale(500, 500);

    setCaption(QString("(%1/%2) %3 - %4").arg(m_index+1).arg(m_urlList.size())
               .arg(m_tag->artist()).arg(m_tag->album()));

    m_pixWidget->setPaletteBackgroundPixmap(m_pixmap);
    m_pixWidget->setFixedSize(m_pixmap.size());

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
    m_takeIt = true;
    m_newSearch = false;
    hide();
}

void GoogleFetcherDialog::slotCancel()
{
    m_takeIt = true;
    m_newSearch = false;
    m_pixmap=QPixmap();
    hide();
}

void GoogleFetcherDialog::slotUser3()
{
    m_takeIt = false;
    m_newSearch = false;

    if(m_index == 0)
        m_index = m_urlList.size() - 1;
    else
        m_index = m_index-1;

    setLayout();
}

void GoogleFetcherDialog::slotUser2()
{
    m_takeIt = false;
    m_newSearch = false;

    if(m_index >= m_urlList.size()-1)
        m_index = 0;
    else
        m_index = m_index+1;

    setLayout();
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
    if (index>m_urlList.count())
        return QPixmap();

    QValueListConstIterator<QString> returnVal = m_urlList.at(index);
    return getPixmapFromURL(*returnVal);
}

QPixmap GoogleFetcherDialog::getPixmapFromURL(KURL url) const
{
    kdDebug(65432) << "imageURL: " << url << endl;

    QString tmpFile;
    if(KIO::NetAccess::download(url, tmpFile, 0)) {
        QPixmap returnVal=QPixmap(tmpFile);
        QFile(tmpFile).remove();
        return returnVal;
    }
    return QPixmap();
}


#include "googlefetcherdialog.moc"
