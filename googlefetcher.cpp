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
#include <dom/html_misc.h>

#include <kapplication.h>
#include <kstatusbar.h>
#include <kmainwindow.h>
#include <klocale.h>
#include <kinputdialog.h>
#include <kurl.h>

#include "googlefetcher.h"
#include "googlefetcherdialog.h"

GoogleFetcher::GoogleFetcher(const FileHandle &file)
    : m_file(file),
      m_searchString(file.tag()->artist() + " " + file.tag()->album())
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

QPixmap GoogleFetcher::pixmap()
{
    m_chosen = false;
    m_selectedIndex = 0;

    displayWaitMessage();

    while(!m_chosen) {
        if(m_urlList.size() == 0) {
            bool ok;

            m_searchString = 
                KInputDialog::getText(i18n("Cover Downloader"),
                                      i18n("No covers found. Enter new search terms:"),
                                      m_searchString, &ok);

            if(ok && !m_searchString.isEmpty())
                displayWaitMessage();
            else {
                m_currentPixmap = QPixmap();
                m_chosen = true;
            }
        }
        else {
            GoogleFetcherDialog dialog("google", m_urlList, m_selectedIndex, m_file, 0);
            dialog.exec();
            m_currentPixmap = dialog.result();
            m_chosen = dialog.takeIt();
            if(dialog.newSearch()) {
                bool ok;
                m_searchString = KInputDialog::getText(i18n("Cover Downloader"),
                                                       i18n("Enter new search terms:"),
                                                       m_searchString, &ok);
                if(ok && !m_searchString.isEmpty())
                    displayWaitMessage();
                else
                    m_searchString = m_loadedQuery;
            }
        }
    }
    return m_currentPixmap;
}

void GoogleFetcher::displayWaitMessage()
{
    KStatusBar *statusBar = static_cast<KMainWindow *>(kapp->mainWidget())->statusBar();
    statusBar->message(i18n("Searching for Images. Please Wait..."));
    loadImageURLs();
    statusBar->clear();
}
