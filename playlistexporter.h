/**
 * Copyright (C) 2004 Michael Pyne <mpyne@kde.org>
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

#ifndef PLAYLISTEXPORTER_H
#define PLAYLISTEXPORTER_H

#include <QtCore/QObject>

class KAction;

/**
 * Abstract base class to define an interface for classes that export
 * PlaylistItem data.
 *
 * @author Michael Pyne <mpyne@kde.org>
 * @see K3bExporter
 */
class PlaylistExporter : public QObject
{
    Q_OBJECT

public:
    PlaylistExporter(QObject *parent = 0) : QObject(parent) { }
    virtual ~PlaylistExporter() { }

    /**
     * Returns a KAction that can be used to invoke the export.
     * Returns 0 if it is not possible.
     *
     * @return pointer to a KAction that can invoke the export, or 0 on
     *         failure.
     */
    virtual KAction *action() = 0;
};

#endif /* PLAYLISTEXPORTER_H */

// vim: set et sw=4 tw=0 sta:
