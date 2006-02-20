/***************************************************************************
    copyright            : (C) 2004 Scott Wheeler
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

#include "gstreamerplayer.h"

#if HAVE_GSTREAMER

#include <kapplication.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kdebug.h>

#include <qfile.h>
#include <qtimer.h>

// Defined because recent versions of glib add support for having gcc check
// whether the sentinel used on g_object_{set,get} is correct.  Although 0
// is a valid NULL pointer in C++, when used in a C function call g++ doesn't
// know to turn it into a pointer so it leaves it as an int instead (which is
// wrong for 64-bit arch).  So, use the handy define below instead.

#define JUK_GLIB_NULL static_cast<gpointer>(0)

#if GSTREAMER_VERSION == 8

/******************************************************************************/
/******************************************************************************/
/******************************  GSTREAMER 0.8  *******************************/
/******************************************************************************/
/******************************************************************************/


////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

GStreamerPlayer::GStreamerPlayer() :
    Player(),
    m_pipeline(0),
    m_source(0),
    m_decoder(0),
    m_volume(0),
    m_sink(0)
{
    readConfig();
    setupPipeline();
}

GStreamerPlayer::~GStreamerPlayer()
{
    stop();
    gst_object_unref(GST_OBJECT(m_pipeline));
}

void GStreamerPlayer::play(const FileHandle &file)
{
    if(!file.isNull()) {
        stop();
        g_object_set(G_OBJECT(m_source), "location", file.absFilePath().local8Bit().data(), JUK_GLIB_NULL);
    }

    gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
}

void GStreamerPlayer::pause()
{
    gst_element_set_state(m_pipeline, GST_STATE_PAUSED);
}

void GStreamerPlayer::stop()
{
    gst_element_set_state(m_pipeline, GST_STATE_NULL);
}

void GStreamerPlayer::setVolume(float volume)
{
    g_object_set(G_OBJECT(m_volume), "volume", volume, JUK_GLIB_NULL);
}

float GStreamerPlayer::volume() const
{
    gdouble value;
    g_object_get(G_OBJECT(m_volume), "volume", &value, JUK_GLIB_NULL);
    return (float) value;
}

bool GStreamerPlayer::playing() const
{
    return gst_element_get_state(m_pipeline) == GST_STATE_PLAYING;
}

bool GStreamerPlayer::paused() const
{
    return gst_element_get_state(m_pipeline) == GST_STATE_PAUSED;
}

int GStreamerPlayer::totalTime() const
{
    return time(GST_QUERY_TOTAL) / GST_SECOND;
}

int GStreamerPlayer::currentTime() const
{
    return time(GST_QUERY_POSITION) / GST_SECOND;
}

int GStreamerPlayer::position() const
{
    long long total   = time(GST_QUERY_TOTAL);
    long long current = time(GST_QUERY_POSITION);
    return total > 0 ? int((double(current) / double(total)) * double(1000) + 0.5) : 0;
}

void GStreamerPlayer::seek(int seekTime)
{
    int type = (GST_FORMAT_TIME | GST_SEEK_METHOD_SET | GST_SEEK_FLAG_FLUSH);
    gst_element_seek(m_sink, GstSeekType(type), seekTime * GST_SECOND);
}

void GStreamerPlayer::seekPosition(int position)
{
    long long total = time(GST_QUERY_TOTAL);
    if(total > 0)
        seek(int(double(position) / double(1000) * double(totalTime()) + 0.5));
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

void GStreamerPlayer::readConfig()
{
    KConfigGroup config(KGlobal::config(), "GStreamerPlayer");
    m_sinkName = config.readEntry("SinkName", QString::null);
}

void GStreamerPlayer::setupPipeline()
{
    static bool initialized = false;
    if(!initialized) {
        int argc = kapp->argc();
        char **argv = kapp->argv();
        gst_init(&argc, &argv);
        initialized = true;
    }

    m_pipeline = gst_thread_new("pipeline");
    m_source   = gst_element_factory_make("filesrc", "source");
    m_decoder  = gst_element_factory_make("spider", "decoder");
    m_volume   = gst_element_factory_make("volume", "volume");

    if(!m_sinkName.isNull())
        m_sink = gst_element_factory_make(m_sinkName.utf8().data(), "sink");
    else {
        m_sink = gst_element_factory_make("alsasink", "sink");
        if(!m_sink)
            m_sink = gst_element_factory_make("osssink", "sink");            
    }
    

    gst_bin_add_many(GST_BIN(m_pipeline), m_source, m_decoder, m_volume, m_sink, 0);
    gst_element_link_many(m_source, m_decoder, m_volume, m_sink, 0);
}

long long GStreamerPlayer::time(GstQueryType type) const
{
    gint64 ns = 0;
    GstFormat format = GST_FORMAT_TIME;
    gst_element_query(m_sink, type, &format, &ns);
    return ns;
}

#else

/******************************************************************************/
/******************************************************************************/
/******************************  GSTREAMER 0.10  ******************************/
/******************************************************************************/
/******************************************************************************/

static GstBusSyncReply messageHandler(GstBus *, GstMessage *message, gpointer data)
{
    if(GST_MESSAGE_TYPE(message) == GST_MESSAGE_EOS) {
        GStreamerPlayer *player = static_cast<GStreamerPlayer *>(data);
        QTimer::singleShot(0, player, SLOT(stop()));
    }

    gst_message_unref(message);
    return GST_BUS_DROP;
}

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

GStreamerPlayer::GStreamerPlayer() :
    Player(),
    m_playbin(0)
{
    setupPipeline();
}

GStreamerPlayer::~GStreamerPlayer()
{
    stop();
    gst_object_unref(GST_OBJECT(m_playbin));
}

void GStreamerPlayer::play(const FileHandle &file)
{
    if(!file.isNull()) {
        stop();
        gchar *uri = g_filename_to_uri(file.absFilePath().local8Bit().data(), NULL, NULL);
        g_object_set(G_OBJECT(m_playbin), "uri", uri, JUK_GLIB_NULL);
    }

    gst_element_set_state(m_playbin, GST_STATE_PLAYING);
}

void GStreamerPlayer::pause()
{
    gst_element_set_state(m_playbin, GST_STATE_PAUSED);
}

void GStreamerPlayer::stop()
{
    gst_element_set_state(m_playbin, GST_STATE_NULL);
}

void GStreamerPlayer::setVolume(float volume)
{
    g_object_set(G_OBJECT(m_playbin), "volume", volume, JUK_GLIB_NULL);
}

float GStreamerPlayer::volume() const
{
    gdouble value;
    g_object_get(G_OBJECT(m_playbin), "volume", &value, JUK_GLIB_NULL);
    return (float) value;
}

bool GStreamerPlayer::playing() const
{
    return state() == GST_STATE_PLAYING;
}

bool GStreamerPlayer::paused() const
{
    return state() == GST_STATE_PAUSED;
}

int GStreamerPlayer::totalTime() const
{
    return time(TotalLength) / GST_SECOND;
}

int GStreamerPlayer::currentTime() const
{
    return time(CurrentPosition) / GST_SECOND;
}

int GStreamerPlayer::position() const
{
    long long total   = time(TotalLength);
    long long current = time(CurrentPosition);
    return total > 0 ? int((double(current) / double(total)) * double(1000) + 0.5) : 0;
}

void GStreamerPlayer::seek(int seekTime)
{
    gst_element_seek(m_playbin, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
                     GST_SEEK_TYPE_SET, seekTime * GST_SECOND, GST_SEEK_TYPE_END, 0);
}

void GStreamerPlayer::seekPosition(int position)
{
    gint64 time = gint64((double(position) / double(1000) * double(totalTime())
                          + 0.5) * double(GST_SECOND));
    gst_element_seek(m_playbin, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
                     GST_SEEK_TYPE_SET, time, GST_SEEK_TYPE_END, 0);
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

void GStreamerPlayer::setupPipeline()
{
    static bool initialized = false;
    if(!initialized) {
        int argc = kapp->argc();
        char **argv = kapp->argv();
        gst_init(&argc, &argv);
        initialized = true;
    }

    m_playbin = gst_element_factory_make("playbin", "playbin");
    gst_bus_set_sync_handler(gst_pipeline_get_bus(GST_PIPELINE(m_playbin)), messageHandler, this);
}

long long GStreamerPlayer::time(TimeQuery type) const
{
    GstQuery *query = (type == CurrentPosition)
        ? gst_query_new_position(GST_FORMAT_TIME)
        : gst_query_new_duration(GST_FORMAT_TIME);

    gint64 ns = 0;
    GstFormat format;

    if(gst_element_query(m_playbin, query))
    {
        if(type == CurrentPosition)
            gst_query_parse_position(query, &format, &ns);
        else
            gst_query_parse_duration(query, &format, &ns);
    }

    gst_query_unref(query);

    return ns;
}

GstState GStreamerPlayer::state() const
{
    GstState state;
    gst_element_get_state(m_playbin, &state, NULL, GST_CLOCK_TIME_NONE);
    return state;
}

#endif

#include "gstreamerplayer.moc"
#endif

// vim: set et sw=4:
