/***************************************************************************
    begin                : Sun May 15 2005 
    copyright            : (C) 2005 by Michael Pyne
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

#include <qpixmap.h>
#include <qmap.h>
#include <qstring.h>
#include <qfile.h>
#include <qimage.h>
#include <qdir.h>
#include <qdatastream.h>
#include <qdict.h>

#include <kdebug.h>
#include <kstaticdeleter.h>
#include <kstandarddirs.h>
#include <kglobal.h>

#include "covermanager.h"

// This is a dictionary to map the track path to their ID.  Otherwise we'd have
// to store this info with each CollectionListItem, which would break the cache
// of users who upgrade, and would just generally be a big mess.
typedef QDict<coverKey> TrackLookupMap;

// This is responsible for making sure that the CoverManagerPrivate class
// gets properly destructed on shutdown.
static KStaticDeleter<CoverManagerPrivate> sd;

CoverManagerPrivate *CoverManager::m_data = 0;

// Used to save and load CoverData from a QDataStream
QDataStream &operator<<(QDataStream &out, const CoverData &data);
QDataStream &operator>>(QDataStream &in, CoverData &data);

//
// Implementation of CoverData struct
//

QPixmap CoverData::pixmap() const
{
    if(m_pixmap.isNull())
        m_pixmap = QPixmap(path);

    return m_pixmap;
}

QPixmap CoverData::thumbnail() const
{
    if(!m_thumbnail.isNull())
        return m_thumbnail;

    QPixmap base = pixmap();
    if(base.isNull())
        return QPixmap();

    // Convert to image for smoothScale()
    QImage image = base.convertToImage();
    m_thumbnail.convertFromImage(image.smoothScale(80, 80));

    return m_thumbnail;
}

/**
 * This class is responsible for actually keeping track of the storage for the
 * different covers and such.  It holds the covers, and the map of path names
 * to cover ids, and has a few utility methods to load and save the data.
 *
 * @author Michael Pyne <michael.pyne@kdemail.net>
 * @see CoverManager
 */
class CoverManagerPrivate
{
public:
    CoverManagerPrivate() : m_trackMapping(1301)
    {
        loadCovers();
    }

    ~CoverManagerPrivate()
    {
        saveCovers();
    }

    CoverDataMap &covers() { return m_covers; }
    TrackLookupMap &tracks() { return m_trackMapping; }

    /**
     * Creates the data directory for the covers if it doesn't already exist.
     * Must be in this class for loadCovers() and saveCovers().
     */
    void createDataDir() const;

    /**
     * Returns the next available unused coverKey that can be used for
     * inserting new items.
     *
     * @return unused id that can be used for new CoverData
     */
    coverKey nextId() const;

    private:
    void loadCovers();
    void saveCovers() const;

    /**
     * @return the full path and filename of the file storing the cover
     * lookup map and the translations between pathnames and ids.
     */
    QString coverLocation() const;

    CoverDataMap m_covers;
    TrackLookupMap m_trackMapping;
};

//
// Implementation of CoverManagerPrivate methods.
//
void CoverManagerPrivate::createDataDir() const
{
    QDir dir;
    QString dirPath(QDir::cleanDirPath(coverLocation() + "/.."));
    if(!dir.exists(dirPath))
        KStandardDirs::makeDir(dirPath);
}

void CoverManagerPrivate::saveCovers() const
{
    kdDebug() << k_funcinfo << endl;

    // Make sure the directory exists first.
    createDataDir();

    QFile file(coverLocation());

    kdDebug() << "Opening covers db: " << coverLocation() << endl;

    if(!file.open(IO_WriteOnly)) {
        kdError() << "Unable to save covers to disk!\n";
        return;
    }

    QDataStream out(&file);

    // Write out the version and count
    out << Q_UINT32(0) << Q_UINT32(m_covers.count());

    // Write out the data
    for(CoverDataMap::ConstIterator it = m_covers.begin(); it != m_covers.end(); ++it) {
        out << Q_UINT32(it.key());
        out << *it.data();
    }

    // Now write out the track mapping.
    out << Q_UINT32(m_trackMapping.count());

    QDictIterator<coverKey> trackMapIt(m_trackMapping);
    while(trackMapIt.current()) {
        out << trackMapIt.currentKey() << Q_UINT32(*trackMapIt.current());
        ++trackMapIt;
    }
}

void CoverManagerPrivate::loadCovers()
{
    kdDebug() << k_funcinfo << endl;

    QFile file(coverLocation());

    if(!file.open(IO_ReadOnly)) {
        // Guess we don't have any covers yet.
        return;
    }

    QDataStream in(&file);
    Q_UINT32 count, version;

    // First thing we'll read in will be the version.
    // Only version 0 is defined for now.
    in >> version;
    if(version > 0) {
        kdError() << "Cover database was created by a higher version of JuK,\n";
        kdError() << "I don't know what to do with it.\n";

        return;
    }
    
    // Read in the count next, then the data.
    in >> count;
    for(Q_UINT32 i = 0; i < count; ++i) {
        // Read the id, and 3 QStrings for every 1 of the count.
        Q_UINT32 id;
        CoverDataPtr data(new CoverData);

        in >> id;
        in >> *data;

        m_covers[(coverKey) id] = data;
    }

    in >> count;
    for(Q_UINT32 i = 0; i < count; ++i) {
        QString path;
        Q_UINT32 id;

        in >> path >> id;
        m_trackMapping.insert(path, new coverKey(id));
    }
}

