/**
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

#include "lyricswidget.h"

#include <QDomDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QIcon>
#include <QAction>
#include <QUrlQuery>
#include <KLocalizedString>
#include <KActionCollection>
#include <KToggleAction>
#include <KConfigGroup>
#include <KSharedConfig>

#include "actioncollection.h"
#include "iconsupport.h"
#include "juk_debug.h"
#include "juktag.h"

static const QUrl BASE_LYRICS_URL = QUrl("https://lyrics.fandom.com/");

using namespace IconSupport; // ""_icon
using namespace ActionCollection; // ""_act

LyricsWidget::LyricsWidget(QWidget* parent)
  : QTextBrowser(parent)
  , m_networkAccessManager(new QNetworkAccessManager)
  , m_lyricsCurrent(false)
{
    setMinimumWidth(200);
    setReadOnly(true);
    setWordWrapMode(QTextOption::WordWrap);
    setOpenExternalLinks(true);

    KToggleAction *show = new KToggleAction("view-media-lyrics"_icon,
                                            i18n("Show &Lyrics"), this);
    ActionCollection::actions()->addAction("showLyrics", show);
    connect(show, SIGNAL(toggled(bool)), this, SLOT(setVisible(bool)));

    KConfigGroup config(KSharedConfig::openConfig(), "LyricsWidget");
    bool shown = config.readEntry("Show", true);
    show->setChecked(shown);
    setVisible(shown);
}

LyricsWidget::~LyricsWidget()
{
    delete m_networkAccessManager;
    saveConfig();
}

void LyricsWidget::saveConfig()
{
    KConfigGroup config(KSharedConfig::openConfig(), "LyricsWidget");
    config.writeEntry("Show", "showLyrics"_act->isChecked());
}

void LyricsWidget::makeLyricsRequest()
{
    m_lyricsCurrent = true;

    if(m_playingFile.isNull()) {
        setHtml(i18n("<i>No file playing.</i>"));
        return;
    }

    m_title = m_playingFile.tag()->artist() + " &#8211; " + m_playingFile.tag()->title();
    setHtml(i18n("<i>Loading...</i>"));

    // lyrics.fandom.com seems to be unavailable as well now so give a better error message
    // and stop pinging the server
    if(1) {
        setHtml(i18n("<i>Lyrics are currently unavailable for %1 (no available lyrics provider)</i>").arg(m_title));
        return;
    }

    QUrl listUrl = BASE_LYRICS_URL;
    listUrl.setPath("/api.php");

    QUrlQuery listUrlQuery;
    listUrlQuery.addQueryItem("action", "lyrics");
    listUrlQuery.addQueryItem("func", "getSong");
    listUrlQuery.addQueryItem("fmt", "xml");
    listUrlQuery.addQueryItem("artist", m_playingFile.tag()->artist());
    listUrlQuery.addQueryItem("song", m_playingFile.tag()->title());
    listUrl.setQuery(listUrlQuery);

    connect(m_networkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(receiveListReply(QNetworkReply*)));
    m_networkAccessManager->get(QNetworkRequest(listUrl));
}

void LyricsWidget::playing(const FileHandle &file)
{
    m_playingFile = file;
    m_lyricsCurrent = false;

    if(isVisible()) {
        makeLyricsRequest();
    }
}

void LyricsWidget::showEvent(QShowEvent *)
{
    if(!m_lyricsCurrent) {
        makeLyricsRequest();
    }
}

void LyricsWidget::receiveListReply(QNetworkReply* reply)
{
    disconnect(m_networkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(receiveListReply(QNetworkReply*)));
    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(JUK_LOG) << "Error while fetching lyrics: " << reply->errorString();
        setHtml(i18n("<span style='color:red'>Error while retrieving lyrics!</span>"));
        return;
    }

    QDomDocument document;
    document.setContent(reply);
    QString artist = document.elementsByTagName("artist").at(0).toElement().text();
    QString title = document.elementsByTagName("song").at(0).toElement().text();

    QUrl url = BASE_LYRICS_URL;
    url.setPath("/api.php");

    QUrlQuery urlQuery;
    urlQuery.addQueryItem("action", "query");
    urlQuery.addQueryItem("prop", "revisions");
    urlQuery.addQueryItem("rvprop", "content");
    urlQuery.addQueryItem("format", "xml");
    urlQuery.addQueryItem("titles", artist + ':' + title);
    url.setQuery(urlQuery);

    connect(m_networkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(receiveLyricsReply(QNetworkReply*)));
    m_networkAccessManager->get(QNetworkRequest(url));
}

void LyricsWidget::receiveLyricsReply(QNetworkReply* reply)
{
    disconnect(m_networkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(receiveLyricsReply(QNetworkReply*)));
    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(JUK_LOG) << "Error while fetching lyrics: " << reply->errorString();
        setHtml(i18n("<span style='color:red'>Error while retrieving lyrics!</span>"));
        return;
    }
    const QUrlQuery replyUrlQuery(reply->url());
    QString titlesUrlPart = replyUrlQuery.queryItemValue(QStringLiteral("titles"), QUrl::FullyEncoded);
    if (titlesUrlPart.isEmpty()) {
        // default homepage, but this code path should never happen at this point.
        titlesUrlPart = QStringLiteral("Lyrics_Wiki");
    }
    const QString lyricsUrl = BASE_LYRICS_URL.toString() + QStringLiteral("wiki/") + titlesUrlPart;

    QString content = QString::fromUtf8(reply->readAll());
    int lIndex = content.indexOf("&lt;lyrics&gt;");
    int rIndex = content.indexOf("&lt;/lyrics&gt;");
    if (lIndex == -1 || rIndex == -1) {
        qCWarning(JUK_LOG) << Q_FUNC_INFO << "Unable to find lyrics in text";
        setText(i18n("No lyrics available."));
        return;
    }
    lIndex += 15; // We skip the tag
    content = content.mid(lIndex, rIndex - lIndex).trimmed();
    content.replace('\n', "<br />");
    //setText(content);
    setHtml("<h1>" + m_title + "</h1>" +
            content +
            i18n("<br /><br /><i>Lyrics provided by <a href='%1'>LyricWiki</a></i>", lyricsUrl));
}
