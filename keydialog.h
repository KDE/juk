/***************************************************************************
                          keydialog.h  -  description
                             -------------------
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

#ifndef __KEYDIALOG_H__
#define __KEYDIALOG_H__

#include <kactioncollection.h>
#include <kdialogbase.h>
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
    KeyDialog(KGlobalAccel* keys, KActionCollection* coll, QWidget *parent = 0, const char* name = 0);

    /**
     * Destructor. Deletes all resources used by a KeyDialog object.
     */
    virtual ~KeyDialog();

    /**
     * This is a member function, provided to allow inserting both global
     * accelerators and actions. It behaves essentially like the functions
     * in KKeyDialog.
     */
    static int configure(KGlobalAccel* keys, KActionCollection* coll, QWidget* parent = 0);

    /**
     * This is a member function, provided to create a global accelerator with
     * standard keys. It behaves like the function in KGlobalAccel.
     */
    static void insert(KGlobalAccel *keys, const QString& action, const QString& label,
		       const QObject* objSlot, const char* methodSlot);

private slots:
    void slotKeys(int group);
    void slotDefault();

private:
    void newDialog(KGlobalAccel* keys, KActionCollection* coll, int selectedButton = 0);
    int configure();

    KActionCollection *m_coll;
    KGlobalAccel      *m_keys;
    KKeyChooser       *m_pKeyChooser;
    QHButtonGroup     *m_group;
    QWidgetStack      *m_widgetstack;
};

#endif // __KEYDIALOG_H__
