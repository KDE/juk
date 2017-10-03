/**
 * Copyright (C) 2002-2004 Scott Wheeler <wheeler@kde.org>
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

#include "statuslabel.h"

#include <KIconLoader>
#include <KSqueezedTextLabel>
#include <KLocalizedString>
#include <KFormat>

#include <QAction>
#include <QMouseEvent>
#include <QLabel>
#include <QFrame>
#include <QHBoxLayout>
#include <QEvent>
#include <QPushButton>

#include "filehandle.h"
#include "playlistinterface.h"
#include "actioncollection.h"
#include "playermanager.h"
#include "tag.h"
#include "juk_debug.h"

using namespace ActionCollection;

////////////////////////////////////////////////////////////////////////////////
// static helpers
////////////////////////////////////////////////////////////////////////////////

static QString formatTime(qint64 milliseconds)
{
    static const KFormat fmt;
    return fmt.formatDuration(milliseconds);
}

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

StatusLabel::StatusLabel(const PlaylistInterface &currentPlaylist, QWidget *parent) :
    QWidget(parent)
{
    auto hboxLayout = new QHBoxLayout(this);

    QFrame *trackAndPlaylist = new QFrame(this);
    hboxLayout->addWidget(trackAndPlaylist);
    trackAndPlaylist->setFrameStyle(QFrame::Box | QFrame::Sunken);
    trackAndPlaylist->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Make sure that we have enough of a margin to suffice for the borders,
    // hence the "lineWidth() * 2"
    QHBoxLayout *trackAndPlaylistLayout = new QHBoxLayout(trackAndPlaylist);
    trackAndPlaylistLayout->setMargin(trackAndPlaylist->lineWidth() * 2);
    trackAndPlaylistLayout->setSpacing(5);
    trackAndPlaylistLayout->setObjectName(QLatin1String("trackAndPlaylistLayout"));
    trackAndPlaylistLayout->addSpacing(5);

    m_playlistLabel = new KSqueezedTextLabel(trackAndPlaylist);
    trackAndPlaylistLayout->addWidget(m_playlistLabel);
    m_playlistLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_playlistLabel->setTextFormat(Qt::PlainText);
    m_playlistLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    m_trackLabel = new KSqueezedTextLabel(trackAndPlaylist);
    trackAndPlaylistLayout->addWidget(m_trackLabel);
    m_trackLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_trackLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_trackLabel->setTextFormat(Qt::PlainText);

    trackAndPlaylistLayout->addSpacing(5);

    m_itemTimeLabel = new QLabel(this);
    hboxLayout->addWidget(m_itemTimeLabel);
    QFontMetrics fontMetrics(font());
    m_itemTimeLabel->setAlignment(Qt::AlignCenter);
    m_itemTimeLabel->setMinimumWidth(fontMetrics.boundingRect("000:00 / 000:00").width());
    m_itemTimeLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    m_itemTimeLabel->setFrameStyle(QFrame::Box | QFrame::Sunken);
    m_itemTimeLabel->installEventFilter(this);

    setItemTotalTime(0);
    setItemCurrentTime(0);

    auto jumpBox = new QFrame(this);
    hboxLayout->addWidget(jumpBox);
    jumpBox->setFrameStyle(QFrame::Box | QFrame::Sunken);
    jumpBox->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);

    auto jumpBoxHLayout = new QHBoxLayout(jumpBox);

    QPushButton *jumpButton = new QPushButton(jumpBox);
    jumpBoxHLayout->addWidget(jumpButton);
    jumpButton->setIcon(SmallIcon("go-up"));
    jumpButton->setFlat(true);

    jumpButton->setToolTip(i18n("Jump to the currently playing item"));
    connect(jumpButton, &QPushButton::clicked, action("showPlaying"), &QAction::trigger);

    installEventFilter(this);

    slotCurrentPlaylistHasChanged(currentPlaylist);
}

void StatusLabel::slotPlayingItemHasChanged(const FileHandle &file)
{
    const Tag *tag = file.tag();
    const QString mid = (tag->artist().isEmpty() || tag->title().isEmpty())
        ? QString()
        : QStringLiteral(" - ");

    setItemTotalTime(tag->seconds());
    setItemCurrentTime(0);

    m_trackLabel->setText(tag->artist() + mid + tag->title());
}

void StatusLabel::slotCurrentPlaylistHasChanged(const PlaylistInterface &currentPlaylist)
{
    if(!currentPlaylist.playing()) {
        return;
    }

    m_playlistLabel->setText(currentPlaylist.name());
    m_trackLabel->setText(
            i18np("1 item", "%1 items", currentPlaylist.count()) +
            QStringLiteral(" - ") +
            formatTime(qint64(1000) * currentPlaylist.time())
            );
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

void StatusLabel::updateTime()
{
    const qint64 milliseconds = m_showTimeRemaining
        ? m_itemTotalTime - m_itemCurrentTime
        : m_itemCurrentTime;
    const QString timeString = formatTime(milliseconds) + QStringLiteral(" / ") +
        formatTime(m_itemTotalTime);

    m_itemTimeLabel->setText(timeString);
}

bool StatusLabel::eventFilter(QObject *o, QEvent *e)
{
    if(!o || !e)
        return false;

    QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(e);
    if(e->type() == QEvent::MouseButtonRelease &&
       mouseEvent->button() == Qt::LeftButton)
    {
        if(o == m_itemTimeLabel) {
            m_showTimeRemaining = !m_showTimeRemaining;
            updateTime();
        }
        else
            action("showPlaying")->trigger();

        return true;
    }
    return false;
}

// vim: set et sw=4 tw=0 sta:
