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

#ifndef FILEHANDLEPROPERTIES_H
#define FILEHANDLEPROPERTIES_H

#include <QMap>
#include <QStringList>

/*
 * These ugly macros make possible a property registration system that makes it
 * easy to add properties to the FileHandle that can be accessed via the DBus
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
            virtual QString value(const FileHandle &f) const override final         \
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
            virtual QString value(const FileHandle &f) const override final         \
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
        virtual ~Property() {}
        virtual QString value(const FileHandle &) const
        {
            return QString();
        }
    };

    static QMap<QByteArray, const Property *> propertyMap;

    static int addToPropertyMap(const QByteArray &name, Property *property)
    {
        propertyMap[name] = property;
        return 0;
    }

    static QString property(const FileHandle &file, const QByteArray &key)
    {
        return propertyMap.contains(key) ? propertyMap[key]->value(file) : QString();
    }

    static QStringList properties()
    {
        static QStringList l;

        if(l.isEmpty()) {
            QMap<QByteArray, const Property *>::ConstIterator it = propertyMap.constBegin();
            for(; it != propertyMap.constEnd(); ++it)
                l.append(QString(it.key()));
        }
        return l;
    }
}

#endif

// vim: set et sw=4 tw=0 sta:
