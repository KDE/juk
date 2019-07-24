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
#include <QIcon>
#include <QFrame>
#include <QEvent>
#include <QPushButton>
#include <QStatusBar>

#include "filehandle.h"
#include "playlistinterface.h"
#include "actioncollection.h"
#include "playermanager.h"
#include "juktag.h"
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

StatusLabel::StatusLabel(const PlaylistInterface &currentPlaylist, QStatusBar *parent) :
    QWidget(parent)
{
    m_playlistLabel = new KSqueezedTextLabel(this);
    m_playlistLabel->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred, QSizePolicy::Label));
    m_playlistLabel->setTextFormat(Qt::PlainText);
    m_playlistLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    parent->addWidget(m_playlistLabel, 1);

    m_trackLabel = new QLabel(this);
    m_trackLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_trackLabel->setTextFormat(Qt::PlainText);
    parent->addPermanentWidget(m_trackLabel);

    m_itemTimeLabel = new QLabel(this);
    QFontMetrics fontMetrics(font());
    m_itemTimeLabel->setAlignment(Qt::AlignCenter);
    m_itemTimeLabel->setMinimumWidth(fontMetrics.boundingRect("000:00 / 000:00").width());
    m_itemTimeLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    m_itemTimeLabel->setFrameStyle(QFrame::Box | QFrame::Sunken);
    m_itemTimeLabel->installEventFilter(this);
    parent->addPermanentWidget(m_itemTimeLabel);

    setItemTotalTime(0);
    setItemCurrentTime(0);

    QPushButton *jumpButton = new QPushButton(this);
    jumpButton->setIcon(QIcon::fromTheme("go-jump"));
    jumpButton->setFlat(true);

    jumpButton->setToolTip(i18n("Jump to the currently playing item"));
    connect(jumpButton, &QPushButton::clicked, action("showPlaying"), &QAction::trigger);

    parent->addPermanentWidget(jumpButton);
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
