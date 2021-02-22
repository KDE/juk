/**
 * Copyright (C) 2002-2004 Scott Wheeler <wheeler@kde.org>
 *           (portions copied from playlist.cpp)
 * Copyright (C) 2018 Michael Pyne <mpyne@kde.org>
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

#include "playlistsharedsettings.h"

#include <QHeaderView>
#include <QTreeWidget>
#include <QAction>

#include <KConfigGroup>
#include <KSharedConfig>
#include <KToggleAction>

#include "actioncollection.h"
#include "playlistitem.h"

////////////////////////////////////////////////////////////////////////////////
// Playlist::SharedSettings public members
////////////////////////////////////////////////////////////////////////////////

Playlist::SharedSettings *Playlist::SharedSettings::instance()
{
    static SharedSettings settings;
    return &settings;
}

void Playlist::SharedSettings::setColumnOrder(const Playlist *l)
{
    if(!l)
        return;

    m_columnOrder.clear();

    for(int i = l->columnOffset(); i < l->columnCount(); ++i)
        m_columnOrder.append(l->header()->logicalIndex(i));

    writeConfig();
}

void Playlist::SharedSettings::toggleColumnVisible(int column)
{
    if(column >= m_columnsVisible.size())
        m_columnsVisible.resize(column + 1);

    m_columnsVisible[column] = !m_columnsVisible[column];

    writeConfig();
}

bool Playlist::SharedSettings::isColumnVisible(int column) const
{
    if(column >= m_columnsVisible.size()) {
        return false;
    }

    return m_columnsVisible[column];
}

void Playlist::SharedSettings::apply(Playlist *l) const
{
    if(!l)
        return;

    const int offset = l->columnOffset();
    auto header = l->header();
    int i = 0;

    for(int logicalIndex : m_columnOrder) {
        header->moveSection(header->visualIndex(logicalIndex), i + offset);
        i++;
    }

    for(int j = 0; j < m_columnsVisible.size(); j++) {
        if(m_columnsVisible[j] && l->isColumnHidden(j + offset))
            l->showColumn(j + offset, false);
        else if(!m_columnsVisible[j] && !l->isColumnHidden(j + offset))
            l->hideColumn(j + offset, false);
    }

    l->updateLeftColumn();
    l->slotColumnResizeModeChanged();
}

////////////////////////////////////////////////////////////////////////////////
// Playlist::ShareSettings protected members
////////////////////////////////////////////////////////////////////////////////

Playlist::SharedSettings::SharedSettings()
{
    KConfigGroup config(KSharedConfig::openConfig(), "PlaylistShared");

    bool resizeColumnsManually = config.readEntry("ResizeColumnsManually", false);
    ActionCollection::action("resizeColumnsManually")->setChecked(resizeColumnsManually);

    // Preallocate spaces so we don't need to check later.
    m_columnsVisible.fill(true, PlaylistItem::lastColumn() + 1);

    // load column order
    m_columnOrder = config.readEntry("ColumnOrder", QList<int>()).toVector();

    QVector<int> l = config.readEntry("VisibleColumns", QList<int>()).toVector();

    if(l.isEmpty()) {

        // Provide some default values for column visibility if none were
        // read from the configuration file.

        m_columnsVisible[PlaylistItem::BitrateColumn] = false;
        m_columnsVisible[PlaylistItem::CommentColumn] = false;
        m_columnsVisible[PlaylistItem::FileNameColumn] = false;
        m_columnsVisible[PlaylistItem::FullPathColumn] = false;
    }
    else {
        // Convert the int list into a bool list.

        m_columnsVisible.fill(false);
        for(int i : qAsConst(l)) {
            if(Q_LIKELY(i < m_columnsVisible.size()))
                m_columnsVisible[i] = true;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// Playlist::SharedSettings private members
////////////////////////////////////////////////////////////////////////////////

void Playlist::SharedSettings::writeConfig()
{
    KConfigGroup config(KSharedConfig::openConfig(), "PlaylistShared");
    config.writeEntry("ColumnOrder", m_columnOrder.toList());

    QList<int> l;
    for(int i = 0; i < m_columnsVisible.size(); i++)
        if(m_columnsVisible[i])
            l << i;

    config.writeEntry("VisibleColumns", l);
    config.writeEntry("ResizeColumnsManually",
            ActionCollection::action<KToggleAction>("resizeColumnsManually")->isChecked());

    KSharedConfig::openConfig()->sync();
}

// vim: set et sw=4 tw=0 sta:
