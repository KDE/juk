/**
 * Copyright (C) 2004 Nathan Toone <nathan@toonetown.com>
 * Copyright (C) 2007, 2017 Michael Pyne <mpyne@kde.org>
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

#include <KLocalizedString>
#include <KIO/Job>
#include <KMessageBox>

#include "covermanager.h"
#include "filehandle.h"
#include "juktag.h"
#include "juk.h"
#include "juk_debug.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QDomDocument>
#include <QDomElement>
#include <QLabel>
#include <QLayout>
#include <QPixmap>
#include <QPointer>
#include <QPushButton>
#include <QStatusBar>
#include <QUrl>
#include <QUrlQuery>

class WebImageFetcher::Private
{
    friend class WebImageFetcher;

    FileHandle file;
    QString artist;
    QString albumName;
    QPointer<KIO::StoredTransferJob> connection;
    QDialog *dialog = nullptr;
    QUrl url;
};

WebImageFetcher::WebImageFetcher(QObject *parent)
  : QObject(parent)
  , d(new Private)
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

    QUrlQuery urlQuery;
    urlQuery.addQueryItem("method", "album.getInfo");
    urlQuery.addQueryItem("api_key", "3e6ecbd7284883089e8f2b5b53b0aecd");
    urlQuery.addQueryItem("artist", d->artist);
    urlQuery.addQueryItem("album", d->albumName);

    QUrl url("http://ws.audioscrobbler.com/2.0/");
    url.setQuery(urlQuery);

    qCDebug(JUK_LOG) << "Using request " << url.toDisplayString();

    d->connection = KIO::storedGet(url, KIO::Reload /* reload always */, KIO::HideProgressInfo);
    connect(d->connection, SIGNAL(result(KJob*)), SLOT(slotWebRequestFinished(KJob*)));

    // Wait for the results...
}

void WebImageFetcher::slotWebRequestFinished(KJob *job)
{
    if (job != d->connection)
        return;

    QStatusBar *statusBar = JuK::JuKInstance()->statusBar();

    if (!job || job->error()) {
        qCCritical(JUK_LOG) << "Error reading image results from last.fm!\n";
        qCCritical(JUK_LOG) << d->connection->errorString();
        return;
    }

    if (d->connection->data().isEmpty()) {
        qCCritical(JUK_LOG) << "last.fm returned an empty result!\n";
        return;
    }

    QDomDocument results("ResultSet");

    QString errorStr;
    int errorCol, errorLine;
    if (!results.setContent(d->connection->data(), &errorStr, &errorLine, &errorCol)) {
        qCCritical(JUK_LOG) << "Unable to create XML document from results.\n";
        qCCritical(JUK_LOG) << "Line " << errorLine << ", " << errorStr;

        return;
    }

    QDomElement n = results.documentElement();

    if (n.isNull()) {
        qCDebug(JUK_LOG) << "No document root in XML results??\n";
        return;
    }
    if (n.nodeName() != QLatin1String("lfm")) {
        qCDebug(JUK_LOG) << "Invalid resulting XML document, not <lfm>";
        return;
    }
    if (n.attribute(QStringLiteral("status")) != QLatin1String("ok")) {
        const QDomElement err = n.firstChildElement(QStringLiteral("error"));
        const int errCode = err.attribute(QStringLiteral("code")).toInt();
        if (errCode == 6) {
            KMessageBox::information(nullptr, i18n("Album '%1' not found.", d->albumName), i18nc("@title:window", "Album not Found"));
        } else {
            KMessageBox::error(nullptr, i18n("Error %1 when searching for cover:\n%2", errCode, err.text()));
        }
        statusBar->clearMessage();
        return;
    }
    n = n.firstChildElement("album");

    //FIXME: We assume they have a sane sorting (smallest -> largest)
    const QString imageUrl = n.lastChildElement("image").text();
    if (imageUrl.isEmpty()) {
        KMessageBox::information(nullptr, i18n("No available cover for the album '%1'.", d->albumName),
                i18nc("@title:window", "Cover not Available"));
        statusBar->clearMessage();
        return;
    }
    d->url = QUrl::fromEncoded(imageUrl.toLatin1());
    //TODO: size attribute can have the values mega, extralarge, large, medium and small

    qCDebug(JUK_LOG) << "Got cover:" << d->url;

    statusBar->showMessage(i18n("Downloading cover. Please Wait..."));

    KIO::StoredTransferJob *newJob = KIO::storedGet(d->url, KIO::Reload /* reload always */, KIO::HideProgressInfo);
    connect(newJob, SIGNAL(result(KJob*)), SLOT(slotImageFetched(KJob*)));
}

void WebImageFetcher::slotImageFetched(KJob* j)
{
    QStatusBar *statusBar = JuK::JuKInstance()->statusBar();
    statusBar->clearMessage();

    KIO::StoredTransferJob *job = qobject_cast<KIO::StoredTransferJob*>(j);

    if (d->dialog)
        return;

    d->dialog = new QDialog;
    d->dialog->setWindowTitle(i18n("Cover found"));

    auto dlgVLayout = new QVBoxLayout(d->dialog);

    if(job->error()) {
        qCCritical(JUK_LOG) << "Unable to grab image" << job->errorText();

        KMessageBox::error(nullptr, i18n("Failed to download requested cover art: %1", job->errorString()),
                i18nc("@title:window", "Could not download cover art"));
        return;
    }

    // TODO: 150x150 seems inconsistent with HiDPI, figure out something better
    QPixmap iconImage, realImage(150, 150);
    iconImage.loadFromData(job->data());
    realImage.fill(Qt::transparent);

    if(iconImage.isNull()) {
        qCCritical(JUK_LOG) << "Thumbnail image is not of a supported format\n";
        return;
    }

    // Scale down if necesssary
    if(iconImage.width() > 150 || iconImage.height() > 150)
        iconImage = iconImage.scaled(150, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QLabel *cover = new QLabel(d->dialog);
    cover->setPixmap(iconImage);
    dlgVLayout->addWidget(cover);

    QLabel *infoLabel = new QLabel(i18n("Cover fetched from <a href='https://last.fm/'>last.fm</a>."), d->dialog);
    infoLabel->setOpenExternalLinks(true);
    infoLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    dlgVLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));
    dlgVLayout->addWidget(infoLabel);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, d->dialog);
    dlgVLayout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, d->dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, d->dialog, &QDialog::reject);

    connect(d->dialog, &QDialog::accepted, this, &WebImageFetcher::slotCoverChosen);
    connect(d->dialog, &QDialog::rejected, this, &WebImageFetcher::destroyDialog);

    d->dialog->setWindowIcon(realImage);
    d->dialog->show();
}


void WebImageFetcher::slotCoverChosen()
{
    qCDebug(JUK_LOG) << "Adding new cover for " << d->file.tag()->fileName()
    << "from URL" << d->url;

    coverKey newId = CoverManager::addCover(d->url, d->file.tag()->artist(), d->file.tag()->album());

    if (newId != CoverManager::NoMatch) {
        emit signalCoverChanged(newId);
        destroyDialog();
    }
}

void WebImageFetcher::destroyDialog()
{
    if (!d->dialog)
        return;

    d->dialog->close();
    d->dialog->deleteLater();
    d->dialog = 0;
}

// vim: set et sw=4 tw=0 sta:
