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
#include "tag.h"

GoogleFetcher::GoogleFetcher(const FileHandle &file)
    : m_file(file),
      m_searchString(file.tag()->artist() + " " + file.tag()->album())
{

}

void GoogleFetcher::loadImageURLs()
{
    if(m_loadedQuery == m_searchString)
        return;

    m_imageList.clear();

    KURL url = "http://images.google.com/images?q=" + m_searchString;

    DOM::HTMLDocument search;
    search.setAsync(false);
    search.load(url.url());

    DOM::Node body = search.body();
    DOM::NodeList topLevelNodes = body.childNodes();

    // On google results, if the fifth (0-based) node is a "Font" node, 
    // then there are results.  Otherwise, there are no results.

    DOM::Node fourthNode = topLevelNodes.item(4);

    if (topLevelNodes.length() > 5 &&
          topLevelNodes.item(4).nodeName().string() == "font") {

        // Go through each of the top (table) nodes

        for(uint i = 5; i < topLevelNodes.length(); i++) {
            DOM::Node thisTopNode = topLevelNodes.item(i);

            if(thisTopNode.nodeName().string() == "table") {
                DOM::NodeList images = thisTopNode.firstChild().firstChild().childNodes();

                // For each table node, pull the images out of the first row

                for(uint j = 0; j < images.length(); j++) {
                    QString imageURL = "http://images.google.com" + images.item(j).firstChild().firstChild().attributes().getNamedItem("src").nodeValue().string();
                    DOM::Node topFont = thisTopNode.firstChild().childNodes().item(1).childNodes().item(j).firstChild();

                    // And pull the size out of the second row

                    for(uint k = 0; k < topFont.childNodes().length(); k++) {
                        if(topFont.childNodes().item(k).nodeName().string() == "font") {
                            m_imageList.append(GoogleImage(imageURL, topFont.childNodes().item(k).firstChild().nodeValue().string()));
                        }
                    }
                }
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
        if(m_imageList.size() == 0) {
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
            GoogleFetcherDialog dialog("google", m_imageList, m_selectedIndex, m_file, 0);
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
