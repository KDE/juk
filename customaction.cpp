/***************************************************************************
                           customaction.cpp  -  description
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

#include <ktoolbar.h>

#include "customaction.h"

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

CustomAction::CustomAction(const QString &text, QObject *parent, const char *name)
    : KAction(text, 0, parent, name),
      m_toolBar(0)
{

}

CustomAction::~CustomAction()
{

}

int CustomAction::plug(QWidget *parent, int index)
{
    QWidget *w = createWidget(parent);

    if(!w)
	return -1;

    // the check for null makes sure that there is only one toolbar that this is
    // "plugged" in to

    if(parent->inherits("KToolBar") && !m_toolBar) {
	m_toolBar = static_cast<KToolBar *>(parent);
	int id = KAction::getToolButtonID();

	m_toolBar->insertWidget(id, w->width(), w, index);

	addContainer(m_toolBar, id);

	connect(m_toolBar, SIGNAL(destroyed()), this, SLOT(slotDestroyed()));

	return (containerCount() - 1);
    }

    return -1;
}


void CustomAction::unplug(QWidget *parent)
{
    if (parent->inherits("KToolBar")) {
        m_toolBar = static_cast<KToolBar *>(parent);

        int index = findContainer(m_toolBar);
        if (index != -1) {
            m_toolBar->removeItem(itemId(index));
            removeContainer(index);

            m_toolBar = 0;
        }
    }
}

#include "customaction.moc"
