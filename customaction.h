/***************************************************************************
                           customaction.h  -  description
                             -------------------
    begin                : Wed Feb 6 2002
    copyright            : (C) 2002 by Scott Wheeler
    email                : scott@slackorama.net
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
#include <ktoolbar.h>

#include <qstring.h>
#include <qobject.h>

class CustomAction : public KAction
{
    Q_OBJECT
public:
    CustomAction(const QString &text, QObject *parent, const char *name);
    ~CustomAction();

    virtual int plug(QWidget *parent, int index = -1);
    virtual void unplug(QWidget *widget);

protected:
    KToolBar *getToolBar();

    QWidget *customWidget;
    KToolBar *toolbar;

private:
    virtual QWidget *createWidget(QWidget *parent) = 0;

signals:
    void pluggedIn(QWidget *parent);

};

#endif
