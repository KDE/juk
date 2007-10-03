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

#include "webimagefetcher.h"

#include <kapplication.h>
#include <kstatusbar.h>
#include <kxmlguiwindow.h>
#include <klocale.h>
#include <kinputdialog.h>
#include <kurl.h>
#include <kdebug.h>
#include <kio/job.h>

#include "covermanager.h"
#include "webimagefetcherdialog.h"
#include "filehandle.h"
#include "tag.h"
#include "juk.h"

#include <QPixmap>
#include <QDomDocument>
#include <QDomElement>
#include <QPointer>
#include <QList>

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

class WebImageFetcher::Private
{
    friend class WebImageFetcher;

    Private() : selectedIndex(0), connection(0), dialog(0)
    {
    }

    FileHandle file;
    QString searchString;
    QString loadedQuery;
    WebImageList imageList;
    uint selectedIndex;
    QPointer<KIO::StoredTransferJob> connection;
    WebImageFetcherDialog *dialog;
};

WebImageFetcher::WebImageFetcher(QObject *parent)
    : QObject(parent), d(new Private)
{
}

WebImageFetcher::~WebImageFetcher()
{
    delete d->connection;
    delete d;
}

void WebImageFetcher::setFile(const FileHandle &file)
{
    d->file = file;
    d->searchString = QString(file.tag()->artist() + ' ' + file.tag()->album());

    if(d->dialog)
	d->dialog->setFile(file);
}

void WebImageFetcher::abortSearch()
{
    if(d->connection)
        d->connection->kill();
}

void WebImageFetcher::chooseCover()
{
    slotLoadImageURLs();
}

void WebImageFetcher::slotLoadImageURLs()
{
    d->imageList.clear();

    KUrl url("http://search.yahooapis.com/ImageSearchService/V1/imageSearch");
    url.addQueryItem("appid", "org.kde.juk/kde4");
    url.addQueryItem("query", d->searchString);
    url.addQueryItem("results", "25");

    kDebug(65432) << "Using request " << url.encodedPathAndQuery();

    d->connection = KIO::storedGet(url, KIO::Reload /* reload always */);
    connect(d->connection, SIGNAL(result(KJob *)), SLOT(slotWebRequestFinished(KJob *)));

    // Wait for the results...
}

void WebImageFetcher::slotWebRequestFinished(KJob *job)
{
    kDebug(65432) ;

    if(job != d->connection)
        return;

    if(!job || job->error()) {
	kError(65432) << "Error reading image results from Yahoo!\n";
	kError(65432) << d->connection->errorString() << endl;
	return;
    }

    kDebug(65432) << "Checking for data!!\n";
    if(d->connection->data().isEmpty()) {
        kError(65432) << "Yahoo image search returned an empty result!\n";
        return;
    }

    QDomDocument results("ResultSet");

    QString errorStr;
    int errorCol, errorLine;
    if(!results.setContent(d->connection->data(), &errorStr, &errorLine, &errorCol)) {
	kError(65432) << "Unable to create XML document from Yahoo results.\n";
	kError(65432) << "Line " << errorLine << ", " << errorStr << endl;

        delete d->connection;
	return;
    }

    delete d->connection;
    QDomNode n = results.documentElement();

    bool hasNoResults = false;

    if(n.isNull()) {
	kDebug(65432) << "No document root in XML results??\n";
	hasNoResults = true;
    }
    else {
	QDomElement result = n.toElement();
	if(result.attribute("totalResultsReturned").toInt() == 0)
	    kDebug(65432) << "Search returned " << result.attribute("totalResultsAvailable") << " results.\n";

	if(result.isNull() || !result.hasAttribute("totalResultsReturned") ||
	    result.attribute("totalResultsReturned").toInt() == 0)
	{
	    hasNoResults = true;
	}
    }

    if(hasNoResults)
    {
	kDebug(65432) << "Search returned no results.\n";
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
	    kError(65432) << "Invalid result returned, skipping.\n";
	    continue;
	}

	d->imageList.append(
	    WebImage(
	        resultUrl.toElement().text(),
		thumbnail.namedItem("Url").toElement().text(),
		width.toElement().text().toInt(),
		height.toElement().text().toInt()
	    )
	);
    }

    // Have results, show them and pick one.

    if(!d->dialog) {
        d->dialog = new WebImageFetcherDialog(d->imageList, d->file, 0);
	d->dialog->setModal(true);

	connect(d->dialog, SIGNAL(coverSelected()), SLOT(slotCoverChosen()));
	connect(d->dialog, SIGNAL(newSearchRequested()), SLOT(slotNewSearch()));
    }

    d->dialog->refreshScreen(d->imageList);
    d->dialog->show();
}

void WebImageFetcher::slotCoverChosen()
{
    QPixmap pixmap = d->dialog->result();
    if(pixmap.isNull()) {
	kError(65432) << "Selected pixmap is null for some reason.\n";
	return;
    }

    kDebug(65432) << "Adding new cover for " << d->file.tag()->fileName();
    coverKey newId = CoverManager::addCover(pixmap, d->file.tag()->artist(), d->file.tag()->album());
    emit signalCoverChanged(newId);
}

void WebImageFetcher::slotNewSearch()
{
    requestNewSearchTerms();
}

void WebImageFetcher::displayWaitMessage()
{
    KStatusBar *statusBar = JuK::JuKInstance()->statusBar();
    statusBar->showMessage(i18n("Searching for Images. Please Wait..."));
    slotLoadImageURLs();
    statusBar->clearMessage();
}

void WebImageFetcher::requestNewSearchTerms(bool noResults)
{
    bool ok;
    QString search = KInputDialog::getText(i18n("Cover Downloader"),
                                           noResults ?
                                             i18n("No matching images found, please enter new search terms:") :
                                             i18n("Enter new search terms:"),
                                           d->searchString, &ok);
    if(ok && !search.isEmpty()) {
        d->searchString = search;
        displayWaitMessage(); // This kicks off the new search
    }
}

#include "webimagefetcher.moc"

// vim: set et sw=4 tw=0 sta:
