/***************************************************************************
    copyright            : (C) 2004 Nathan Toone <nathan@toonetown.com>
    copyright            : (C) 2007 Michael Pyne <michael.pyne@kdemail.com>
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
#include <dom/html_table.h>
#include <dom/dom_exception.h>
#include <dom/dom2_traversal.h>

#include <khtml_part.h>

#include <kapplication.h>
#include <kstatusbar.h>
#include <kxmlguiwindow.h>
#include <klocale.h>
#include <kinputdialog.h>
#include <kurl.h>

#include "googlefetcher.h"
#include "googlefetcherdialog.h"
#include "tag.h"
#include "juk.h"

#include <QPixmap>

GoogleImage::GoogleImage(QString thumbURL, QString size) :
    m_thumbURL(thumbURL)
{
    // thumbURL is in the following format - and we can regex the imageURL
    // images?q=tbn:hKSEWNB8aNcJ:www.styxnet.com/deyoung/styx/stygians/cp_portrait.jpg

    m_imageURL = thumbURL.remove(QRegExp("^.*q=tbn:[^:]*:"));

    // Ensure that the image url starts with http if it doesn't already.
    if(!m_imageURL.startsWith("http://"))
        m_imageURL.prepend("http://");

    m_size = size.replace("pixels - ", "\n(") + ')';
}


GoogleFetcher::GoogleFetcher(const FileHandle &file)
    : m_file(file),
      m_searchString(file.tag()->artist() + ' ' + file.tag()->album())
{

}

void GoogleFetcher::slotLoadImageURLs(GoogleFetcher::ImageSize size)
{
    if(m_loadedQuery == m_searchString && m_loadedSize == size)
        return;

    m_imageList.clear();

    KUrl url("http://images.google.com/images");
    url.addQueryItem("q", m_searchString);
    url.addQueryItem("hl", "en");
    url.addQueryItem("nojs", "1");

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

    // We don't normally like exceptions but missing DOMException will kill
    // JuK rather abruptly whether we like it or not so we don't really have a
    // choice if we're going to screen-scrape Google.
    try {

    KHTMLPart part;

    // Create empty document.

    part.begin();
    part.end();

    DOM::HTMLDocument search = part.htmlDocument();
    search.setAsync(false); // Grab the document before proceeding.

    kDebug(65432) << "Performing Google Search: " << url << endl;

    search.load(url.url());

    DOM::HTMLElement body = search.body();
    DOM::NodeList topLevelNodes = body.getElementsByTagName("table");

    if(!hasImageResults(search))
    {
        kDebug(65432) << "Search returned no results.\n";
        emit signalNewSearch(m_imageList);
        return;
    }

    // Go through each of the top (table) nodes

    for(uint i = 0; i < topLevelNodes.length(); i++) {
        DOM::Node thisTopNode = topLevelNodes.item(i);

        // The get named item test seems to accurately determine whether a
        // <TABLE> tag contains the actual images or is just layout filler.
        // The parent node check is due to the fact that we only want top-level
        // tables, but the getElementsByTagName returns all tables in the
        // tree.
        DOM::HTMLTableElement table = thisTopNode;
        if(table.isNull() || table.parentNode() != body || table.getAttribute("align").isEmpty())
            continue;

        DOM::HTMLCollection rows = table.rows();
        uint imageIndex = 0;

        // Some tables will have an extra row saying "Displaying only foo-size
        // images".  These tables have three rows, so we need to have
        // increment imageIndex for these.
        if(rows.length() > 2)
            imageIndex = 1;

        // A list of <TDs> containing the hyperlink to the site, with image.
        DOM::NodeList images = rows.item(imageIndex).childNodes();

        // For each table node, pull the images out of the first row

        for(uint j = 0; j < images.length(); j++) {
            DOM::Element tdElement = images.item(j);
            if(tdElement.isNull()) {
                // Whoops....
                kError(65432) << "Expecting a <TD> in a <TR> parsing Google Images!\n";
                continue;
            }

            // Grab first item out of list of images.  There should only be
            // one anyways.
            DOM::Element imgElement = tdElement.getElementsByTagName("img").item(0);
            if(imgElement.isNull()) {
                kError(65432) << "Expecting a <IMG> in a <TD> parsing Google Images!\n";
                continue;
            }

            QString imageURL = "http://images.google.com" +
                imgElement.getAttribute("src").string();

            // Pull the matching <TD> node for the row under the one we've
            // got.
            tdElement = rows.item(imageIndex + 1).childNodes().item(j);

            // Iterate over it until we find a string with "pixels".
            unsigned long whatToShow = DOM::NodeFilter::SHOW_TEXT;
            DOM::NodeIterator it = search.createNodeIterator(tdElement, whatToShow, 0, false);
            DOM::Node node;

            for(node = it.nextNode(); !node.isNull(); node = it.nextNode()) {
                if(node.nodeValue().string().contains("pixels")) {
                    m_imageList.append(GoogleImage(imageURL, node.nodeValue().string()));
                    break;
                }
            }
        }
    }
    } // try
    catch (DOM::DOMException &e)
    {
        kError(65432) << "Caught DOM Exception: " << e.code << endl;
    }
    catch (...)
    {
        kError(65432) << "Caught unknown exception.\n";
    }

    emit signalNewSearch(m_imageList);
}

QPixmap GoogleFetcher::pixmap()
{
    bool chosen = false;
    m_loadedSize = All;

    displayWaitMessage();

    QPixmap pixmap;

    while(!chosen) {

        if(m_imageList.isEmpty())
            chosen = !requestNewSearchTerms(true);
        else {
            GoogleFetcherDialog dialog("google", m_imageList, m_file, 0);
            connect(&dialog, SIGNAL(sizeChanged(GoogleFetcher::ImageSize)),
                    this, SLOT(slotLoadImageURLs(GoogleFetcher::ImageSize)));
            connect(this, SIGNAL(signalNewSearch(GoogleImageList &)),
                    &dialog, SLOT(refreshScreen(GoogleImageList &)));
            dialog.exec();
            pixmap = dialog.result();
            chosen = dialog.takeIt();

            if(dialog.newSearch())
                requestNewSearchTerms();
        }
    }
    return pixmap;
}

void GoogleFetcher::displayWaitMessage()
{
    KStatusBar *statusBar = JuK::JuKInstance()->statusBar();
    statusBar->showMessage(i18n("Searching for Images. Please Wait..."));
    slotLoadImageURLs();
    statusBar->clearMessage();
}

bool GoogleFetcher::requestNewSearchTerms(bool noResults)
{
    bool ok;
    m_searchString = KInputDialog::getText(i18n("Cover Downloader"),
                                           noResults ?
                                             i18n("No matching images found, please enter new search terms:") :
                                             i18n("Enter new search terms:"),
                                           m_searchString, &ok);
    if(ok && !m_searchString.isEmpty())
        displayWaitMessage();
    else
        m_searchString = m_loadedQuery;

    return ok;
}

bool GoogleFetcher::hasImageResults(DOM::HTMLDocument &search)
{
    unsigned long typesToShow = DOM::NodeFilter::SHOW_TEXT;

    DOM::NodeIterator it = search.createNodeIterator(search.body(), typesToShow, 0, false);
    DOM::Node node;

    for(node = it.nextNode(); !node.isNull(); node = it.nextNode()) {
        // node should be a text node.
        if(node.nodeValue().string().contains("did not match any"))
            return false;
    }

    return true;
}

#include "googlefetcher.moc"

// vim: set et sw=4 tw=0 sta:
