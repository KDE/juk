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

#ifndef JUK_ACTIONCOLLECTION_H
#define JUK_ACTIONCOLLECTION_H

class KActionCollection;
class KAction;

namespace ActionCollection
{
    /**
     * The global action collection for JuK.
     */
    KActionCollection *actions();

    /**
     * Returns the action for the associated key from the global action
     * collection.
     */
    KAction *action(const char *key);

    /**
     * Returns the action for the associated key but includes a cast to the
     * type \a T.  i.e. KSelectAction *a = action<KSelectAction>("chooser");
     */
    template <class T> T *action(const char *key)
    {
        return dynamic_cast<T *>(action(key));
    }
}

#endif
