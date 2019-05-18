/**
 * Copyright (C) 2005, 2008 Michael Pyne <mpyne@kde.org>
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

#include "covermanager.h"

#include <QGlobalStatic>
#include <QTimer>
#include <QPixmap>
#include <QString>
#include <QFile>
#include <QImage>
#include <QDir>
#include <QDataStream>
#include <QHash>
#include <QPixmapCache>
#include <QByteArray>
#include <QMap>
#include <QTemporaryFile>
#include <QStandardPaths>
#include <QUrl>

#include <kio/job.h>

#include "juk.h"
#include "coverproxy.h"
#include "juk_debug.h"

// This is a dictionary to map the track path to their ID.  Otherwise we'd have
// to store this info with each CollectionListItem, which would break the cache
// of users who upgrade, and would just generally be a big mess.
typedef QHash<QString, coverKey> TrackLookupMap;

static const char dragMimetype[] = "application/x-juk-coverid";

const coverKey CoverManager::NoMatch = 0;

// Used to save and load CoverData from a QDataStream
QDataStream &operator<<(QDataStream &out, const CoverData &data);
QDataStream &operator>>(QDataStream &in, CoverData &data);

//
// Implementation of CoverSaveHelper class
//

CoverSaveHelper::CoverSaveHelper(QObject *parent) :
    QObject(parent),
    m_timer(new QTimer(this))
{
    connect(m_timer, SIGNAL(timeout()), SLOT(commitChanges()));

    // Wait 5 seconds before committing to avoid lots of disk activity for
    // rapid changes.

    m_timer->setSingleShot(true);
    m_timer->setInterval(5000);
}

void CoverSaveHelper::saveCovers()
{
    m_timer->start(); // Restarts if already triggered.
}

void CoverSaveHelper::commitChanges()
{
    CoverManager::saveCovers();
}

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
 * @author Michael Pyne <mpyne@kde.org>
 * @see CoverManager
 */
class CoverManagerPrivate
{
public:

    /// Maps coverKey id's to CoverData
    CoverDataMap covers;

    /// Maps file names to coverKey id's.
    TrackLookupMap tracks;

    /// A map of outstanding download KJobs to their coverKey
    QMap<KJob*, coverKey> downloadJobs;

    /// A static pixmap cache is maintained for covers, with key format of:
    /// 'f' followed by the pathname for FullSize covers, and
    /// 't' followed by the pathname for Thumbnail covers.
    /// However only thumbnails are currently cached.

    CoverManagerPrivate() : m_timer(new CoverSaveHelper(0)), m_coverProxy(0)
    {
        loadCovers();
    }

    ~CoverManagerPrivate()
    {
        delete m_timer;
        delete m_coverProxy;
        saveCovers();
    }

    void requestSave()
    {
        m_timer->saveCovers();
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

    CoverProxy *coverProxy() {
        if(!m_coverProxy)
            m_coverProxy = new CoverProxy;
        return m_coverProxy;
    }

    private:
    void loadCovers();

    /**
     * @return the full path and filename of the file storing the cover
     * lookup map and the translations between pathnames and ids.
     */
    QString coverLocation() const;

    CoverSaveHelper *m_timer;

    CoverProxy *m_coverProxy;
};

// This is responsible for making sure that the CoverManagerPrivate class
// gets properly destructed on shutdown.
Q_GLOBAL_STATIC(CoverManagerPrivate, sd)

//
// Implementation of CoverManagerPrivate methods.
//
void CoverManagerPrivate::createDataDir() const
{
    QDir dir;
    QString dirPath(QDir::cleanPath(coverLocation() + "/.."));
    dir.mkpath(dirPath);
}

void CoverManagerPrivate::saveCovers() const
{
    // Make sure the directory exists first.
    createDataDir();

    QFile file(coverLocation());

    qCDebug(JUK_LOG) << "Opening covers db: " << coverLocation();

    if(!file.open(QIODevice::WriteOnly)) {
        qCCritical(JUK_LOG) << "Unable to save covers to disk!\n";
        return;
    }

    QDataStream out(&file);

    // Write out the version and count
    out << quint32(0) << quint32(covers.size());

    qCDebug(JUK_LOG) << "Writing out" << covers.size() << "covers.";

    // Write out the data
    for(const auto &it : covers) {
        out << quint32(it.first);
        out << it.second;
    }

    // Now write out the track mapping.
    out << quint32(tracks.count());

    qCDebug(JUK_LOG) << "Writing out" << tracks.count() << "tracks.";

    TrackLookupMap::ConstIterator trackMapIt = tracks.constBegin();
    while(trackMapIt != tracks.constEnd()) {
        out << trackMapIt.key() << quint32(trackMapIt.value());
        ++trackMapIt;
    }
}

void CoverManagerPrivate::loadCovers()
{
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
        qCCritical(JUK_LOG) << "Cover database was created by a higher version of JuK,\n";
        qCCritical(JUK_LOG) << "I don't know what to do with it.\n";

        return;
    }

    // Read in the count next, then the data.
    in >> count;

    qCDebug(JUK_LOG) << "Loading" << count << "covers.";
    for(quint32 i = 0; i < count; ++i) {
        // Read the id, and 3 QStrings for every 1 of the count.
        quint32 id;
        CoverData data;

        in >> id;
        in >> data;
        data.refCount = 0;

        covers[(coverKey) id] = data;
    }

    in >> count;
    qCDebug(JUK_LOG) << "Loading" << count << "tracks";
    for(quint32 i = 0; i < count; ++i) {
        QString path;
        quint32 id;

        in >> path >> id;

        // If we somehow already managed to load a cover id with this path,
        // don't do so again.  Possible due to a coding error during 3.5
        // development.

        if(Q_LIKELY(!tracks.contains(path))) {
            ++covers[(coverKey) id].refCount; // Another track using this.
            tracks.insert(path, id);
        }
    }

    qCDebug(JUK_LOG) << "Tracks hash table has" << tracks.size() << "entries.";
}

QString CoverManagerPrivate::coverLocation() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
            + "coverdb/covers";
}

coverKey CoverManagerPrivate::nextId() const
{
    // Start from 1...
    coverKey key = 1;

    while(covers.find(key) != covers.end())
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
          CoverDataMap::const_iterator it    = begin();
    const CoverDataMap::const_iterator endIt = end();

    for(; it != endIt; ++it) {
        if(it->second.album == album.toLower() && it->second.artist == artist.toLower())
            return it->first;
    }

    return NoMatch;
}

QPixmap CoverManager::coverFromId(coverKey id, Size size)
{
    const auto &info = data()->covers.find(id);
    if(info == data()->covers.end())
        return QPixmap();

    if(size == Thumbnail)
        return info->second.thumbnail();

    return info->second.pixmap();
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

    QPixmap pix;
    if(QPixmapCache::find(path, &pix))
        return pix;

    // Not in cache, load it and add it.

    if(!pix.load(coverData.path))
        return QPixmap();

    // Only thumbnails are cached to avoid depleting global cache.  Caching
    // full size pics is not really useful as they are infrequently shown.

    if(size == Thumbnail) {
        // Double scale is faster and 99% as accurate
        QSize newSize(pix.size());
        newSize.scale(80, 80, Qt::KeepAspectRatio);
        pix = pix.scaled(2 * newSize)
                 .scaled(newSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        QPixmapCache::insert(path, pix);
    }

    return pix;
}

coverKey CoverManager::addCover(const QPixmap &large, const QString &artist, const QString &album)
{
    qCDebug(JUK_LOG) << "Adding new pixmap to cover database.";
    if(large.isNull()) {
        qCDebug(JUK_LOG) << "The pixmap you're trying to add is NULL!";
        return NoMatch;
    }

    QTemporaryFile tempFile;
    if(!tempFile.open() || !large.save(tempFile.fileName(), "PNG")) {
        qCCritical(JUK_LOG) << "Unable to save pixmap to " << tempFile.fileName();
        return NoMatch;
    }

    return addCover(QUrl::fromLocalFile(tempFile.fileName()), artist, album);
}

coverKey CoverManager::addCover(const QUrl &path, const QString &artist, const QString &album)
{
    coverKey id = data()->nextId();
    CoverData coverData;

    QString fileNameExt = path.fileName();
    int extPos = fileNameExt.lastIndexOf('.');

    fileNameExt = fileNameExt.mid(extPos);
    if(extPos == -1)
        fileNameExt = "";

    // Copy it to a local file first.

    QString ext = QString("/coverdb/coverID-%1%2").arg(id).arg(fileNameExt);
    coverData.path = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
            + ext;
    qCDebug(JUK_LOG) << "Saving pixmap to " << coverData.path;
    data()->createDataDir();

    coverData.artist = artist.toLower();
    coverData.album = album.toLower();
    coverData.refCount = 0;

    data()->covers.emplace(id, coverData);

    // Can't use NetAccess::download() since if path is already a local file
    // (which is possible) then that function will return without copying, since
    // it assumes we merely want the file on the hard disk somewhere.

    KIO::FileCopyJob *job = KIO::file_copy(
         path, QUrl::fromLocalFile(coverData.path),
         -1 /* perms */, KIO::HideProgressInfo | KIO::Overwrite
         );
    QObject::connect(job, SIGNAL(result(KJob*)),
                     data()->coverProxy(), SLOT(handleResult(KJob*)));
    data()->downloadJobs.insert(job, id);

    job->start();

    data()->requestSave(); // Save changes when possible.

    return id;
}

/**
 * This is called when our cover downloader has completed.  Typically there
 * should be no issues so we just need to ensure that the newly downloaded
 * cover is picked up by invalidating any cache entry for it.  If it didn't
 * download successfully we're in kind of a pickle as we've already assigned
 * a coverKey, which we need to go and erase.
 */
void CoverManager::jobComplete(KJob *job, bool completedSatisfactory)
{
    coverKey id = NoMatch;
    if(data()->downloadJobs.contains(job))
        id = data()->downloadJobs[job];

    if(id == NoMatch) {
        qCCritical(JUK_LOG) << "No information on what download job" << job << "is.";
        data()->downloadJobs.remove(job);
        return;
    }

    if(!completedSatisfactory) {
        qCCritical(JUK_LOG) << "Job" << job << "failed, but not handled yet.";
        removeCover(id);
        data()->downloadJobs.remove(job);
        JuK::JuKInstance()->coverDownloaded(QPixmap());
        return;
    }

    CoverData coverData = data()->covers[id];

    // Make sure the new cover isn't inadvertently cached.
    QPixmapCache::remove(QString("f%1").arg(coverData.path));
    QPixmapCache::remove(QString("t%1").arg(coverData.path));

    JuK::JuKInstance()->coverDownloaded(coverFromData(coverData, CoverManager::Thumbnail));
}

bool CoverManager::hasCover(coverKey id)
{
    return data()->covers.find(id) != data()->covers.end();
}

bool CoverManager::removeCover(coverKey id)
{
    if(!hasCover(id))
        return false;

    // Remove cover from cache.
    CoverData coverData = coverInfo(id);
    QPixmapCache::remove(QString("f%1").arg(coverData.path));
    QPixmapCache::remove(QString("t%1").arg(coverData.path));

    // Remove references to files that had that track ID.
    QList<QString> affectedFiles = data()->tracks.keys(id);
    foreach (const QString &file, affectedFiles) {
        data()->tracks.remove(file);
    }

    // Remove covers from disk.
    QFile::remove(coverData.path);

    // Finally, forget that we ever knew about this cover.
    data()->covers.erase(id);
    data()->requestSave();

    return true;
}

bool CoverManager::replaceCover(coverKey id, const QPixmap &large)
{
    if(!hasCover(id))
        return false;

    CoverData coverData = coverInfo(id);

    // Empty old pixmaps from cache.
    QPixmapCache::remove(QString("t%1").arg(coverData.path));
    QPixmapCache::remove(QString("f%1").arg(coverData.path));

    large.save(coverData.path, "PNG");

    // No save is needed, as all that has changed is the on-disk cover data,
    // not the list of tracks or covers.

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

CoverDataMapIterator CoverManager::begin()
{
    return data()->covers.begin();
}

CoverDataMapIterator CoverManager::end()
{
    return data()->covers.end();
}

void CoverManager::setIdForTrack(const QString &path, coverKey id)
{
    coverKey oldId = data()->tracks.value(path, NoMatch);
    if(data()->tracks.contains(path) && (id == oldId))
        return; // We're already done.

    if(oldId != NoMatch) {
        data()->covers[oldId].refCount--;
        data()->tracks.remove(path);

        if(data()->covers[oldId].refCount == 0) {
            qCDebug(JUK_LOG) << "Cover " << oldId << " is unused, removing.\n";
            removeCover(oldId);
        }
    }

    if(id != NoMatch) {
        data()->covers[id].refCount++;
        data()->tracks.insert(path, id);
    }

    data()->requestSave();
}

coverKey CoverManager::idForTrack(const QString &path)
{
    return data()->tracks.value(path, NoMatch);
}

CoverData CoverManager::coverInfo(coverKey id)
{
    if(hasCover(id))
        return data()->covers[id];

    // TODO throw new something or other
    return CoverData{};
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
