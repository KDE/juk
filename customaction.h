/***************************************************************************
                           customaction.h  -  description
                             -------------------
    begin                : Wed Feb 6 2002
    copyright            : (C) 2002 by Scott Wheeler
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

#ifndef CUSTOMACTION_H
#define CUSTOMACTION_H

#include <kaction.h>


// Many months after writing this, despite having felt rather clever at the time
// I am now rather convinced that this is The Wrong Way (tm) to have handled
// things and will hopefully un-hack this later.

class CustomAction : public KAction
{
    Q_OBJECT
public:
    CustomAction(const QString &text, QObject *parent, const char *name);
    virtual ~CustomAction();

    virtual int plug(QWidget *parent, int index = -1);
    virtual void unplug(QWidget *widget);

protected:
    KToolBar *toolBar() const { return m_toolBar; }

    KToolBar *m_toolBar;

signals:
    void pluggedIn(QWidget *parent);

private:
    virtual QWidget *createWidget(QWidget *parent) = 0;
};

#endif
