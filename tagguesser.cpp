/*
 * tagguesser.cpp - Copyright (c) 2003 Frerich Raabe <raabe@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include "tagguesser.h"

#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kmacroexpander.h>
#include <qhash.h>
#include <kconfiggroup.h>

FileNameScheme::FileNameScheme(const QString &s)
    : m_regExp(),
    m_titleField(-1),
    m_artistField(-1),
    m_albumField(-1),
    m_trackField(-1),
    m_commentField(-1)
{
    int fieldNumber = 1;
    int i = s.indexOf('%');
    while (i > -1) {
        switch (s[ i + 1 ].toLatin1()) {
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
        i = s.indexOf('%', i + 1);
    }
    m_regExp.setPattern(composeRegExp(s));
}

bool FileNameScheme::matches(const QString &fileName) const
{
    /* Strip extension ('.mp3') because '.' may be part of a title, and thus
     * does not work as a separator.
     */
    QString stripped = fileName;
    stripped.truncate(stripped.lastIndexOf('.'));
    return m_regExp.exactMatch(stripped);
}

QString FileNameScheme::title() const
{
    if(m_titleField == -1)
        return QString();
    return m_regExp.capturedTexts()[ m_titleField ];
}

QString FileNameScheme::artist() const
{
    if(m_artistField == -1)
        return QString();
    return m_regExp.capturedTexts()[ m_artistField ];
}

QString FileNameScheme::album() const
{
    if(m_albumField == -1)
        return QString();
    return m_regExp.capturedTexts()[ m_albumField ];
}

QString FileNameScheme::track() const
{
    if(m_trackField == -1)
        return QString();
    return m_regExp.capturedTexts()[ m_trackField ];
}

QString FileNameScheme::comment() const
{
    if(m_commentField == -1)
        return QString();
    return m_regExp.capturedTexts()[ m_commentField ];
}

QString FileNameScheme::composeRegExp(const QString &s) const
{
    QHash<QChar, QString> substitutions;

    KConfigGroup config(KGlobal::config(), "TagGuesser");

    substitutions[ 't' ] = config.readEntry("Title regexp", "([\\w\\s'&_,\\.]+)");
    substitutions[ 'a' ] = config.readEntry("Artist regexp", "([\\w\\s'&_,\\.]+)");
    substitutions[ 'A' ] = config.readEntry("Album regexp", "([\\w\\s'&_,\\.]+)");
    substitutions[ 'T' ] = config.readEntry("Track regexp", "(\\d+)");
    substitutions[ 'c' ] = config.readEntry("Comment regexp", "([\\w\\s_]+)");

    QString regExp = QRegExp::escape(s.simplified());
    regExp = ".*" + regExp;
    regExp.replace(' ', "\\s+");
    regExp = KMacroExpander::expandMacros(regExp, substitutions);
    regExp += "[^/]*$";
    return regExp;
}

QStringList TagGuesser::schemeStrings()
{
    QStringList schemes;

    KConfigGroup config(KGlobal::config(), "TagGuesser");
    schemes = config.readEntry("Filename schemes", QStringList());

    if ( schemes.isEmpty() ) {
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
        schemes += "%a/%A/[%T] %t [%c]";
        schemes += "%a/%A/[%T] %t (%c)";
        schemes += "%a/%A/[%T] %t";
    }
    return schemes;
}

void TagGuesser::setSchemeStrings(const QStringList &schemes)
{
    KSharedConfig::Ptr cfg = KGlobal::config();
    KConfigGroup group(cfg, "TagGuesser");
    group.writeEntry("Filename schemes", schemes);
    cfg->sync();
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
    m_title = m_artist = m_album = m_track = m_comment = QString();

    FileNameScheme::List::ConstIterator it = m_schemes.constBegin();
    FileNameScheme::List::ConstIterator end = m_schemes.constEnd();
    for (; it != end; ++it) {
        const FileNameScheme schema(*it);
        if(schema.matches(absFileName)) {
            m_title = capitalizeWords(schema.title().replace('_', " ")).trimmed();
            m_artist = capitalizeWords(schema.artist().replace('_', " ")).trimmed();
            m_album = capitalizeWords(schema.album().replace('_', " ")).trimmed();
            m_track = schema.track().trimmed();
            m_comment = schema.comment().replace('_', " ").trimmed();
            break;
        }
    }
}

QString TagGuesser::capitalizeWords(const QString &s)
{
    if(s.isEmpty())
        return s;

    QString result = s;
    result[ 0 ] = result[ 0 ].toUpper();

    const QRegExp wordRegExp("\\s\\w");
    int i = result.indexOf( wordRegExp );
    while ( i > -1 ) {
        result[ i + 1 ] = result[ i + 1 ].toUpper();
        i = result.indexOf( wordRegExp, ++i );
    }

    return result;
}

// vim: set et sw=4 tw=0 sta:
