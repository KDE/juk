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
        g_object_set(G_OBJECT(m_source), "location", file.absFilePath().local8Bit().data(), 0);
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
    g_object_set(G_OBJECT(m_volume), "volume", volume, 0);
}

float GStreamerPlayer::volume() const
{
    gfloat value;
    g_object_get(G_OBJECT(m_volume), "volume", &value, 0);
    return value;
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

#include "gstreamerplayer.moc"
#endif

// vim: set et sw=4:
