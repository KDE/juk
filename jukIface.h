#ifndef JUKIFACE_H
#define JUKIFACE_H

#include <dcopobject.h>
#include <qstringlist.h>

class CollectionIface : public DCOPObject
{
    K_DCOP
k_dcop:
    void openFile(const QString &file) { open(file); }
    void openFile(const QStringList &files) { open(files); }
    void openFile(const QString &playlist, const QString &file) { open(playlist, file); }
    void openFile(const QString &playlist, const QStringList &files) { open(playlist, files); }

    virtual QStringList playlists() const = 0;
    virtual void createPlaylist(const QString &) = 0;
    virtual void remove() = 0;

    virtual void removeTrack(const QString &playlist, const QString &file) { removeTrack(playlist, file); }
    virtual void removeTrack(const QString &playlist, const QStringList &files) = 0;

    virtual QString playlist() const = 0;
    virtual void setPlaylist(const QString &playlist) = 0;

    virtual QStringList playlistTracks(const QString &playlist) const = 0;
    virtual QString trackProperty(const QString &file, const QString &property) const = 0;

protected:
    CollectionIface() : DCOPObject("Collection") {}
    virtual void open(const QStringList &files) = 0;
    virtual void open(const QString &playlist, const QStringList &files) = 0;
};

class PlayerIface : public DCOPObject
{
    K_DCOP
k_dcop:
    virtual bool playing() const = 0;
    virtual bool paused() const = 0;
    virtual float volume() const = 0;
    virtual int status() const = 0;
    
    virtual QStringList trackProperties() = 0;
    virtual QString trackProperty(const QString &property) const = 0;

    virtual void play() = 0;
    virtual void play(const QString &file) = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual void playPause() = 0;

    virtual void back() = 0;
    virtual void forward() = 0;
    virtual void seekBack() = 0;
    virtual void seekForward() = 0;

    virtual void volumeUp() = 0;
    virtual void volumeDown() = 0;
    virtual void mute() = 0;
    virtual void setVolume(float volume) = 0;
    virtual void seek(int time) = 0;

    virtual QString playingString() const = 0;
    virtual int currentTime() const = 0;
    virtual int totalTime() const = 0;

protected:
    PlayerIface() : DCOPObject("Player") {}
};

#endif
