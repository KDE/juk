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
#include <dom/html_document.h>
#include <dom/dom_string.h>
#include <dom/html_misc.h>
#include <dom/dom_node.h>
#include <kio/netaccess.h>
#include <kapplication.h>
#include <kpushbutton.h>
#include <klocale.h>
#include <qinputdialog.h>

#include <qfile.h>
#include <qvbox.h>
#include <qlayout.h>
#include <qwidget.h>
#include <qlabel.h>
#include <qdialog.h>

#include "googlefetcher.h"
#include "googlefetcherdialog.h"


GoogleFetcher::GoogleFetcher(const Tag *tag)
    : m_tag(tag),
      m_searchString(m_tag->artist() + " " + m_tag->album())
{

}

void GoogleFetcher::loadImageURLs()
{
    if(m_loadedQuery == m_searchString)
        return;

    m_urlList.clear();

    KURL url = "http://images.google.com/images?q="+m_searchString;
    DOM::HTMLDocument *search = new DOM::HTMLDocument;
    search->setAsync(false);
    search->load(url.url());
    DOM::HTMLCollection col = search->links();

    for(uint i = 0; i < col.length(); i++) {
        DOM::Node result = col.item(i).attributes().getNamedItem("href");
        if(!result.isNull()) {

            QString href = result.nodeValue().string();

            if(!href.isNull() && href.startsWith("/imgres")) {
                DOM::HTMLDocument *resdoc = new DOM::HTMLDocument;
                resdoc->setAsync(false);
                resdoc->load("http://images.google.com" + href + "&frame=small");
                DOM::HTMLCollection finalcol=resdoc->links();

                // The right link is always the first in the collection.

                m_urlList.append(finalcol.item(0).attributes().getNamedItem("href").nodeValue().string());
            }
        }
    }
    m_loadedQuery = m_searchString;
}

QPixmap GoogleFetcher::getPixmap()
{
    m_chosen = false;
    m_selectedIndex = 0;

    displayWaitBox();

    while(!m_chosen) {
        if(m_urlList.size() == 0) {
            bool ok;

            m_searchString = 
                QInputDialog::getText(i18n("Cover Downloader"),
                                      i18n("No covers found. Enter new search terms:"),
                                      QLineEdit::Normal, m_searchString, &ok, 0);

            if(ok && !m_searchString.isEmpty())
                displayWaitBox();
            else {
                m_currentPixmap = QPixmap();
                m_chosen = true;
            }
        }
        else {
            GoogleFetcherDialog dialog("google", m_urlList, m_selectedIndex, m_tag, 0);
            dialog.exec();
            m_currentPixmap = dialog.result();
            m_chosen = dialog.takeIt();
            if(dialog.newSearch()) {
                bool ok;
                m_searchString = QInputDialog::getText(i18n("Cover Downloader"),
                                                       i18n("Enter new search terms:"),
                                                       QLineEdit::Normal, m_searchString, &ok, 0);
                if(ok && !m_searchString.isEmpty())
                    displayWaitBox();
                else
                    m_searchString = m_loadedQuery;
            }
        }
    }
    return m_currentPixmap;
}

void GoogleFetcher::displayWaitBox()
{
    m_container = new QVBox(0, 0, Qt::WDestructiveClose);
    m_container->setCaption(kapp->makeStdCaption(m_tag->artist() + " - " +m_tag->album()));
    QWidget *widget = new QWidget(m_container);
    QLabel *pleaseWait = new QLabel(widget);
    pleaseWait->setText(i18n("Searching for Images. Please Wait..."));
    pleaseWait->adjustSize();
    m_container->setFixedSize(pleaseWait->size());
    m_container->show();
    loadImageURLs();

    // check that it wasn't closed

    if(pleaseWait->size().height() <= 0 || pleaseWait->size().width() <= 0) {
        m_chosen = true;
        return;
    }

    delete m_container;
}
