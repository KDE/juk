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

#include "actioncollection.h"

#include <kactioncollection.h>

#include "juk.h"
#include "juk_debug.h"

namespace ActionCollection
{
    KActionCollection *actions()
    {
        // Use KXMLGUIClient::actionCollection() (class JuK derives from
        // KXMLGUIClient) to construct the KActionCollection.
        // This makes sure that KActionCollection::parentGUIClient() is not
        // NULL and prevents the application from crashing when adding an
        // item to a toolbar using RMB (see bug #258641).
        // XXX This should not just be:
        //      return JuK::JuKInstance()->actionCollection();
        // as actions() may be called while within JuK's dtor, in which case
        // JuKInstance()->... would result to a crash.
        static KActionCollection *a = JuK::JuKInstance()->actionCollection();
        // The widget of the action collection is set in Juk::setupActions().
        return a;
    }

    QAction *action(const QString &key)
    {
#ifndef NO_DEBUG
        QAction *a = actions()->action(key);
        if(!a)
            qCWarning(JUK_LOG) << "QAction \"" << key << "\" is not defined yet.";
        return a;
#else
        return actions()->action(key);
#endif
    }

    QAction *operator ""_act(const char *str, std::size_t len)
    {
        return action(QString::fromLatin1(str, len));
    }
}

// vim: set et sw=4 tw=0 sta:
