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

#ifndef JUK_COVERMANAGER_H
#define JUK_COVERMANAGER_H

#include <ksharedptr.h>

#include <qmap.h>
#include <qdragobject.h>

class CoverManagerPrivate;
class QString;
class QPixmap;
class QDataStream;

/**
 * This class holds the data on a cover.  This includes the path to the cover
 * representation on-disk, and the artist and album associated with the cover.
 * Don't assume that the artist or album information is filled out, it is
 * there to allow the CoverManager to try to automatically assign covers to
 * new tracks.
 * 
 * @author Michael Pyne <michael.pyne@kdemail.net>
 * @see CoverManager
 */
class CoverData : public KShared
{
public:
    QPixmap pixmap() const;
    QPixmap thumbnail() const;

    QString artist;
    QString album;
    QString path;

    unsigned refCount; // Refers to number of tracks using this.
};

typedef KSharedPtr<CoverData> CoverDataPtr;
typedef unsigned long coverKey; ///< Type of the id for a cover.
typedef QMap<coverKey, CoverDataPtr> CoverDataMap;

/**
 * This class is used to drag covers in JuK.  It adds a special mimetype that
 * contains the cover ID used for this cover, and also supports an image/png
 * mimetype for dragging to other applications.
 *
 * As of this writing the mimetype is application/x-juk-coverid
 *
 * @author Michael Pyne <michael.pyne@kdemail.net>
 */
class CoverDrag : public QDragObject
{
public:
    CoverDrag(coverKey id, QWidget *src);

    virtual const char *format(int i) const;
    virtual QByteArray encodedData(const char *mimetype) const;

    void setId(coverKey id) { m_id = id; }

    /**
     * Returns true if CoverDrag can decode the given mime source.  Note that
     * true is returned only if \p e contains a cover id, even though
     * CoverDrag can convert it to an image.
     */
    static bool canDecode(const QMimeSource *e);
    static bool decode(const QMimeSource *e, coverKey &id);

    static const char* mimetype;

private:
    coverKey m_id;
};

/**
 * This class holds all of the cover art, and manages looking it up by artist
 * and/or album.  This class is similar to a singleton class, but instead all
 * of the methods are static.  This way you can invoke methods like this:
 * \code
 *   CoverManager::method()
 * \endcode
 * instead of using:
 * \code
 *   CoverManager::instance()->method()
 * \endcode
 *
 * @author Michael Pyne <michael.pyne@kdemail.net>
 */
class CoverManager
{
public:
    /// The set of different sizes you can request a pixmap as.
    typedef enum { Thumbnail, FullSize } Size;

    /**
     * Tries to match @p artist and @p album to a cover in the database.
     *
     * @param artist The artist to look for matching covers on.
     * @param album The album to look for matching covers on.
     * @return NoMatch if no match could be found, otherwise the id of the
     *         cover art that matches the given metadata.
     */
    static coverKey idFromMetadata(const QString &artist, const QString &album);

    /**
     * Returns the cover art for @p id.
     *
     * @param id The id of the cover.
     * @param size The size to return it as.  Note that FullSize doesn't
     *             necessarily mean the pixmap is large, so you may need to
     *             scale it up.
     * @return QPixmap::null if there is no cover art for @p id, otherwise the
     *         cover art.
     */
    static QPixmap coverFromId(coverKey id, Size size = Thumbnail);

    /**
     * Returns the cover art for @p ptr.  This function is intended for use
     * by CoverData.
     *
     * @param ptr The CoverData to get the cover of.  Note that it is a
     *            CoverData, not CoverDataPtr.
     * @param size The size to return it as.
     * @see CoverData
     */
    static QPixmap coverFromData(const CoverData &coverData, Size size = Thumbnail);

    /**
     * Returns the full suite of information known about the cover given by
     * @p id.
     *
     * @param id the id of the cover to retrieve info on.
     * @return 0 if there is no info on @p id, otherwise its information.
     */
    static CoverDataPtr coverInfo(coverKey id);

    /**
     * Adds @p large to the cover database, associating with it @p artist and
     * @p album.
     *
     * @param large The full size cover (the thumbnail is automatically
     *              generated).
     * @param artist The artist of the new cover.
     * @param album  The album of the new cover.
     */
    static coverKey addCover(const QPixmap &large, const QString &artist = "", const QString &album = "");

    /**
     * Adds the file pointed to by the local path @p path to the database,
     * associating it with @p artist and @p album.
     *
     * @param path The absolute path to the fullsize cover art.
     * @param artist The artist of the new cover.
     * @param album  The album of the new cover.
     */
    static coverKey addCover(const QString &path, const QString &artist = "", const QString &album = "");

    /**
     * Function to determine if @p id matches any covers in the database.
     * 
     * @param id The id of the cover to search for.
     * @return true if the database has a cover identified by @p id, false
     *         otherwise.
     */
    static bool hasCover(coverKey id);

    /**
     * Removes the cover identified by @p id.
     *
     * @param id the id of the cover to remove.
     * @return true if the removal was successful, false if unsuccessful or if
     *         the cover didn't exist.
     */
    static bool removeCover(coverKey id);

    /**
     * Replaces the cover art for the cover identified by @p id with @p large.
     * Any other metadata such as artist and album is unchanged.
     *
     * @param id The id of the cover to replace.
     * @param large The full size cover art for the new cover.
     */
    static bool replaceCover(coverKey id, const QPixmap &large);

    /**
     * Saves the current CoverManager information to disk.  Changes are not
     * automatically written to disk due to speed issues, so you can
     * periodically call this function while running to reduce the chance of
     * lost data in the event of a crash.
     */
    static void saveCovers();

    /**
     * This is a hack, as we should be shut down automatically by
     * KStaticDeleter, but JuK is crashing for me on shutdown before
     * KStaticDeleter gets a chance to run, which is cramping my testing.
     */
    static void shutdown();

    /**
     * @return Iterator pointing to the first element in the cover database.
     */
    static CoverDataMap::ConstIterator begin();

    /**
     * @return Iterator pointing after the last element in the cover database.
     */
    static CoverDataMap::ConstIterator end();

    /**
     * @return A list of all of the id's listed in the database.
     */
    static QValueList<coverKey> keys();

    /**
     * Associates @p path with the cover identified by @id.  No comparison of
     * metadata is performed to enforce this matching.
     *
     * @param path The absolute file path to the track.
     * @param id The identifier of the cover to use with @p path.
     */
    static void setIdForTrack(const QString &path, coverKey id);

    /**
     * Returns the identifier of the cover for the track at @p path.
     *
     * @param path The absolute file path to the track.
     * @return NoMatch if @p path doesn't have a cover, otherwise the id of
     *         its cover.
     */
    static coverKey idForTrack(const QString &path);

    /**
     * This identifier is used to indicate that no cover was found in the
     * database.
     */
    static const coverKey NoMatch = 0;

    private:
    static CoverManagerPrivate *m_data;

    static CoverManagerPrivate *data();
    static QPixmap createThumbnail(const QPixmap &base);
};

#endif /* JUK_COVERMANAGER_H */

// vim: set et sw=4 ts=4:
