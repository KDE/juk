/***************************************************************************
    begin                : Tue Mar 11 19:00:00 CET 2003
    copyright            : (C) 2003 by Stefan Asserhall
    email                : stefan.asserhall@telia.com
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KEYDIALOG_H
#define KEYDIALOG_H

#include <kdeversion.h>
#include <kglobalaccel.h>
#include <kkeydialog.h>

#include <qhbuttongroup.h>
#include <qwidgetstack.h>

class KeyDialog : public KDialogBase
{
    Q_OBJECT

public:
    /**
     * Constructs a KeyDialog called @p name as a child of @p parent.
     */
    KeyDialog(KGlobalAccel *keys, KActionCollection *actionCollection, QWidget *parent = 0, const char* name = 0);

    /**
     * Destructor. Deletes all resources used by a KeyDialog object.
     */
    virtual ~KeyDialog();

    /**
     * This is a member function, provided to allow inserting both global
     * accelerators and actions. It behaves essentially like the functions
     * in KKeyDialog.
     */
    static int configure(KGlobalAccel *keys, KActionCollection *actionCollection, QWidget *parent = 0);

    /**
     * This is a member function, provided to create a global accelerator with
     * standard keys. It behaves like the function in KGlobalAccel.
     */
    static void insert(KGlobalAccel *keys, const QString &action, const QString &label,
                       const QObject *objSlot, const char *methodSlot);

private:

    /**
     * Groups of keys that can be selected in the dialog.
     */
    enum KeyGroup { NoKeys = 0, StandardKeys = 1, MultimediaKeys = 2 };

    struct KeyInfo {
        QString action;
        KShortcut shortcut[3][2];
    };

    void newDialog(KGlobalAccel *keys, KActionCollection *actionCollection, int selectedButton = 0);
    int configure();

private slots:
    void slotKeys(int group);
    void slotDefault();

private:
    KActionCollection *m_actionCollection;
    KGlobalAccel      *m_keys;
    KKeyChooser       *m_pKeyChooser;
    QHButtonGroup     *m_group;
    QWidgetStack      *m_widgetStack;

    static const KeyInfo keyInfo[];
    static const uint    keyInfoCount;
};

#endif // KEYDIALOG_H
