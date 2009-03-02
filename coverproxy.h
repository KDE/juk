/***************************************************************************
    begin                : Sun Mar 01 2009
    copyright            : (C) 2009 by Michael Pyne
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

#ifndef JUK_COVERPROXY_H
#define JUK_COVERPROXY_H

#include <QtCore/QObject>

class KJob;

/**
 * This class is responsible for tracking status of KIO::Jobs that are
 * downloading covers for the CoverManager.
 */
class CoverProxy : public QObject
{
    Q_OBJECT

public:
    CoverProxy(QObject *parent = 0);

private slots:
    void handleResult(KJob *);
};

#endif
