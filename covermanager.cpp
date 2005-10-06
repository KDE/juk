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
#include <qcache.h>
#include <qmime.h>
#include <qbuffer.h>

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

const char *CoverDrag::mimetype = "application/x-juk-coverid";
// Caches the QPixmaps for the covers so that the covers are not all kept in
// memory for no reason.
typedef QCache<QPixmap> CoverPixmapCache;

CoverManagerPrivate *CoverManager::m_data = 0;

// Used to save and load CoverData from a QDataStream
QDataStream &operator<<(QDataStream &out, const CoverData &data);
QDataStream &operator>>(QDataStream &in, CoverData &data);

//
// Implementation of CoverData struct
//

QPixmap CoverData::pixmap() const
{
    return CoverManager::coverFromData(*this, CoverManager::FullSize);
}

QPixmap CoverData::thumbnail() const
{
    return CoverManager::coverFromData(*this, CoverManager::Thumbnail);
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

    /// Maps coverKey id's to CoverDataPtrs
    CoverDataMap covers;

    /// Maps file names to coverKey id's.
    TrackLookupMap tracks;

    /// A cache of the cover representations.  The key format is:
    /// 'f' followed by the pathname for FullSize covers, and
    /// 't' followed by the pathname for Thumbnail covers.
    CoverPixmapCache pixmapCache;

    CoverManagerPrivate() : tracks(1301), pixmapCache(2 * 1024 * 768)
    {
        loadCovers();
        pixmapCache.setAutoDelete(true);
    }

    ~CoverManagerPrivate()
    {
        saveCovers();
    }

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

    void saveCovers() const;

    private:
    void loadCovers();

    /**
     * @return the full path and filename of the file storing the cover
     * lookup map and the translations between pathnames and ids.
     */
    QString coverLocation() const;
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
    out << Q_UINT32(0) << Q_UINT32(covers.count());

    // Write out the data
    for(CoverDataMap::ConstIterator it = covers.begin(); it != covers.end(); ++it) {
        out << Q_UINT32(it.key());
        out << *it.data();
    }

    // Now write out the track mapping.
    out << Q_UINT32(tracks.count());

    QDictIterator<coverKey> trackMapIt(tracks);
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
        data->refCount = 0;

        covers[(coverKey) id] = data;
    }

    in >> count;
    for(Q_UINT32 i = 0; i < count; ++i) {
        QString path;
        Q_UINT32 id;

        in >> path >> id;

        // If we somehow already managed to load a cover id with this path,
        // don't do so again.  Possible due to a coding error during 3.5
        // development.

        if(!tracks.find(path)) {
            ++covers[(coverKey) id]->refCount; // Another track using this.
            tracks.insert(path, new coverKey(id));
        }
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

    while(covers.contains(key))
        ++key;

    return key;
}

//
// Implementation of CoverDrag
//
CoverDrag::CoverDrag(coverKey id, QWidget *src) : QDragObject(src, "coverDrag"),
                                                  m_id(id)
{
    QPixmap cover = CoverManager::coverFromId(id);
    if(!cover.isNull())
        setPixmap(cover);
}

const char *CoverDrag::format(int i) const
{
    if(i == 0)
        return mimetype;
    if(i == 1)
        return "image/png";

    return 0;
}

QByteArray CoverDrag::encodedData(const char *mimetype) const
{
    if(qstrcmp(CoverDrag::mimetype, mimetype) == 0) {
        QByteArray data;
        QDataStream ds(data, IO_WriteOnly);

        ds << Q_UINT32(m_id);
        return data;
    }
    else if(qstrcmp(mimetype, "image/png") == 0) {
        QPixmap large = CoverManager::coverFromId(m_id, CoverManager::FullSize);
        QImage img = large.convertToImage();
        QByteArray data;
        QBuffer buffer(data);

        buffer.open(IO_WriteOnly);
        img.save(&buffer, "PNG"); // Write in PNG format.

        return data;
    }

    return QByteArray();
}

bool CoverDrag::canDecode(const QMimeSource *e)
{
    return e->provides(mimetype);
}

