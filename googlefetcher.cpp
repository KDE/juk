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

GoogleImage::GoogleImage(QString thumbURL, QString size) :
    m_thumbURL(thumbURL)
{
    // thumbURL is in the following format - and we can regex the imageURL
    // images?q=tbn:hKSEWNB8aNcJ:www.styxnet.com/deyoung/styx/stygians/cp_portrait.jpg

    m_imageURL = "http://" + thumbURL.remove(QRegExp("^.*q=tbn:[^:]*:"));
    m_size = size.replace("pixels - ", "\n(") + ")";
}


GoogleFetcher::GoogleFetcher(const FileHandle &file)
    : m_file(file),
      m_searchString(file.tag()->artist() + " " + file.tag()->album())
{

}

void GoogleFetcher::slotLoadImageURLs(GoogleFetcher::ImageSize size)
{
    if(m_loadedQuery == m_searchString && m_loadedSize == size)
        return;

    m_imageList.clear();

    KURL url("http://images.google.com/images");
    url.addQueryItem("q", m_searchString);
    
    switch (size) {
        case XLarge:
            url.addQueryItem("imgsz", "xlarge|xxlarge");
            break;
        case Large:
            url.addQueryItem("imgsz", "large");
            break;
        case Medium:
            url.addQueryItem("imgsz", "medium");
            break;
        case Small:
            url.addQueryItem("imgsz", "small");
            break;
        case Icon:
            url.addQueryItem("imgsz", "icon");
            break;
        default:
            break;
    }
    
    m_loadedQuery = m_searchString;
    m_loadedSize = size;

    DOM::HTMLDocument search;
    search.setAsync(false);
    search.load(url.url());

    DOM::Node body = search.body();
    DOM::NodeList topLevelNodes = body.childNodes();

    // On google results, if the fifth (0-based) node is a "Font" node, 
    // then there are results.  Otherwise, there are no results.

    DOM::Node fourthNode = topLevelNodes.item(4);

    if(topLevelNodes.length() <= 5 ||
       topLevelNodes.item(4).nodeName().string() != "font")
    {
        emit signalNewSearch(m_imageList);
        return;
    }

    // Go through each of the top (table) nodes

    for(uint i = 5; i < topLevelNodes.length(); i++) {
        DOM::Node thisTopNode = topLevelNodes.item(i);

        if(thisTopNode.nodeName().string() == "table") {

            uint imageIndex = 0;
            if(!thisTopNode.firstChild().firstChild().firstChild()
               .attributes().getNamedItem("colspan").isNull())
            {
                imageIndex = 1;
            }

            DOM::NodeList images = thisTopNode.firstChild().childNodes().item(imageIndex).childNodes();

            // For each table node, pull the images out of the first row
                
            for(uint j = 0; j < images.length(); j++) {
                QString imageURL = "http://images.google.com" +
                    images.item(j).firstChild().firstChild().attributes()
                    .getNamedItem("src").nodeValue().string();

                DOM::Node topFont = thisTopNode.firstChild().childNodes()
                    .item(imageIndex + 1).childNodes().item(j).firstChild();

                // And pull the size out of the second row

                for(uint k = 0; k < topFont.childNodes().length(); k++) {
                    if(topFont.childNodes().item(k).nodeName().string() == "font") {
                        m_imageList.append(GoogleImage(imageURL, topFont.childNodes().item(k)
                                                       .firstChild().nodeValue().string()));
                    }
                }
            }
        }
    }
    emit signalNewSearch(m_imageList);
}

QPixmap GoogleFetcher::pixmap()
{
    bool chosen = false;
    const int selectedIndex = 0;
    m_loadedSize = All;

    displayWaitMessage();

    QPixmap pixmap;

    while(!chosen) {
        GoogleFetcherDialog dialog("google", m_imageList, selectedIndex, m_file, 0);

        connect(&dialog, SIGNAL(sizeChanged(GoogleFetcher::ImageSize)),
                this, SLOT(slotLoadImageURLs(GoogleFetcher::ImageSize)));
        connect(this, SIGNAL(signalNewSearch(GoogleImageList &)),
                &dialog, SLOT(refreshScreen(GoogleImageList &)));
        dialog.exec();
        pixmap = dialog.result();
        chosen = dialog.takeIt();
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
    return pixmap;
}

void GoogleFetcher::displayWaitMessage()
{
    KStatusBar *statusBar = static_cast<KMainWindow *>(kapp->mainWidget())->statusBar();
    statusBar->message(i18n("Searching for Images. Please Wait..."));
    slotLoadImageURLs();
    statusBar->clear();
}

#include "googlefetcher.moc"

