/*
 * tagguesser.cpp - (c) 2003 Frerich Raabe <raabe@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include "tagguesser.h"

#include <kapplication.h>
#include <kconfig.h>
#include <kdeversion.h>
#if KDE_VERSION > KDE_MAKE_VERSION(3,1,0)
#    include <kmacroexpander.h>
#endif

#include <qmap.h>
#include <qregexp.h>
#include <qurl.h>

FileNameScheme::FileNameScheme(const QString &s)
    : m_regExp(),
    m_titleField(-1),
    m_artistField(-1),
    m_albumField(-1),
    m_trackField(-1),
    m_commentField(-1)
{
    int fieldNumber = 1;
    int i = s.find('%');
    while (i > -1) {
        switch (s[ i + 1 ]) {
            case 't': m_titleField = fieldNumber++;
                      break;
            case 'a': m_artistField = fieldNumber++;
                      break;
            case 'A': m_albumField = fieldNumber++;
                      break;
            case 'T': m_trackField = fieldNumber++;
                      break;
            case 'c': m_commentField = fieldNumber++;
                      break;
            default:
                      break;
        }
        i = s.find('%', i + 1);
    }
    m_regExp.setPattern(composeRegExp(s));
}

bool FileNameScheme::matches(const QString &fileName) const
{
    return m_regExp.exactMatch(fileName);
}

QString FileNameScheme::title() const
{
    if(m_titleField == -1)
        return QString::null;
    return m_regExp.capturedTexts()[ m_titleField ];
}

QString FileNameScheme::artist() const
{
    if(m_artistField == -1)
        return QString::null;
    return m_regExp.capturedTexts()[ m_artistField ];
}

QString FileNameScheme::album() const
{
    if(m_albumField == -1)
        return QString::null;
    return m_regExp.capturedTexts()[ m_albumField ];
}

QString FileNameScheme::track() const
{
    if(m_trackField == -1)
        return QString::null;
    return m_regExp.capturedTexts()[ m_trackField ];
}

QString FileNameScheme::comment() const
{
    if(m_commentField == -1)
        return QString::null;
    return m_regExp.capturedTexts()[ m_commentField ];
}

QString FileNameScheme::composeRegExp(const QString &s) const
{
    QMap<QChar, QString> substitutions;
    substitutions[ 't' ] = "([\\w\\s']+)";
    substitutions[ 'a' ] = "([\\w\\s]+)";
    substitutions[ 'A' ] = "([\\w\\s]+)";
    substitutions[ 'T' ] = "(\\d+)";
    substitutions[ 'c' ] = "([\\w\\s]+)";

    QString regExp = QRegExp::escape(s.simplifyWhiteSpace());
    regExp = ".*" + regExp;
    regExp.replace(' ', "\\s+");
#if KDE_VERSION > KDE_MAKE_VERSION(3,1,0)
    KMacroExpander::expandMacros(regExp, substitutions);
#else
    QMap<QChar, QString>::ConstIterator it = substitutions.begin();
    QMap<QChar, QString>::ConstIterator end = substitutions.end();
    for (; it != end; ++it)
        regExp.replace("%" + QString(it.key()), it.data());
#endif
    regExp += "\\.[^\\.]+";
    return regExp;
}

QStringList TagGuesser::schemeStrings()
{
    QStringList schemes = kapp->config()->readListEntry( "Filename schemes" );
    if ( schemes.isEmpty() ) {
        schemes += "%a/%A/[%T] %t";
        schemes += "%a - (%T) - %t [%c]";
        schemes += "%a - (%T) - %t (%c)";
        schemes += "%a - (%T) - %t";
        schemes += "%a - [%T] - %t [%c]";
        schemes += "%a - [%T] - %t (%c)";
        schemes += "%a - [%T] - %t";
        schemes += "%a - %T - %t [%c]";
        schemes += "%a - %T - %t (%c)";
        schemes += "%a - %T - %t";
        schemes += "(%T) %a - %t [%c]";
        schemes += "(%T) %a - %t (%c)";
        schemes += "(%T) %a - %t";
        schemes += "[%T] %a - %t [%c]";
        schemes += "[%T] %a - %t (%c)";
        schemes += "[%T] %a - %t";
        schemes += "%T %a - %t [%c]";
        schemes += "%T %a - %t (%c)";
        schemes += "%T %a - %t";
        schemes += "(%a) %t [%c]";
        schemes += "(%a) %t (%c)";
        schemes += "(%a) %t";
        schemes += "%a - %t [%c]";
        schemes += "%a - %t (%c)";
        schemes += "%a - %t";
    }
    return schemes;
}

void TagGuesser::setSchemeStrings(const QStringList &schemes)
{
    kapp->config()->writeEntry("Filename schemes", schemes);
}

TagGuesser::TagGuesser()
{
    loadSchemes();
}

TagGuesser::TagGuesser(const QString &absFileName)
{
    loadSchemes();
    guess(absFileName);
}

void TagGuesser::loadSchemes()
{
    const QStringList schemes = schemeStrings();
    QStringList::ConstIterator it = schemes.begin();
    QStringList::ConstIterator end = schemes.end();
    for ( ; it != end; ++it )
        m_schemes += FileNameScheme( *it );
}

void TagGuesser::guess(const QString &absFileName)
{
    FileNameScheme::List::ConstIterator it = m_schemes.begin();
    FileNameScheme::List::ConstIterator end = m_schemes.end();
    for (; it != end; ++it) {
        const FileNameScheme schema(*it);
        if(schema.matches(absFileName)) {
            m_title = schema.title();
            m_artist = schema.artist();
            m_album = schema.album();
            m_track = schema.track();
            m_comment = schema.comment();
            break;
        }
    }
}

// vim:ts=4:sw=4:noet
