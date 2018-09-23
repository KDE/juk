/**
 * Copyright (C) 2004 Scott Wheeler <wheeler@kde.org>
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

#ifndef MUSICBRAINZQUERY_H
#define MUSICBRAINZQUERY_H

#include <config-juk.h>

#if HAVE_TUNEPIMP

#include "ktrm.h"
#include "filehandle.h"

class MusicBrainzLookup : public KTRMLookup
{
public:
    explicit MusicBrainzLookup(const FileHandle &file);
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

// vim: set et sw=4 tw=0 sta:
