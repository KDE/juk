/***************************************************************************
    copyright            : (C) 2004 by Scott Wheeler
    email                : wheeler@kde.org
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it  under the terms of the GNU Lesser General Public License version  *
 *   2.1 as published by the Free Software Foundation.                     *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
 *   USA                                                                   *
 ***************************************************************************/

/*
 * At some point this will likely be a library class, as such it's been written
 * as such and is LGPL'ed.
 */

#ifndef KTRM_H
#define KTRM_H

#include <config.h>

#if HAVE_MUSICBRAINZ

#include <qstring.h>
#include <qvaluelist.h>
#include <qmap.h>

/**
 * This struct represents the results of a TRM lookup and MusicBrainz
 * identification.
 */

struct KTRMResult
{
    KTRMResult() : track(0), year(0), relevance(0) {}

    QString title;
    QString artist;
    QString album;
    int track;
    int year;
    int relevance;

    bool operator<(const KTRMResult &r) const
    {
	return r.relevance < relevance;
    }

    bool isEmpty()
    {
        return title.isEmpty() && artist.isEmpty() && album.isEmpty() &&
            track == 0 && year == 0;
    }
};

typedef QValueList<KTRMResult> KTRMResultList;

class KTRMLookup
{
public:
    KTRMLookup(const QString &file, bool autoDelete = false);
    virtual ~KTRMLookup();

    QString file() const;

    int fileId() const;

    /**
     * This method is called if the track was recognized by the TRM server.
     * results() will return just one value.  This may be reimplemented to
     * provide specific behavion in the case of the track being recognized.
     */
    virtual void recognized();

    /**
     * This method is called if the track was not recognized by the TRM server.
     * results() will return an empty set.  This may be reimplemented to provide
     * specific behavion in the case of the track not being recognized.
     */
    virtual void unrecognized();

    /**
     * This method is called if there are multiple potential matches for the TRM
     * value.  results() will return a list of the potential matches, sorted by
     * liklihood.  This may be reimplemented to provide
     * specific behavion in the case of the track not being recognized.
     */
    virtual void collision();

    /**
     * This method is called if the track was not recognized by the TRM server.
     * results() will return an empty set.  This may be reimplemented to provide
     * specific behavion in the case of the track not being recognized.
     */
    virtual void error();

    /**
     * Returns the list of matches found by the lookup.  In the case that there
     * was a TRM collision this list will contain multiple entries.  In the case
     * that it was recognized this will only contain one entry.  Otherwise it
     * will remain empty.
     */
    KTRMResultList results() const;

protected:
    /**
     * This method is called when any of terminal states (recognized,
     * unrecognized, collision or error) has been reached after the specifc
     * method for the result has been called.
     *
     * This should be reimplemented in the case that there is some general
     * processing to be done for all terminal states.
     */
    virtual void finished();

private:
    class KTRMLookupPrivate;
    KTRMLookupPrivate *d;
};

#endif
#endif
