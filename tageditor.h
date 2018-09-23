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

#ifndef JUK_TAGEDITOR_H
#define JUK_TAGEDITOR_H

#include <QWidget>
#include <QList>
#include <QMap>

#include "ui_tageditor.h"

class KComboBox;
class KLineEdit;
class KIntSpinBox;
class KTextEdit;
class KConfigGroup;

class QCheckBox;
class QBoxLayout;

class CollectionObserver;
class Playlist;
class PlaylistItem;

typedef QList<PlaylistItem *> PlaylistItemList;

class TagEditor : public QWidget, public Ui::TagEditor
{
    Q_OBJECT

public:
    explicit TagEditor(QWidget *parent = 0);
    virtual ~TagEditor();
    PlaylistItemList items() const { return m_items; }
    void setupObservers();

public slots:
    void slotSave() { save(m_items); }
    void slotSetItems(const PlaylistItemList &list);
    void slotRefresh();
    void slotClear();
    void slotPlaylistDestroyed(Playlist *p);
    /**
     * Update collection if we're visible, or defer otherwise
     */
    void slotUpdateCollection();

private:
    void updateCollection();

    void setupActions();
    void setupLayout();
    void readConfig();
    void readCompletionMode(const KConfigGroup &config, KComboBox *box, const QString &key);
    void saveConfig();
    void save(const PlaylistItemList &list);
    void saveChangesPrompt();
    virtual void showEvent(QShowEvent *e);

private slots:
    void slotDataChanged();
    void slotItemRemoved(PlaylistItem *item);
    void slotPlaylistRemoved() { m_currentPlaylist = 0; }

private:
    typedef QMap<QWidget *, QCheckBox *> BoxMap;
    BoxMap m_enableBoxes;

    QStringList m_genreList;

    PlaylistItemList m_items;
    Playlist *m_currentPlaylist;

    CollectionObserver *m_observer;

    bool m_dataChanged;
    bool m_collectionChanged;
    bool m_performingSave;

    friend class CollectionObserver;
};

#endif

// vim: set et sw=4 tw=0 sta:
