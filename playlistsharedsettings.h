/**
 * Copyright (C) 2002-2004 Scott Wheeler <wheeler@kde.org>
 * Copyright (C) 2007 Michael Pyne <mpyne@kde.org>
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

#ifndef JUK_PLAYLISTSHAREDSETTINGS_H
#define JUK_PLAYLISTSHAREDSETTINGS_H

#include <QVector>

#include "playlist.h"

/**
 * Shared settings between the playlists.
 */

class Playlist::SharedSettings
{
public:
    static SharedSettings *instance();

    /**
     * Sets the default column order to that of Playlist @param p.
     */
    void setColumnOrder(const Playlist *l);
    void toggleColumnVisible(int column);
    bool isColumnVisible(int column) const;

    /**
     * Apply the settings.
     */
    void apply(Playlist *l) const;
    void sync() { writeConfig(); }

protected:
    SharedSettings();
    ~SharedSettings() {}

private:
    void writeConfig();

    QVector<int> m_columnOrder;
    QVector<bool> m_columnsVisible;
};


#endif

// vim: set et sw=4 tw=0 sta:
