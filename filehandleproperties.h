/***************************************************************************
  Copyright (C) 2004 by Scott Wheeler <wheeler@kde.org>
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef FILEHANDLEPROPERTIES_H
#define FILEHANDLEPROPERTIES_H

#include <qmap.h>

/*
 * These ugly macros make possible a property registration system that makes it
 * easy to add properties to the FileHandle that can be accessed via the DCOP
 * interface.
 *
 * Properties should actually be added to the filehandle.cpp file.  This file
 * is just here to separate out some of the macro-related ugliness.
 */

#define AddProperty(name, method)                                                   \
    namespace FileHandleProperties                                                  \
    {                                                                               \
        struct name##Property : public Property                                     \
        {                                                                           \
            virtual QString value(const FileHandle &f) const                        \
            {                                                                       \
                return f.method;                                                    \
            }                                                                       \
            static const int dummy;                                                 \
        };                                                                          \
        static name##Property name##Instance;                                       \
        const int name##Property::dummy = addToPropertyMap(#name, &name##Instance); \
    }

#define AddNumberProperty(name, method)                                             \
    namespace FileHandleProperties                                                  \
    {                                                                               \
        struct name##Property : public Property                                     \
        {                                                                           \
            virtual QString value(const FileHandle &f) const                        \
            {                                                                       \
                return QString::number(f.method);                                   \
            }                                                                       \
            static const int dummy;                                                 \
        };                                                                          \
        static name##Property name##Instance;                                       \
        const int name##Property::dummy = addToPropertyMap(#name, &name##Instance); \
    }

namespace FileHandleProperties
{
    struct Property
    {
        virtual QString value(const FileHandle &) const
        {
            return QString::null;
        }
    };

    static QMap<QCString, const Property *> propertyMap;

    static int addToPropertyMap(const QCString &name, Property *property)
    {
        propertyMap[name] = property;
        return 0;
    }

    static QString property(const FileHandle &file, const QCString &key)
    {
        return propertyMap.contains(key) ? propertyMap[key]->value(file) : QString::null;
    }

    static QStringList properties()
    {
        static QStringList l;

        if(l.isEmpty()) {
            QMap<QCString, const Property *>::ConstIterator it = propertyMap.begin();
            for(; it != propertyMap.end(); ++it)
                l.append(QString(it.key()));
        }
        return l;
    }
}

#endif