bool CoverDrag::decode(const QMimeSource *e, coverKey &id)
{
    if(!canDecode(e))
        return false;

    QByteArray data = e->encodedData(mimetype);
    QDataStream ds(data, IO_ReadOnly);
    Q_UINT32 i;

    ds >> i;
    id = (coverKey) i;

    return true;
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

QPixmap CoverManager::coverFromData(const CoverData &coverData, Size size)
{
    QString path = coverData.path;

    // Prepend a tag to the path to separate in the cache between full size
    // and thumbnail pixmaps.  If we add a different kind of pixmap in the
    // future we also need to add a tag letter for it.
    if(size == FullSize)
        path.prepend('f');
    else
        path.prepend('t');

    // Check in cache for the pixmap.
    QPixmap *pix = data()->pixmapCache[path];
    if(pix) {
        kdDebug(65432) << "Found pixmap in cover cache.\n";
        return *pix;
    }

    // Not in cache, load it and add it.
    pix = new QPixmap(coverData.path);
    if(pix->isNull())
        return QPixmap();

    if(size == Thumbnail) {
        // Convert to image for smoothScale()
        QImage image = pix->convertToImage();
        pix->convertFromImage(image.smoothScale(80, 80, QImage::ScaleMin));
    }

    QPixmap returnValue = *pix; // Save it early.
    if(!data()->pixmapCache.insert(path, pix, pix->height() * pix->width()))
        delete pix;

    return returnValue;
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

    coverData->artist = artist.lower();
    coverData->album = album.lower();
    coverData->refCount = 0;

    data()->covers[id] = coverData;

    // Make sure the new cover isn't inadvertently cached.
    data()->pixmapCache.remove(QString("f%1").arg(coverData->path));
    data()->pixmapCache.remove(QString("t%1").arg(coverData->path));

    return id;
}

coverKey CoverManager::addCover(const QString &path, const QString &artist, const QString &album)
{
    return addCover(QPixmap(path), artist, album);
}

bool CoverManager::hasCover(coverKey id)
{
    return data()->covers.contains(id);
}

bool CoverManager::removeCover(coverKey id)
{
    if(!hasCover(id))
        return false;

    // Remove cover from cache.
    CoverDataPtr coverData = coverInfo(id);
    data()->pixmapCache.remove(QString("f%1").arg(coverData->path));
    data()->pixmapCache.remove(QString("t%1").arg(coverData->path));

    // Remove references to files that had that track ID.
    QDictIterator<coverKey> it(data()->tracks);
    for(; it.current(); ++it)
        if(*it.current() == id)
            data()->tracks.remove(it.currentKey());

    // Remove covers from disk.
    QFile::remove(coverData->path);

    // Finally, forget that we ever knew about this cover.
    data()->covers.remove(id);

    return true;
}

bool CoverManager::replaceCover(coverKey id, const QPixmap &large)
{
    if(!hasCover(id))
        return false;

    CoverDataPtr coverData = coverInfo(id);

    // Empty old pixmaps from cache.
    data()->pixmapCache.remove(QString("%1%2").arg("t", coverData->path));
    data()->pixmapCache.remove(QString("%1%2").arg("f", coverData->path));

    large.save(coverData->path, "PNG");
    return true;
}

CoverManagerPrivate *CoverManager::data()
{
    if(!m_data)
        sd.setObject(m_data, new CoverManagerPrivate);

    return m_data;
}

void CoverManager::saveCovers()
{
    data()->saveCovers();
}

void CoverManager::shutdown()
{
    sd.destructObject();
}

CoverDataMap::ConstIterator CoverManager::begin()
{
    return data()->covers.constBegin();
}

CoverDataMap::ConstIterator CoverManager::end()
{
    return data()->covers.constEnd();
}

QValueList<coverKey> CoverManager::keys()
{
    return data()->covers.keys();
}

void CoverManager::setIdForTrack(const QString &path, coverKey id)
{
    coverKey *oldId = data()->tracks.find(path);
    if(oldId && (id == *oldId))
        return; // We're already done.

    if(oldId && *oldId != NoMatch) {
        data()->covers[*oldId]->refCount--;
        data()->tracks.remove(path);

        if(data()->covers[*oldId]->refCount == 0) {
            kdDebug(65432) << "Cover " << *oldId << " is unused, removing.\n";
            removeCover(*oldId);
        }
    }

    if(id != NoMatch) {
        data()->covers[id]->refCount++;
        data()->tracks.insert(path, new coverKey(id));
    }
}

coverKey CoverManager::idForTrack(const QString &path)
{
    coverKey *coverPtr = data()->tracks.find(path);

    if(!coverPtr)
        return NoMatch;

    return *coverPtr;
}

CoverDataPtr CoverManager::coverInfo(coverKey id)
{
    if(data()->covers.contains(id))
        return data()->covers[id];

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
