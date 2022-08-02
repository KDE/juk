/**
 * Copyright (C) 2002 Daniel Molkentin <molkentin@kde.org>
 * Copyright (C) 2002-2004 Scott Wheeler <wheeler@kde.org>
 * Copyright (C) 2007, 2008, 2009 Michael Pyne <mpyne@kde.org>
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

#ifndef JUK_SYSTEMTRAY_H
#define JUK_SYSTEMTRAY_H

#include <KStatusNotifierItem>

#include <QVector>
#include <QColor>
#include <QPixmap>
#include <QIcon>
#include <QFrame>
#include <QVBoxLayout>

class SystemTray;
class PlayerManager;
class QLabel;
class QTimer;
class FileHandle;

/**
 * Workalike of KPassivePopup intended to more easily support JuK's particular
 * usage pattern, including things like staying open while under the mouse.
 *
 * @author Michael Pyne <mpyne@kde.org>
 */
class PassiveInfo final : public QFrame
{
    Q_OBJECT
public:
    PassiveInfo();

    // Sets view as a child widget to show in the popup window.
    // This widget does not take ownership of the widget.  If you want it auto-deleted,
    // either re-parent it or create it using this widget as its parent.
    void setView(QWidget *view);

    QWidget *view() const;

public slots:
    virtual void show();

signals:
    void mouseEntered();
    void timeExpired();
    void previousSong();
    void nextSong();

protected:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void enterEvent (QEnterEvent *e) override;
#else
    void enterEvent (QEvent *e) override;
#endif
    void leaveEvent(QEvent *) override;
    void wheelEvent(QWheelEvent *) override;

private:
    // Move us near the required position.
    void positionSelf();

private slots:
    void timerExpired();

private:
    QTimer *m_startFadeTimer;
    QVBoxLayout *m_layout;
    bool m_justDie;
};

class SystemTray final : public KStatusNotifierItem
{
    Q_OBJECT

public:
    explicit SystemTray(PlayerManager *player, QWidget *parent = nullptr);

signals:
    // Emitted when the fade process is complete.
    void fadeDone();

private:
    static const int STEPS = 40; ///< Number of intermediate steps for fading.

    void createPopup();
    void setToolTip(const QString &tip = QString(), const QPixmap &cover = QPixmap());

    // Creates the widget layout for the popup, returning the QWidget that
    // holds the text labels.

    QWidget *createInfoBox(QBoxLayout *parentLayout, const FileHandle &file);
    void addSeparatorLine(QBoxLayout *parentLayout);
    void addCoverButton(QBoxLayout *parentLayout, const QPixmap &cover);
    void createButtonBox(QBoxLayout *parentLayout);

    // Interpolates from start color to end color.  If @p step == 0, then
    // m_startColor is returned, while @p step == @steps returns
    // m_endColor.

    QColor interpolateColor(int step, int steps = STEPS);

private slots:
    void slotPlay();
    void slotPause();
    void slotStop();
    void slotPopupDestroyed();
    void slotNextStep(); ///< This is the fading routine.
    void slotPopupLargeCover();
    void slotForward();
    void slotBack();
    void slotFadeOut(); ///< Fades out the text
    void slotMouseInPopup(); ///< Forces the text back to its normal color.
    void scrollEvent(int delta, Qt::Orientation orientation);

private:
    PassiveInfo *m_popup = nullptr;
    PlayerManager *m_player;

    QIcon m_backPix;
    QIcon m_forwardPix;

    QVector<QLabel *> m_labels;

    QTimer *m_fadeStepTimer = nullptr;
    QColor m_startColor, m_endColor;
    int m_step = 0;
    bool m_fade = true;

    /// Used to choose between manual fade and windowOpacity
    bool m_hasCompositionManager = false;
};

#endif // JUK_SYSTEMTRAY_H

// vim: set et sw=4 tw=0 sta:
