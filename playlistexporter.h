/***************************************************************************
    begin                : Tue Jun 1 2004
    copyright            : (C) 2004 by Michael Pyne
    email                : michael.pyne@kdemail.net
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PLAYLISTEXPORTER_H
#define PLAYLISTEXPORTER_H



class KAction;
class KActionCollection;

/** 
 * Abstract base class to define an interface for classes that export
 * PlaylistItem data.
 *
 * @author Michael Pyne <michael.pyne@kdemail.net>
 * @see K3bExporter
 */
class PlaylistExporter : public QObject
{
public:
    PlaylistExporter(QWidget *parent = 0) : QObject(parent) { }
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

// vim: set et ts=4 sw=4:
