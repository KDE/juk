/***************************************************************************
    begin                : Fri Feb 27 2004
    copyright            : (C) 2004 by Scott Wheeler
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

#include "actioncollection.h"

#include <kactioncollection.h>
#include <kdebug.h>

namespace ActionCollection
{
    KActionCollection *actions()
    {
        static KActionCollection *a =
            new KActionCollection(static_cast<QObject *>(0));
        // The widget of the action collection is set in Juk::setupActions().
        return a;
    }

    QAction *action(const QString &key)
    {
#ifndef NO_DEBUG
        QAction *a = actions()->action(key);
        if(!a)
            kWarning(65432) << "KAction \"" << key << "\" is not defined yet." << endl;
        return a;
#else
        return actions()->action(key);
#endif
    }
}

// vim: set et sw=4 tw=0 sta:
