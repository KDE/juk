/***************************************************************************
                          tageditor.h  -  description
                             -------------------
    begin                : Sat Sep 7 2002
    copyright            : (C) 2002, 2003 by Scott Wheeler
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

#ifndef TAGEDITOR_H
#define TAGEDITOR_H

#include <qvaluelist.h>
#include <qwidget.h>

class KComboBox;
class KLineEdit;
class KIntSpinBox;
class KEdit;
class KPushButton;
class KConfig;

class QCheckBox;
class QBoxLayout;

class Playlist;
class PlaylistItem;
typedef QValueList<PlaylistItem *> PlaylistItemList;

class TagEditor : public QWidget
{
    Q_OBJECT

public: 
    TagEditor(QWidget *parent = 0, const char *name = 0);
    virtual ~TagEditor();
    void save() { save(m_items); }
    PlaylistItemList items() const { return m_items; }

public slots:
    void slotSetItems(const PlaylistItemList &list);
    void slotRefresh();
    void slotClear();
    void slotUpdateCollection();
   
private:
    void setupLayout();
    void readConfig();
    void readCompletionMode(KConfig *config, KComboBox *box, const QString &key);
    void saveConfig();
    void save(const PlaylistItemList &list);
    void saveChangesPrompt();
    /**
     * Adds an item to JuK's tagging layout.  This handles the creation and
     * placement of the "enable" box as well.
     */
    void addItem(const QString &text, QWidget *item, QBoxLayout *layout, const QString &iconName = QString::null);

    /**
     * Adds a widget to m_hideList and returns that widget.
     */
    QWidget *addHidden(QWidget *w) { m_hideList.append(w); return w; }

    virtual void showEvent(QShowEvent *e);

    virtual bool eventFilter(QObject *watched, QEvent *e);

private slots:
    void slotDataChanged(bool c = true);
    void slotItemRemoved(PlaylistItem *item);

private:
    typedef QMap<QWidget *, QCheckBox *> BoxMap;
    BoxMap m_enableBoxes;

    QStringList m_genreList;

    KComboBox *m_artistNameBox;
    KLineEdit *m_trackNameBox;
    KComboBox *m_albumNameBox;
    KComboBox *m_genreBox;
    KLineEdit *m_fileNameBox;
    KIntSpinBox *m_trackSpin;
    KIntSpinBox *m_yearSpin;
    KLineEdit *m_lengthBox;
    KLineEdit *m_bitrateBox;
    KEdit *m_commentBox;

    QValueList<QWidget *> m_hideList;

    PlaylistItemList m_items;
    Playlist *m_currentPlaylist;
    
    bool m_dataChanged; 
};

#endif
