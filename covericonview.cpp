/**
 * Copyright (C) 2005 Michael Pyne <mpyne@kde.org>
 * Copyright (C) 2014 Arnold Dumas <contact@arnolddumas.fr>
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

#include "covericonview.h"
#include "covermanager.h"

using CoverUtility::CoverIconViewItem;

CoverIconViewItem::CoverIconViewItem(coverKey id, QListWidget *parent) :
    QListWidgetItem(parent), m_id(id)
{
    const auto &data = CoverManager::coverInfo(id);
    setText(QString("%1 - %2").arg(data.artist, data.album));
    setIcon(data.thumbnail());
    setSizeHint(QSize(140, 150));
}

CoverIconView::CoverIconView(QWidget *parent, const char *name) : QListWidget(parent)
{
    setObjectName(QLatin1String(name));
    setResizeMode(QListWidget::Adjust);
    setViewMode(QListWidget::IconMode);
    setIconSize(QSize(130, 140)); // FIXME: HiDPI
    setMovement(QListWidget::Static);
    setContextMenuPolicy(Qt::CustomContextMenu);
}

CoverIconViewItem *CoverIconView::currentItem() const
{
    return static_cast<CoverIconViewItem *>(QListWidget::currentItem());
}

// vim: set et sw=4 tw=0 sta:
