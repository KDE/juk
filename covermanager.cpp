/***************************************************************************
    begin                : Sun May 15 2005
    copyright            : (C) 2005, 2008 by Michael Pyne
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

#include "covermanager.h"

#include <QPixmap>
#include <QString>
#include <QFile>
#include <QImage>
#include <QDir>
#include <QDataStream>
#include <QHash>
#include <Q3Cache>
#include <QBuffer>
#include <QByteArray>

#include <kdebug.h>
#include <ktemporaryfile.h>
#include <kdemacros.h>
#include <kurl.h>
#include <kstandarddirs.h>
#include <kglobal.h>
#include <kio/netaccess.h>

// This is a dictionary to map the track path to their ID.  Otherwise we'd have
// to store this info with each CollectionListItem, which would break the cache
// of users who upgrade, and would just generally be a big mess.
typedef QHash<QString, coverKey> TrackLookupMap;

static const char dragMimetype[] = "application/x-juk-coverid";

// Caches the QPixmaps for the covers so that the covers are not all kept in
// memory for no reason.
typedef Q3Cache<QPixmap> CoverPixmapCache;

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

    CoverManagerPrivate() : pixmapCache(2 * 1024 * 768)
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

// This is responsible for making sure that the CoverManagerPrivate class
// gets properly destructed on shutdown.
K_GLOBAL_STATIC(CoverManagerPrivate, sd);

//
// Implementation of CoverManagerPrivate methods.
//
void CoverManagerPrivate::createDataDir() const
{
    QDir dir;
    QString dirPath(QDir::cleanPath(coverLocation() + "/.."));
    if(!dir.exists(dirPath))
        KStandardDirs::makeDir(dirPath);
}

void CoverManagerPrivate::saveCovers() const
{
    kDebug() ;

    // Make sure the directory exists first.
    createDataDir();

    QFile file(coverLocation());

    kDebug() << "Opening covers db: " << coverLocation();

    if(!file.open(QIODevice::WriteOnly)) {
        kError() << "Unable to save covers to disk!\n";
        return;
    }

    QDataStream out(&file);

    // Write out the version and count
    out << quint32(0) << quint32(covers.count());

    // Write out the data
    for(CoverDataMap::const_iterator it = covers.begin(); it != covers.end(); ++it) {
        out << quint32(it.key());
        out << *it.value();
    }

    // Now write out the track mapping.
    out << quint32(tracks.count());

    TrackLookupMap::ConstIterator trackMapIt = tracks.constBegin();
    while(trackMapIt != tracks.constEnd()) {
        out << trackMapIt.key() << quint32(trackMapIt.value());
        ++trackMapIt;
    }
}

void CoverManagerPrivate::loadCovers()
{
    kDebug() ;

    QFile file(coverLocation());

    if(!file.open(QIODevice::ReadOnly)) {
        // Guess we don't have any covers yet.
        return;
    }

    QDataStream in(&file);
    quint32 count, version;

    // First thing we'll read in will be the version.
    // Only version 0 is defined for now.
    in >> version;
    if(version > 0) {
        kError() << "Cover database was created by a higher version of JuK,\n";
        kError() << "I don't know what to do with it.\n";

        return;
    }

    // Read in the count next, then the data.
    in >> count;
    for(quint32 i = 0; i < count; ++i) {
        // Read the id, and 3 QStrings for every 1 of the count.
        quint32 id;
        CoverDataPtr data(new CoverData);

        in >> id;
        in >> *data;
        data->refCount = 0;

        covers[(coverKey) id] = data;
    }

    in >> count;
    for(quint32 i = 0; i < count; ++i) {
        QString path;
        quint32 id;

        in >> path >> id;

        // If we somehow already managed to load a cover id with this path,
        // don't do so again.  Possible due to a coding error during 3.5
        // development.

        if(KDE_ISLIKELY(!tracks.contains(path))) {
            ++covers[(coverKey) id]->refCount; // Another track using this.
            tracks.insert(path, id);
        }
    }
}

QString CoverManagerPrivate::coverLocation() const
{
    return KGlobal::dirs()->saveLocation("appdata") + "coverdb/covers";
}

// XXX: This could probably use some improvement, I don't like the linear
// search for ID idea.  Linear search is used instead of covers.size() since we want to
// re-use old IDs if possible.
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
CoverDrag::CoverDrag(coverKey id) :
    QMimeData()
{
    QPixmap cover = CoverManager::coverFromId(id);
    setImageData(cover.toImage());
    setData(dragMimetype, QByteArray::number(qulonglong(id), 10));
}

bool CoverDrag::isCover(const QMimeData *data)
{
    return data->hasImage() || data->hasFormat(dragMimetype);
}

coverKey CoverDrag::idFromData(const QMimeData *data)
{
    bool ok = false;

    if(!data->hasFormat(dragMimetype))
        return CoverManager::NoMatch;

    coverKey id = data->data(dragMimetype).toULong(&ok);
    if(!ok)
        return CoverManager::NoMatch;

    return id;
}

const char *CoverDrag::mimetype()
{
    return dragMimetype;
}

//
// Implementation of CoverManager methods.
//
coverKey CoverManager::idFromMetadata(const QString &artist, const QString &album)
{
    // Search for the string, yay!  It might make sense to use a cache here,
    // if so it's not hard to add a QCache.
    CoverDataMap::const_iterator it = begin();
    CoverDataMap::const_iterator endIt = end();

    for(; it != endIt; ++it) {
        if(it.value()->album == album.toLower() && it.value()->artist == artist.toLower())
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
        kDebug(65432) << "Found pixmap in cover cache.\n";
        return *pix;
    }

    // Not in cache, load it and add it.

    pix = new QPixmap(coverData.path);
    if(pix->isNull())
        return QPixmap();

    if(size == Thumbnail)
        *pix = pix->scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QPixmap returnValue = *pix; // Save it early.

    if(!data()->pixmapCache.insert(path, pix, pix->height() * pix->width()))
        delete pix;

    return returnValue;
}

coverKey CoverManager::addCover(const QPixmap &large, const QString &artist, const QString &album)
{
    kDebug() << "Adding new pixmap to cover database.\n";

    if(large.isNull()) {
        kDebug() << "The pixmap you're trying to add is NULL!\n";
        return NoMatch;
    }

    KTemporaryFile tempFile;
    if(!tempFile.open()) {
        kError() << "Unable to open file for pixmap cover, unable to add cover to DB\n";
        return NoMatch;
    }

    // Now that file is open, file name will be available, which is where we want
    // to save the pixmap as a .png.

    if(!large.save(tempFile.fileName(), "PNG")) {
        kError() << "Unable to save pixmap to " << tempFile.fileName() << endl;
        return NoMatch;
    }

    return addCover(KUrl::fromPath(tempFile.fileName()), artist, album);
}

coverKey CoverManager::addCover(const KUrl &path, const QString &artist, const QString &album)
{
    coverKey id = data()->nextId();
    CoverDataPtr coverData(new CoverData);

    QString fileNameExt = path.fileName();
    int extPos = fileNameExt.lastIndexOf('.');

    fileNameExt = fileNameExt.mid(extPos);
    if(extPos == -1)
        fileNameExt = "";

    // Copy it to a local file first.

    QString ext = QString("/coverdb/coverID-%1%2").arg(id).arg(fileNameExt);
    coverData->path = KGlobal::dirs()->saveLocation("appdata") + ext;

    kDebug() << "Saving pixmap to " << coverData->path;
    data()->createDataDir();

    // Can't use NetAccess::download() since if path is already a local file
    // (which is possible) then that function will return without copying, since
    // it assumes we merely want the file on the hard disk somewhere.

    if(!KIO::NetAccess::file_copy(path, KUrl::fromPath(coverData->path))) {
        kError() << "Failed to download cover from " << path << endl;
        return NoMatch;
    }

    coverData->artist = artist.toLower();
    coverData->album = album.toLower();
    coverData->refCount = 0;

    data()->covers[id] = coverData;

    // Make sure the new cover isn't inadvertently cached.
    data()->pixmapCache.remove(QString("f%1").arg(coverData->path));
    data()->pixmapCache.remove(QString("t%1").arg(coverData->path));

    return id;
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
    QList<QString> affectedFiles = data()->tracks.keys(id);
    foreach (const QString &file, affectedFiles) {
        data()->tracks.remove(file);
    }

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
    data()->pixmapCache.remove(QString("t%1").arg(coverData->path));
    data()->pixmapCache.remove(QString("f%1").arg(coverData->path));

    large.save(coverData->path, "PNG");
    return true;
}

CoverManagerPrivate *CoverManager::data()
{
    return sd;
}

void CoverManager::saveCovers()
{
    data()->saveCovers();
}

void CoverManager::shutdown()
{
    sd.destroy();
}

CoverDataMapIterator CoverManager::begin()
{
    return data()->covers.constBegin();
}

CoverDataMapIterator CoverManager::end()
{
    return data()->covers.constEnd();
}

CoverList CoverManager::keys()
{
    return data()->covers.keys();
}

void CoverManager::setIdForTrack(const QString &path, coverKey id)
{
    coverKey oldId = data()->tracks.value(path, NoMatch);
    if(data()->tracks.contains(path) && (id == oldId))
        return; // We're already done.

    if(oldId != NoMatch) {
        data()->covers[oldId]->refCount--;
        data()->tracks.remove(path);

        if(data()->covers[oldId]->refCount == 0) {
            kDebug(65432) << "Cover " << oldId << " is unused, removing.\n";
            removeCover(oldId);
        }
    }

    if(id != NoMatch) {
        data()->covers[id]->refCount++;
        data()->tracks.insert(path, id);
    }
}

coverKey CoverManager::idForTrack(const QString &path)
{
    return data()->tracks.value(path, NoMatch);
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

// vim: set et sw=4 tw=0 sta:
