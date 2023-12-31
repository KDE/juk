/**
 * Copyright (C) 2002-2004 Scott Wheeler <wheeler@kde.org>
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

#ifndef JUK_H
#define JUK_H

#include <kxmlguiwindow.h>

class KToggleAction;
class KGlobalAccel;

class SliderAction;
class StatusLabel;
class SystemTray;
class PlayerManager;
class PlaylistSplitter;
class Scrobbler;

class JuK : public KXmlGuiWindow
{
    Q_OBJECT

public:
    explicit JuK(const QStringList &filesToOpen, QWidget* parent = 0);
    virtual ~JuK();

    static JuK* JuKInstance();

    PlayerManager *playerManager() const;

private:
    void setupLayout();
    void setupActions();
    void setupGlobalAccels();

    void keyPressEvent(QKeyEvent *) override;

    void activateScrobblerIfEnabled();

    void readConfig();
    void saveConfig();

    virtual bool queryClose() override;

private slots:
    void slotSetupSystemTray();
    void slotShowHide();
    void slotQuit();
    void slotToggleSystemTray(bool enabled);
    void slotEditKeys();
    void slotConfigureTagGuesser();
    void slotConfigureFileRenamer();
    void slotConfigureScrobbling();
    void slotUndo();
    void slotCheckAlbumNextAction(bool albumRandomEnabled);
    void slotProcessArgs();
    void slotClearOldCovers();

private:
    PlaylistSplitter *m_splitter = nullptr;
    StatusLabel *m_statusLabel   = nullptr;
    SystemTray *m_systemTray     = nullptr;

    KToggleAction *m_randomPlayAction        = nullptr;
    KToggleAction *m_toggleSystemTrayAction  = nullptr;
    KToggleAction *m_toggleDockOnCloseAction = nullptr;
    KToggleAction *m_togglePopupsAction      = nullptr;

    PlayerManager *m_player     = nullptr;
    Scrobbler     *m_scrobbler  = nullptr;

    QStringList m_filesToOpen;
    bool m_shuttingDown = false;
    uint m_pmToken      = 0;

    static JuK* m_instance;
};

#endif

// vim: set et sw=4 tw=0 sta:
