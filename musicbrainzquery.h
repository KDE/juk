/***************************************************************************
    begin                : Tue Aug 3 2004
    copyright            : (C) 2004 by Scott Wheeler
    email                : wheeler@kde.org
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef MUSICBRAINZQUERY_H
#define MUSICBRAINZQUERY_H

#include <config.h>

#if HAVE_MUSICBRAINZ

#include "ktrm.h"
#include "filehandle.h"

class MusicBrainzLookup : public KTRMLookup
{
public:
    MusicBrainzLookup(const FileHandle &file);
    virtual void recognized();
    virtual void unrecognized();
    virtual void collision();
    virtual void error();

private:
    void message(const QString &s) const;
    void confirmation();

    FileHandle m_file;
};

#endif
#endif
