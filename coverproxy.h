/**
 * Copyright (C) 2009 Michael Pyne <mpyne@kde.org>
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

#ifndef JUK_COVERPROXY_H
#define JUK_COVERPROXY_H

#include <QObject>

class KJob;

/**
 * This class is responsible for tracking status of KIO::Jobs that are
 * downloading covers for the CoverManager.
 */
class CoverProxy : public QObject
{
    Q_OBJECT

public:
    explicit CoverProxy(QObject *parent = 0);

private slots:
    void handleResult(KJob *);
};

#endif