QString CoverManagerPrivate::coverLocation() const
{
    return KGlobal::dirs()->saveLocation("appdata") + "coverdb/covers";
}

// XXX: This could probably use some improvement, I don't like the linear
// search for ID idea.
coverKey CoverManagerPrivate::nextId() const
{
    // Start from 1...
    coverKey key = 1;

    while(m_covers.contains(key))
        ++key;

    return key;
}

//
// Implementation of CoverManager methods.
//
coverKey CoverManager::idFromMetadata(const QString &artist, const QString &album)
{
    // Search for the string, yay!  It might make sense to use a cache here,
    // if so it's not hard to add a QCache.
    CoverDataMap::ConstIterator it = begin();
    CoverDataMap::ConstIterator endIt = end();

    for(; it != endIt; ++it) {
        if(it.data()->album == album.lower() && it.data()->artist == artist.lower())
            return it.key();
    }

    return NoMatch;
}

QPixmap CoverManager::coverFromId(coverKey id, Size size)
{
    CoverDataPtr info = coverInfo(id);

    if(!info)
        return QPixmap();

    if(size == Thumbnail)
        return info->thumbnail();

    return info->pixmap();
}

coverKey CoverManager::addCover(const QPixmap &large, const QString &artist, const QString &album)
{
    kdDebug() << k_funcinfo << endl;

    coverKey id = data()->nextId();
    CoverDataPtr coverData(new CoverData);

    if(large.isNull()) {
        kdDebug() << "The pixmap you're trying to add is NULL!\n";
        return NoMatch;
    }

    // Save it to file first!

    QString ext = QString("/coverdb/coverID-%1.png").arg(id);
    coverData->path = KGlobal::dirs()->saveLocation("appdata") + ext;

    kdDebug() << "Saving pixmap to " << coverData->path << endl;
    data()->createDataDir();

    if(!large.save(coverData->path, "PNG")) {
        kdError() << "Unable to save pixmap to " << coverData->path << endl;
        return NoMatch;
    }

    coverData->setPixmap(large);
    coverData->artist = artist.lower();
    coverData->album = album.lower();

    data()->covers()[id] = coverData;
    if(!data()->covers().contains(id))
        kdError(65432) << "coverData should have: " << id << endl;

    return id;
}

coverKey CoverManager::addCover(const QString &path, const QString &artist, const QString &album)
{
    return addCover(QPixmap(path), artist, album);
}

bool CoverManager::hasCover(coverKey id)
{
    return data()->covers().contains(id);
}

bool CoverManager::removeCover(coverKey id)
{
    if(!hasCover(id))
        return false;

    data()->covers().remove(id);

    QDictIterator<coverKey> it(data()->tracks());

    // Remove references to files that had that track ID.
    for(; it.current(); ++it)
        if(*it.current() == id)
            data()->tracks().remove(it.currentKey());

    return true;
}

bool CoverManager::replaceCover(coverKey id, const QPixmap &large)
{
    if(!hasCover(id))
        return false;

    CoverDataPtr coverData = coverInfo(id);
    coverData->setPixmap(large);

    large.save(coverData->path, "PNG");
    return true;
}

CoverManagerPrivate *CoverManager::data()
{
    if(!m_data)
        sd.setObject(m_data, new CoverManagerPrivate);

    return m_data;
}

void CoverManager::shutdown()
{
    sd.destructObject();
}

CoverDataMap::ConstIterator CoverManager::begin()
{
    return data()->covers().constBegin();
}

CoverDataMap::ConstIterator CoverManager::end()
{
    return data()->covers().constEnd();
}

QValueList<coverKey> CoverManager::keys()
{
    return data()->covers().keys();
}

void CoverManager::setIdForTrack(const QString &path, coverKey id)
{
    if(id == NoMatch)
        data()->tracks().remove(path);
    else
        data()->tracks().insert(path, new coverKey(id));
}

coverKey CoverManager::idForTrack(const QString &path)
{
    coverKey *coverPtr = data()->tracks().find(path);

    if(!coverPtr)
        return NoMatch;

    return *coverPtr;
}

CoverDataPtr CoverManager::coverInfo(coverKey id)
{
    if(data()->covers().contains(id))
        return data()->covers()[id];

    return CoverDataPtr(0);
}

/**
 * Write @p data out to @p out.
 *
 * @param out the data stream to write @p data out to.
 * @param data the CoverData to write out.
 * @return the data stream that the data was written to.
 */
QDataStream &operator<<(QDataStream &out, const CoverData &data)
{
    out << data.artist;
    out << data.album;
    out << data.path;

    return out;
}

/**
 * Read @p data from @p in.
 *
 * @param in the data stream to read from.
 * @param data the CoverData to read into.
 * @return the data stream read from.
 */
QDataStream &operator>>(QDataStream &in, CoverData &data)
{
    in >> data.artist;
    in >> data.album;
    in >> data.path;

    return in;
}

// vim: set et sw=4 ts=4:
