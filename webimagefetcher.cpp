/**
 * Copyright (C) 2004 Nathan Toone <nathan@toonetown.com>
 * Copyright (C) 2007 Michael Pyne <mpyne@kde.org>
 * Copyright (C) 2012 Martin Sandsmark <martin.sandsmark@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "webimagefetcher.h"

#include <KApplication>
#include <KStatusBar>
#include <KXmlGuiWindow>
#include <KLocale>
#include <KInputDialog>
#include <KUrl>
#include <KDebug>
#include <KIO/Job>
#include <KPushButton>
#include <KDialog>

#include "covermanager.h"
#include "filehandle.h"
#include "tag.h"
#include "juk.h"

#include <QPixmap>
#include <QDomDocument>
#include <QDomElement>
#include <QPointer>
#include <QLayout>
#include <QLabel>
#include <QPainter>

#include <kiconloader.h>


class WebImageFetcher::Private
{
    friend class WebImageFetcher;

    Private() : connection(0), dialog(0)
    {
    }

    FileHandle file;
    QString artist;
    QString albumName;
    QPointer<KIO::StoredTransferJob> connection;
    KDialog *dialog;
    KUrl url;
};

WebImageFetcher::WebImageFetcher(QObject *parent)
        : QObject(parent), d(new Private)
{
}

WebImageFetcher::~WebImageFetcher()
{
    delete d;
}

void WebImageFetcher::setFile(const FileHandle &file)
{
    d->file = file;
    d->artist = file.tag()->artist();
    d->albumName = file.tag()->album();
}

void WebImageFetcher::abortSearch()
{
    if (d->connection)
        d->connection->kill();
}
void WebImageFetcher::searchCover()
{
    QStatusBar *statusBar = JuK::JuKInstance()->statusBar();
    statusBar->showMessage(i18n("Searching for cover. Please Wait..."));


    KUrl url("http://ws.audioscrobbler.com/2.0/");
    url.addQueryItem("method", "album.getInfo");
    url.addQueryItem("api_key", "3e6ecbd7284883089e8f2b5b53b0aecd");
    url.addQueryItem("artist", d->artist);
    url.addQueryItem("album", d->albumName);

    kDebug() << "Using request " << url.encodedPathAndQuery();

    d->connection = KIO::storedGet(url, KIO::Reload /* reload always */, KIO::HideProgressInfo);
    connect(d->connection, SIGNAL(result(KJob*)), SLOT(slotWebRequestFinished(KJob*)));

    // Wait for the results...
}

void WebImageFetcher::slotWebRequestFinished(KJob *job)
{
    kDebug() << "Results received.\n";

    if (job != d->connection)
        return;

    if (!job || job->error()) {
        kError() << "Error reading image results from last.fm!\n";
        kError() << d->connection->errorString() << endl;
        return;
    }

    kDebug() << "Checking for data!!\n";
    if (d->connection->data().isEmpty()) {
        kError() << "last.fm returned an empty result!\n";
        return;
    }

    QDomDocument results("ResultSet");

    QString errorStr;
    int errorCol, errorLine;
    if (!results.setContent(d->connection->data(), &errorStr, &errorLine, &errorCol)) {
        kError() << "Unable to create XML document from results.\n";
        kError() << "Line " << errorLine << ", " << errorStr << endl;

        return;
    }
    
    QDomNode n = results.documentElement();

    if (n.isNull()) {
        kDebug() << "No document root in XML results??\n";
        return;
    }
    n = n.firstChildElement("album");

    d->url = n.lastChildElement("image").text(); //FIXME: We assume they have a sane sorting (smallest -> largest)
    //TODO: size attribute can have the values mega, extralarge, large, medium and small
    
    kDebug() << "Got cover:" << d->url;

    QStatusBar *statusBar = JuK::JuKInstance()->statusBar();
    statusBar->showMessage(i18n("Downloading cover. Please Wait..."));
    
    KIO::StoredTransferJob *newJob = KIO::storedGet(d->url, KIO::Reload /* reload always */, KIO::HideProgressInfo);
    connect(newJob, SIGNAL(result(KJob*)), SLOT(slotImageFetched(KJob*)));
}

void WebImageFetcher::slotImageFetched(KJob* j)
{
    QStatusBar *statusBar = JuK::JuKInstance()->statusBar();
    statusBar->clearMessage();

    KIO::StoredTransferJob *job = qobject_cast<KIO::StoredTransferJob*>(j);
    
    if (d->dialog) return;
    d->dialog = new KDialog();
    d->dialog->setCaption(i18n("Cover found"));
    d->dialog->setButtons(KDialog::Apply | KDialog::Cancel);
    d->dialog->button(KDialog::Apply)->setText(i18n("Store"));
    QWidget *mainWidget = new QWidget();
    d->dialog->setMainWidget(mainWidget);
    mainWidget->setLayout(new QVBoxLayout);
    
    if(job->error()) {
        kError() << "Unable to grab image\n";
        d->dialog->setWindowIcon(DesktopIcon("dialog-error"));
        return;
    }

    QPixmap iconImage, realImage(150, 150);
    iconImage.loadFromData(job->data());
    realImage.fill(Qt::transparent);

    if(iconImage.isNull()) {
        kError() << "Thumbnail image is not of a supported format\n";
        return;
    }

    // Scale down if necesssary
    if(iconImage.width() > 150 || iconImage.height() > 150)
        iconImage = iconImage.scaled(150, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QLabel *cover = new QLabel();
    cover->setPixmap(iconImage);
    mainWidget->layout()->addWidget(cover);
    QLabel *infoLabel = new QLabel(i18n("Cover fetched from <a href='http://last.fm/'>last.fm</a>."));
    infoLabel->setOpenExternalLinks(true);
    infoLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    mainWidget->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));
    mainWidget->layout()->addWidget(infoLabel);
    
    d->dialog->setWindowIcon(realImage);
    d->dialog->show();
    connect(d->dialog, SIGNAL(applyClicked()), SLOT(slotCoverChosen()));
}


void WebImageFetcher::slotCoverChosen()
{
    kDebug() << "Adding new cover for " << d->file.tag()->fileName()
    << "from URL" << d->url;

    coverKey newId = CoverManager::addCover(d->url, d->file.tag()->artist(), d->file.tag()->album());

    if (newId != CoverManager::NoMatch) {
        emit signalCoverChanged(newId);
        d->dialog->close();
        d->dialog->deleteLater();
        d->dialog = 0;
    }
}

#include "webimagefetcher.moc"

// vim: set et sw=4 tw=0 sta:
