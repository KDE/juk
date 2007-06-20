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

#include <qhttp.h>
#include <qdom.h>
#include <qwaitcondition.h>

#include <kapplication.h>
#include <kstatusbar.h>
#include <kdebug.h>
#include <kmainwindow.h>
#include <klocale.h>
#include <kinputdialog.h>
#include <kurl.h>

#include "covermanager.h"
#include "webimagefetcher.h"
#include "webimagefetcherdialog.h"
#include "tag.h"

WebImage::WebImage()
{
}

WebImage::WebImage(const QString &imageURL, const QString &thumbURL,
                         int width, int height) :
    m_imageURL(imageURL),
    m_thumbURL(thumbURL),
    m_size(QString("\n%1 x %2").arg(width).arg(height))
{
}


WebImageFetcher::WebImageFetcher(QObject *parent)
    : QObject(parent),
      m_connection(new QHttp(this)),
      m_connectionId(-1),
      m_dialog(0)
{
    connect(m_connection, SIGNAL(requestFinished(int,bool)), SLOT(slotWebRequestFinished(int,bool)));
}

WebImageFetcher::~WebImageFetcher()
{
    delete m_dialog;
}

void WebImageFetcher::setFile(const FileHandle &file)
{
    m_file = file;
    m_searchString = QString(file.tag()->artist() + ' ' + file.tag()->album());

    if(m_dialog)
	m_dialog->setFile(file);
}

void WebImageFetcher::abortSearch()
{
    m_connection->abort();
}

void WebImageFetcher::chooseCover()
{
    slotLoadImageURLs();
}

void WebImageFetcher::slotLoadImageURLs()
{
    m_imageList.clear();

    KURL url("http://search.yahooapis.com/ImageSearchService/V1/imageSearch");
    url.addQueryItem("appid", "org.kde.juk/kde3");
    url.addQueryItem("query", m_searchString);
    url.addQueryItem("results", "25");

    kdDebug(65432) << "Using request " << url.encodedPathAndQuery() << endl;

    m_connection->setHost(url.host());
    m_connectionId = m_connection->get(url.encodedPathAndQuery());

    // Wait for the results...
}

void WebImageFetcher::slotWebRequestFinished(int id, bool error)
{
    if(id != m_connectionId)
	return;

    if(error) {
	kdError(65432) << "Error reading image results from Yahoo!\n";
	kdError(65432) << m_connection->errorString() << endl;
	return;
    }

    QDomDocument results("ResultSet");

    QString errorStr;
    int errorCol, errorLine;
    if(!results.setContent(m_connection->readAll(), &errorStr, &errorLine, &errorCol)) {
	kdError(65432) << "Unable to create XML document from Yahoo results.\n";
	kdError(65432) << "Line " << errorLine << ", " << errorStr << endl;
	return;
    }

    QDomNode n = results.documentElement();

    bool hasNoResults = false;

    if(n.isNull()) {
	kdDebug(65432) << "No document root in XML results??\n";
	hasNoResults = true;
    }
    else {
	QDomElement result = n.toElement();
	if(result.attribute("totalResultsReturned").toInt() == 0)
	    kdDebug(65432) << "Search returned " << result.attribute("totalResultsAvailable") << " results.\n";

	if(result.isNull() || !result.hasAttribute("totalResultsReturned") ||
	    result.attribute("totalResultsReturned").toInt() == 0)
	{
	    hasNoResults = true;
	}
    }

    if(hasNoResults)
    {
	kdDebug(65432) << "Search returned no results.\n";
	requestNewSearchTerms(true /* no results */);
        return;
    }

    // Go through each of the top (result) nodes

    n = n.firstChild();
    while(!n.isNull()) {
	QDomNode resultUrl = n.namedItem("Url");
	QDomNode thumbnail = n.namedItem("Thumbnail");
	QDomNode height = n.namedItem("Height");
	QDomNode width = n.namedItem("Width");

	// We have the necessary info, move to next node before we forget.
	n = n.nextSibling();

	if(resultUrl.isNull() || thumbnail.isNull() || height.isNull() || width.isNull()) {
	    kdError(65432) << "Invalid result returned, skipping.\n";
	    continue;
	}

	m_imageList.append(
	    WebImage(
	        resultUrl.toElement().text(),
		thumbnail.namedItem("Url").toElement().text(),
		width.toElement().text().toInt(),
		height.toElement().text().toInt()
	    )
	);
    }

    // Have results, show them and pick one.

    if(!m_dialog) {
        m_dialog = new WebImageFetcherDialog(m_imageList, m_file, 0);
	m_dialog->setModal(true);

	connect(m_dialog, SIGNAL(coverSelected()), SLOT(slotCoverChosen()));
	connect(m_dialog, SIGNAL(newSearchRequested()), SLOT(slotNewSearch()));
    }

    m_dialog->refreshScreen(m_imageList);
    m_dialog->show();
}

void WebImageFetcher::slotCoverChosen()
{
    QPixmap pixmap = m_dialog->result();
    if(pixmap.isNull()) {
	kdError(65432) << "Selected pixmap is null for some reason.\n";
	return;
    }

    kdDebug(65432) << "Adding new cover for " << m_file.tag()->fileName() << endl;
    coverKey newId = CoverManager::addCover(pixmap, m_file.tag()->artist(), m_file.tag()->album());
    emit signalCoverChanged(newId);
}

void WebImageFetcher::slotNewSearch()
{
    requestNewSearchTerms();
}

void WebImageFetcher::displayWaitMessage()
{
    KStatusBar *statusBar = static_cast<KMainWindow *>(kapp->mainWidget())->statusBar();
    statusBar->message(i18n("Searching for Images. Please Wait..."));
    slotLoadImageURLs();
    statusBar->clear();
}

void WebImageFetcher::requestNewSearchTerms(bool noResults)
{
    bool ok;
    QString search = KInputDialog::getText(i18n("Cover Downloader"),
                                           noResults ?
                                             i18n("No matching images found, please enter new search terms:") :
                                             i18n("Enter new search terms:"),
                                           m_searchString, &ok);
    if(ok && !search.isEmpty()) {
	m_searchString = search;
        displayWaitMessage(); // This kicks off the new search.
    }
}

#include "webimagefetcher.moc"
