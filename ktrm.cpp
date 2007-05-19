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
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA         *
 *   02110-1301, USA                                                       *
 ***************************************************************************/

#include "ktrm.h"
#include <config-juk.h>
#ifdef HAVE_TUNEPIMP //Silence warning about HAVE_TUNEPIMP not being defined.
#if HAVE_TUNEPIMP

#include <kapplication.h>
#include <k3resolver.h>
#include <kprotocolmanager.h>
#include <kurl.h>
#include <kdebug.h>
#include <kio/job.h>

#include <QMutex>
#include <QRegExp>
#include <QEvent>
#include <QObject>
#include <QFile>
#include <QCustomEvent>
#include <QDomDocument>

#if HAVE_TUNEPIMP >= 5
#include <tunepimp-0.5/tp_c.h>
#else
#include <tunepimp/tp_c.h>
#endif
#include <fixx11h.h>

#include "ktrm.moc"
#include <q3tl.h>

class KTRMLookup;

extern "C"
{
#if HAVE_TUNEPIMP >= 4
    static void TRMNotifyCallback(tunepimp_t pimp, void *data, TPCallbackEnum type, int fileId, TPFileStatus status);
#else
    static void TRMNotifyCallback(tunepimp_t pimp, void *data, TPCallbackEnum type, int fileId);
#endif
}

/**
 * This represents the main TunePimp instance and handles incoming requests.
 */

class KTRMRequestHandler
{
public:
    static KTRMRequestHandler *instance()
    {
        static QMutex mutex;
        mutex.lock();
        static KTRMRequestHandler handler;
        mutex.unlock();
        return &handler;
    }

    int startLookup(KTRMLookup *lookup)
    {
        int id;

        if(!m_fileMap.contains(lookup->file())) {
#if HAVE_TUNEPIMP >= 4
            id = tp_AddFile(m_pimp, QFile::encodeName(lookup->file()), 0);
#else
            id = tp_AddFile(m_pimp, QFile::encodeName(lookup->file()));
#endif
            m_fileMap.insert(lookup->file(), id);
        }
        else {
            id = m_fileMap[lookup->file()];
            tp_IdentifyAgain(m_pimp, id);
        }
        m_lookupMap[id] = lookup;
        return id;
    }

    void endLookup(KTRMLookup *lookup)
    {
        tp_ReleaseTrack(m_pimp, tp_GetTrack(m_pimp, lookup->fileId()));
        tp_Remove(m_pimp, lookup->fileId());
        m_lookupMap.remove(lookup->fileId());
    }

    bool lookupMapContains(int fileId) const
    {
        m_lookupMapMutex.lock();
        bool contains = m_lookupMap.contains(fileId);
        m_lookupMapMutex.unlock();
        return contains;
    }

    KTRMLookup *lookup(int fileId) const
    {
        m_lookupMapMutex.lock();
        KTRMLookup *l = m_lookupMap[fileId];
        m_lookupMapMutex.unlock();
        return l;
    }

    void removeFromLookupMap(int fileId)
    {
        m_lookupMapMutex.lock();
        m_lookupMap.remove(fileId);
        m_lookupMapMutex.unlock();
    }

    const tunepimp_t &tunePimp() const
    {
        return m_pimp;
    }

protected:
    KTRMRequestHandler()
    {
        m_pimp = tp_New("KTRM", "0.1");
        //tp_SetDebug(m_pimp, true);
#if HAVE_TUNEPIMP < 5
        tp_SetTRMCollisionThreshold(m_pimp, 100);
        tp_SetAutoFileLookup(m_pimp,true);
#endif        
        tp_SetAutoSaveThreshold(m_pimp, -1);
        tp_SetMoveFiles(m_pimp, false);
        tp_SetRenameFiles(m_pimp, false);
#if HAVE_TUNEPIMP >= 4
        tp_SetFileNameEncoding(m_pimp, "UTF-8");
#else
        tp_SetUseUTF8(m_pimp, true);
#endif
        tp_SetNotifyCallback(m_pimp, TRMNotifyCallback, 0);
#if HAVE_TUNEPIMP < 5
        // Re-read proxy config.
        KProtocolManager::reparseConfiguration();

        if(KProtocolManager::useProxy()) {
            // split code copied from kcm_kio.
            QString noProxiesFor = KProtocolManager::noProxyFor();
            QStringList noProxies = noProxiesFor.split(QRegExp("[',''\t'' ']"), QString::SkipEmptyParts);
            bool useProxy = true;

            // Host that libtunepimp will contact.
            QString tunepimpHost = "www.musicbrainz.org";
            QString tunepimpHostWithPort = "www.musicbrainz.org:80";

            // Check what hosts are allowed to proceed without being proxied,
            // or is using reversed proxy, what hosts must be proxied.
            for(QStringList::ConstIterator it = noProxies.constBegin(); it != noProxies.constEnd(); ++it) {
                QString normalizedHost = KNetwork::KResolver::normalizeDomain(*it);

                if(normalizedHost == tunepimpHost ||
                   tunepimpHost.endsWith('.' + normalizedHost))
                {
                    useProxy = false;
                    break;
                }

                // KDE's proxy mechanism also supports exempting a specific
                // host/port combo, check that also.
                if(normalizedHost == tunepimpHostWithPort ||
                   tunepimpHostWithPort.endsWith('.' + normalizedHost))
                {
                    useProxy = false;
                    break;
                }
            }

            // KDE supports a reverse proxy mechanism.  Uh, yay.
            if(KProtocolManager::useReverseProxy())
                useProxy = !useProxy;

            if(useProxy) {
                KUrl proxy = KProtocolManager::proxyFor("http");
                QString proxyHost = proxy.host();

                kDebug(65432) << "Using proxy server " << proxyHost << " for www.musicbrainz.org.\n";
                tp_SetProxy(m_pimp, proxyHost.toAscii(), short(proxy.port()));
            }
        }
#else
        tp_SetMusicDNSClientId(m_pimp, "0c6019606b1d8a54d0985e448f3603ca");
#endif
    }

    ~KTRMRequestHandler()
    {
        tp_Delete(m_pimp);
    }

private:
    tunepimp_t m_pimp;
    QMap<int, KTRMLookup *> m_lookupMap;
    QMap<QString, int> m_fileMap;
    mutable QMutex m_lookupMapMutex;
};


/**
 * A custom event type used for signalling that a TRM lookup is finished.
 */

class KTRMEvent : public QCustomEvent
{
public:
    enum Status {
        Recognized,
        Unrecognized,
        Collision,
        PuidGenerated,
        Error
    };

    KTRMEvent(int fileId, Status status) :
        QCustomEvent(id),
        m_fileId(fileId),
        m_status(status) {}

    int fileId() const
    {
        return m_fileId;
    }

    Status status() const
    {
        return m_status;
    }

    static const int id = User + 1984; // random, unique, event id

private:
    int m_fileId;
    Status m_status;
};

/**
 * A helper class to intercept KTRMQueryEvents and call recognized() (from the GUI
 * thread) for the lookup.
 */

class KTRMEventHandler : public QObject
{
public:
    static void send(int fileId, KTRMEvent::Status status)
    {
        KApplication::postEvent(instance(), new KTRMEvent(fileId, status));
    }

protected:
    KTRMEventHandler() : QObject() {}

    static KTRMEventHandler *instance()
    {
        static QMutex mutex;
        mutex.lock();
        static KTRMEventHandler handler;
        mutex.unlock();
        return &handler;
    }

    virtual void customEvent(QCustomEvent *event)
    {
        if(!event->type() == KTRMEvent::id)
            return;

        KTRMEvent *e = static_cast<KTRMEvent *>(event);

        static QMutex mutex;
        mutex.lock();

        if(!KTRMRequestHandler::instance()->lookupMapContains(e->fileId())) {
            mutex.unlock();
            return;
        }

        KTRMLookup *lookup = KTRMRequestHandler::instance()->lookup(e->fileId());
#if HAVE_TUNEPIMP >= 4
        if ( e->status() != KTRMEvent::Unrecognized)
#endif
            KTRMRequestHandler::instance()->removeFromLookupMap(e->fileId());

        mutex.unlock();

        switch(e->status()) {
        case KTRMEvent::Recognized:
            lookup->recognized();
            break;
        case KTRMEvent::Unrecognized:
            lookup->unrecognized();
            break;
        case KTRMEvent::Collision:
            lookup->collision();
            break;
        case KTRMEvent::Error:
            lookup->error();
            break;
        }
    }
};

/**
 * Callback function for TunePimp lookup events.
 */
#if HAVE_TUNEPIMP >= 4
static void TRMNotifyCallback(tunepimp_t /*pimp*/, void * /*data*/, TPCallbackEnum type, int fileId, TPFileStatus status)
#else
static void TRMNotifyCallback(tunepimp_t pimp, void *data, TPCallbackEnum type, int fileId)
#endif
{
    if(type != tpFileChanged)
        return;

#if HAVE_TUNEPIMP < 4
    track_t track = tp_GetTrack(pimp, fileId);
    TPFileStatus status = tr_GetStatus(track);
#endif

    switch(status) {
    case eRecognized:
        KTRMEventHandler::send(fileId, KTRMEvent::Recognized);
        break;
    case eUnrecognized:
        KTRMEventHandler::send(fileId, KTRMEvent::Unrecognized);
        break;
#if HAVE_TUNEPIMP >= 5
    case ePUIDLookup:
    case ePUIDCollision:
    case eFileLookup:
        KTRMEventHandler::send(fileId, KTRMEvent::PuidGenerated);
        break;
#else       
    case eTRMCollision:
#if HAVE_TUNEPIMP >= 4
    case eUserSelection:
#endif
        KTRMEventHandler::send(fileId, KTRMEvent::Collision);
        break;
#endif
    case eError:
        KTRMEventHandler::send(fileId, KTRMEvent::Error);
        break;
    default:
        break;
    }
#if HAVE_TUNEPIMP < 4
    tp_ReleaseTrack(pimp, track);
#endif
}

////////////////////////////////////////////////////////////////////////////////
// KTRMResult implementation
////////////////////////////////////////////////////////////////////////////////

class KTRMResult::KTRMResultPrivate
{
public:
    KTRMResultPrivate() : track(0), year(0), relevance(0) {}
    QString title;
    QString artist;
    QString album;
    int track;
    int year;
    int relevance;
};

////////////////////////////////////////////////////////////////////////////////
// KTRMResult public methods
////////////////////////////////////////////////////////////////////////////////

KTRMResult::KTRMResult()
{
    d = new KTRMResultPrivate;
}

KTRMResult::KTRMResult(const KTRMResult &result)
{
    d = new KTRMResultPrivate(*result.d);
}

KTRMResult::~KTRMResult()
{
    delete d;
}

QString KTRMResult::title() const
{
    return d->title;
}

QString KTRMResult::artist() const
{
    return d->artist;
}

QString KTRMResult::album() const
{
    return d->album;
}

int KTRMResult::track() const
{
    return d->track;
}

int KTRMResult::year() const
{
    return d->year;
}

bool KTRMResult::operator<(const KTRMResult &r) const
{
    return r.d->relevance < d->relevance;
}

bool KTRMResult::operator>(const KTRMResult &r) const
{
    return r.d->relevance > d->relevance;
}

bool KTRMResult::isEmpty() const
{
    return d->title.isEmpty() && d->artist.isEmpty() && d->album.isEmpty() &&
        d->track == 0 && d->year == 0;
}

////////////////////////////////////////////////////////////////////////////////
// KTRMLookup implementation
////////////////////////////////////////////////////////////////////////////////

class KTRMLookup::KTRMLookupPrivate
{
public:
    KTRMLookupPrivate() :
        fileId(-1) {}
    QString file;
    KTRMResultList results;
    int fileId;
    bool autoDelete;
};

////////////////////////////////////////////////////////////////////////////////
// KTRMLookup public methods
////////////////////////////////////////////////////////////////////////////////

KTRMLookup::KTRMLookup(const QString &file, bool autoDelete)
{
    d = new KTRMLookupPrivate;
    d->file = file;
    d->autoDelete = autoDelete;
    d->fileId = KTRMRequestHandler::instance()->startLookup(this);
}

KTRMLookup::~KTRMLookup()
{
    KTRMRequestHandler::instance()->endLookup(this);
    delete d;
}

QString KTRMLookup::file() const
{
    return d->file;
}

int KTRMLookup::fileId() const
{
    return d->fileId;
}

void KTRMLookup::recognized()
{
    kDebug() << k_funcinfo << d->file << endl;

    d->results.clear();

    metadata_t *metaData = md_New();
    track_t track = tp_GetTrack(KTRMRequestHandler::instance()->tunePimp(), d->fileId);
    tr_Lock(track);
    tr_GetServerMetadata(track, metaData);

    KTRMResult result;

    result.d->title = QString::fromUtf8(metaData->track);
    result.d->artist = QString::fromUtf8(metaData->artist);
    result.d->album = QString::fromUtf8(metaData->album);
    result.d->track = metaData->trackNum;
    result.d->year = metaData->releaseYear;

    d->results.append(result);

    md_Delete(metaData);
    tr_Unlock(track);
    finished();
}

void KTRMLookup::unrecognized()
{
    kDebug() << k_funcinfo << d->file << endl;
#if HAVE_TUNEPIMP >= 4
    char trm[255];
    bool finish = false;
    trm[0] = 0;
    track_t track = tp_GetTrack(KTRMRequestHandler::instance()->tunePimp(), d->fileId);
    tr_Lock(track);
#if HAVE_TUNEPIMP >= 5
    tr_GetPUID(track, trm, 255);
#else
    tr_GetTRM(track, trm, 255);
#endif

    if ( !trm[0] ) {
        tr_SetStatus(track, ePending);
        tp_Wake(KTRMRequestHandler::instance()->tunePimp(), track);
    }
    else
        finish = true;
    tr_Unlock(track);
    tp_ReleaseTrack(KTRMRequestHandler::instance()->tunePimp(), track);
    if ( !finish )
       return;
#endif
    d->results.clear();
    finished();
}

void KTRMLookup::collision()
{
#if HAVE_TUNEPIMP && HAVE_TUNEPIMP < 5
    kDebug() << k_funcinfo << d->file << endl;

    track_t track = tp_GetTrack(KTRMRequestHandler::instance()->tunePimp(), d->fileId);

    if(track <= 0) {
        kDebug() << "invalid track number" << endl;
        return;
    }

    tr_Lock(track);
    int resultCount = tr_GetNumResults(track);

    if(resultCount > 0) {
        TPResultType type;
        result_t *results = new result_t[resultCount];
        tr_GetResults(track, &type, results, &resultCount);

        switch(type) {
        case eNone:
            kDebug() << k_funcinfo << "eNone" << endl;
            break;
        case eArtistList:
            kDebug() << "eArtistList" << endl;
            break;
        case eAlbumList:
            kDebug() << "eAlbumList" << endl;
            break;
        case eTrackList:
        {
            kDebug() << "eTrackList" << endl;
            albumtrackresult_t **tracks = (albumtrackresult_t **) results;
            d->results.clear();

            for(int i = 0; i < resultCount; i++) {
                KTRMResult result;

                result.d->title = QString::fromUtf8(tracks[i]->name);
#if HAVE_TUNEPIMP >= 4
                result.d->artist = QString::fromUtf8(tracks[i]->artist.name);
                result.d->album = QString::fromUtf8(tracks[i]->album.name);
                result.d->year = tracks[i]->album.releaseYear;
#else
                result.d->artist = QString::fromUtf8(tracks[i]->artist->name);
                result.d->album = QString::fromUtf8(tracks[i]->album->name);
                result.d->year = tracks[i]->album->releaseYear;
#endif
                result.d->track = tracks[i]->trackNum;
                result.d->relevance = tracks[i]->relevance;

                d->results.append(result);
            }
            break;
        }
        case eMatchedTrack:
            kDebug() << k_funcinfo << "eMatchedTrack" << endl;
            break;
        }

        delete [] results;
    }

    tr_Unlock(track);

    finished();
#endif
}

void KTRMLookup::puidGenerated()
{
#if HAVE_TUNEPIMP >= 5
    char puid[255] = {0};
    track_t track = tp_GetTrack(KTRMRequestHandler::instance()->tunePimp(), d->fileId);
    tr_Lock(track);

    tr_GetPUID(track, puid, 255);
    tr_Unlock(track);
    tp_ReleaseTrack(KTRMRequestHandler::instance()->tunePimp(), track);
    d->results.clear();

    KIO::Job *job = KIO::storedGet( QString( "http://musicbrainz.org/ws/1/track/?type=xml&puid=%1" ).arg( puid ) , false, false );
    connect( job, SIGNAL( result( KJob* ) ), SLOT( lookupResult( KJob* ) ) );
#endif
}

void KTRMLookup::lookupResult( KJob* job )
{
#if HAVE_TUNEPIMP >= 5
    if ( !job->error() == 0 ) {
        finished();
        return;
    }
    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( job );
    QString xml = QString::fromUtf8( storedJob->data().data(), storedJob->data().size() );

    QDomDocument doc;
    QDomElement e;

    if( !doc.setContent( xml ) ) {
        finished();
        return;
    }

    e = doc.namedItem( "metadata" ).toElement().namedItem( "track-list" ).toElement();

    QStringList strList = QStringList::split ( '/', d->file );

    QDomNode n = e.namedItem("track");
    for( ; !n.isNull();  n = n.nextSibling() ) {
        QDomElement track = n.toElement();
        KTRMResult result;

        result.d->title = track.namedItem( "title" ).toElement().text();
        result.d->artist = track.namedItem( "artist" ).toElement().namedItem( "name" ).toElement().text();
        QDomNode releaseNode = track.namedItem("release-list").toElement().namedItem("release");
        for( ; !releaseNode.isNull();  releaseNode = releaseNode.nextSibling() ) {
            KTRMResult tmpResult( result );
            QDomElement release = releaseNode.toElement();

            tmpResult.d->album = release.namedItem( "title" ).toElement().text();
            QDomNode tracklistN = release.namedItem( "track-list" );
            if ( !tracklistN.isNull() ) {
                QDomElement tracklist = tracklistN.toElement();
                if ( !tracklist.attribute( "offset" ).isEmpty() )
                    tmpResult.d->track = tracklist.attribute( "offset" ).toInt() + 1;
            }
            //tmpResult.d->year = ???;
            tmpResult.d->relevance =
                4 * stringSimilarity(strList,tmpResult.d->title) +
                2 * stringSimilarity(strList,tmpResult.d->artist) +
                1 * stringSimilarity(strList,tmpResult.d->album);
/*            
            if( !d->results.contains( tmpResult ) )
                d->results.append( tmpResult );
                */
        }
     }

     qHeapSort(d->results);

     finished();
#else
    Q_UNUSED( job );
#endif
}


void KTRMLookup::error()
{
    kDebug() << k_funcinfo << d->file << endl;

    d->results.clear();
    finished();
}

KTRMResultList KTRMLookup::results() const
{
    return d->results;
}

////////////////////////////////////////////////////////////////////////////////
// KTRMLookup protected methods
////////////////////////////////////////////////////////////////////////////////

void KTRMLookup::finished()
{
    if(d->autoDelete)
        delete this;
}

////////////////////////////////////////////////////////////////////////////////
// Helper Functions used for sorting MusicBrainz results
////////////////////////////////////////////////////////////////////////////////
double stringSimilarity(QStringList &l, QString &s)
{
    double max = 0, current = 0;
    for ( QStringList::Iterator it = l.begin(); it != l.end(); ++it ) {
       if( max < (current = stringSimilarity((*it),s)))
            max = current;
    }
    return max;
}
double stringSimilarity(QString s1, QString s2)
{
    s1.remove( QRegExp("[\\s\\t\\r\\n]") );
    s2.remove( QRegExp("[\\s\\t\\r\\n]") );

    double nCommon = 0;
    int p1 = 0, p2 = 0, x1 = 0, x2 = 0;
    int l1 = s1.length(), l2 = s2.length(), l3 = l1 + l2;
    QChar c1 = 0, c2 = 0;

    while(p1 < l1 && p2 < l2) {
        c1 = s1.at(p1); c2 = s2.at(p2);
        if( c1.toUpper() == c2.toUpper()) {
            ++nCommon;
            ++p1; ++p2;
        }
        else {
            x1 = s1.indexOf(c2,p1,Qt::CaseInsensitive);
            x2 = s2.indexOf(c1,p2,Qt::CaseInsensitive);

            if( (x1 == x2 || -1 == x1) || (-1 != x2 && x1 > x2) )
                ++p2;
            else
                ++p1;
        }
    }
    return l3 ? (double)(nCommon*2) / (double)(l3) : 1;
}

#endif
#endif

// vim: set et sw=4 tw=0 sta:
