/***********************************************************************
 * Copyright 2012  Eike Hein <hein@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************/

#ifndef JUK_MEDIAPLAYER2PLAYER_H
#define JUK_MEDIAPLAYER2PLAYER_H

#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>
#include <QPointer>

#include "playermanager.h"

class MediaPlayer2Player : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Player") // Docs: https://specifications.freedesktop.org/mpris-spec/latest/Media_Player.html

    Q_PROPERTY(QString PlaybackStatus READ PlaybackStatus NOTIFY playbackStatusChanged)
    Q_PROPERTY(QString LoopStatus READ LoopStatus CONSTANT)

    // These NOTIFYs are a lie for now, we don't support changes, but MPRIS
    // spec says this is a read/write property
    Q_PROPERTY(double Rate READ Rate WRITE setRate NOTIFY metadataChanged)
    Q_PROPERTY(bool Shuffle READ Shuffle WRITE setShuffle NOTIFY metadataChanged)

    Q_PROPERTY(QVariantMap Metadata READ Metadata NOTIFY metadataChanged)
    Q_PROPERTY(double Volume READ Volume WRITE setVolume NOTIFY signalVolumeChanged)
    Q_PROPERTY(qlonglong Position READ Position NOTIFY Seeked)
    Q_PROPERTY(double MinimumRate READ MinimumRate CONSTANT)
    Q_PROPERTY(double MaximumRate READ MaximumRate CONSTANT)
    Q_PROPERTY(bool CanGoNext READ CanGoNext CONSTANT)
    Q_PROPERTY(bool CanGoPrevious READ CanGoPrevious CONSTANT)
    Q_PROPERTY(bool CanPlay READ CanPlay CONSTANT)
    Q_PROPERTY(bool CanPause READ CanPause CONSTANT)
    Q_PROPERTY(bool CanSeek READ CanSeek NOTIFY canSeekChanged)
    Q_PROPERTY(bool CanControl READ CanControl CONSTANT)

    public:
        explicit MediaPlayer2Player(QObject* parent);
        ~MediaPlayer2Player();

        QString PlaybackStatus() const;
        QString LoopStatus() const;
        Q_NOREPLY void setLoopStatus(const QString& loopStatus) const;
        double Rate() const;
        Q_NOREPLY void setRate(double rate) const;
        bool Shuffle() const;
        void setShuffle(bool shuffle) const;
        QVariantMap Metadata() const;
        double Volume() const;
        Q_NOREPLY void setVolume(double volume) const;
        qlonglong Position() const;
        double MinimumRate() const;
        double MaximumRate() const;
        bool CanGoNext() const;
        bool CanGoPrevious() const;
        bool CanPlay() const;
        bool CanPause() const;
        bool CanSeek() const;
        bool CanControl() const;

    signals:
        void Seeked(qlonglong Position);
        void playbackStatusChanged();
        void metadataChanged();
        void signalVolumeChanged();
        void canSeekChanged();

    public slots:
        Q_NOREPLY void Next() const;
        Q_NOREPLY void Previous() const;
        Q_NOREPLY void Pause() const;
        Q_NOREPLY void PlayPause() const;
        Q_NOREPLY void Stop() const;
        Q_NOREPLY void Play() const;
        Q_NOREPLY void Seek(qlonglong Offset) const;
        Q_NOREPLY void SetPosition(const QDBusObjectPath& TrackId, qlonglong Position) const;
        Q_NOREPLY void OpenUri(QString Uri) const;

    private slots:
        void currentSourceChanged() const;
        void stateUpdated() const;
        void totalTimeChanged() const;
        void seekableChanged(bool seekable) const;
        void volumeChanged(float newVol) const;
        void slotSeeked(int newPos);

    private:
        void signalPropertiesChange(const QVariantMap& properties) const;

        int oldPos;
        QPointer<PlayerManager> m_player;
};

#endif
