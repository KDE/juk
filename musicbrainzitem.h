/***************************************************************************
                          musicbrainzitem.h  -  description
                             -------------------
    begin                : Thur Sep 04 2003
    copyright            : (C) 2003 by Adam Treat
    email                : manyoso@yahoo.com
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef MUSICBRAINZITEM_H
#define MUSICBRAINZITEM_H

#include <klistview.h>

#include <qobject.h>
#include <qptrstack.h>
#include <qvaluevector.h>

#include "musicbrainzquery.h"

/**
 * Items for the MusicBrainz queries.
 */

class MusicBrainzItem : public QObject, public KListViewItem
{
    Q_OBJECT

public:
    MusicBrainzItem( KListView* parent, MusicBrainzQuery::Track track, const QString &name, const QString &artist, const QString &album );
    MusicBrainzQuery::Track m_track;

public slots:

protected:

    virtual ~MusicBrainzItem();

protected slots:

signals:

private:
};

#endif
