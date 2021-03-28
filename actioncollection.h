/**
 * Copyright (C) 2004 Scott Wheeler <wheeler@kde.org>
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

#ifndef ACTIONCOLLECTION_H
#define ACTIONCOLLECTION_H

class KActionCollection;
class QAction;
class QString;

#include <cstddef>

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
    QAction *action(const QString &key);

    /**
     * Returns the action for the associated key but includes a cast to the
     * type \a T.  i.e. KSelectAction *a = action<KSelectAction>("chooser");
     */
    template <class T> T *action(const QString &key)
    {
        return dynamic_cast<T *>(action(key));
    }

    /**
     * What ActionCollection::action does, in shorter form.
     */
    QAction *operator "" _act(const char *str, std::size_t len);
}

#endif

// vim: set et sw=4 tw=0 sta:
