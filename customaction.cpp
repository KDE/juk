/***************************************************************************
                           customaction.cpp  -  description
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

#include "customaction.h"

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

CustomAction::CustomAction(const QString &text, QObject *parent, const char *name)
  : KAction(text, 0, parent, name)
{
  toolbar=NULL;
}

CustomAction::~CustomAction()
{
}

int CustomAction::plug(QWidget *parent, int index)
{
  customWidget = createWidget(parent);

  if(customWidget) {
    // the check for null makes sure that there is only one toolbar that this is
    // "plugged" in to
    if (parent->inherits("KToolBar") && !toolbar) {
      toolbar = static_cast<KToolBar *>(parent);
      int id = KAction::getToolButtonID();
      
      toolbar->insertWidget(id, customWidget->width(), customWidget, index);
      
      addContainer(toolbar, id);
      connect(toolbar, SIGNAL(destroyed()), this, SLOT(slotDestroyed()));
      
      return (containerCount() - 1);
    }
    
    return(-1);
  }
  else {
    return(-1);
  }
}


void CustomAction::unplug(QWidget *parent)
{
  if (parent->inherits("KToolBar")) {
    toolbar = static_cast<KToolBar *>(parent);
    
    int index = findContainer(toolbar);
    if (index != -1) {
      toolbar->removeItem(itemId(index));
      removeContainer(index);

      toolbar = 0;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

KToolBar *CustomAction::getToolBar()
{
  return(toolbar);
}
