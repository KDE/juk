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
#include <kdebug.h>
#include <kglobal.h>
#include <kmacroexpander.h>

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
    /* Strip extension ('.mp3') because '.' may be part of a title, and thus
     * does not work as a separator.
     */
    QString stripped = fileName;
    stripped.truncate(stripped.findRev('.'));
    return m_regExp.exactMatch(stripped);
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

    KConfigGroup config(KGlobal::config(), "TagGuesser");

    substitutions[ 't' ] = config.readEntry("Title regexp", "([\\w\\s'&_,\\.]+)");
    substitutions[ 'a' ] = config.readEntry("Artist regexp", "([\\w\\s'&_,\\.]+)");
    substitutions[ 'A' ] = config.readEntry("Album regexp", "([\\w\\s'&_,\\.]+)");
    substitutions[ 'T' ] = config.readEntry("Track regexp", "(\\d+)");
    substitutions[ 'c' ] = config.readEntry("Comment regexp", "([\\w\\s_]+)");

    QString regExp = QRegExp::escape(s.simplifyWhiteSpace());
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
    schemes = config.readListEntry("Filename schemes");

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
    KConfig *cfg = kapp->config();
    {
        KConfigGroupSaver saver(cfg, "TagGuesser");
        cfg->writeEntry("Filename schemes", schemes);
    }
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
    m_title = m_artist = m_album = m_track = m_comment = QString::null;

    FileNameScheme::List::ConstIterator it = m_schemes.begin();
    FileNameScheme::List::ConstIterator end = m_schemes.end();
    for (; it != end; ++it) {
        const FileNameScheme schema(*it);
        if(schema.matches(absFileName)) {
            m_title = capitalizeWords(schema.title().replace('_', " ")).stripWhiteSpace();
            m_artist = capitalizeWords(schema.artist().replace('_', " ")).stripWhiteSpace();
            m_album = capitalizeWords(schema.album().replace('_', " ")).stripWhiteSpace();
            m_track = schema.track().stripWhiteSpace();
            m_comment = schema.comment().replace('_', " ").stripWhiteSpace();
            break;
        }
    }
}

QString TagGuesser::capitalizeWords(const QString &s)
{
    if(s.isEmpty())
        return s;

    QString result = s;
    result[ 0 ] = result[ 0 ].upper();

    const QRegExp wordRegExp("\\s\\w");
    int i = result.find( wordRegExp );
    while ( i > -1 ) {
        result[ i + 1 ] = result[ i + 1 ].upper();
        i = result.find( wordRegExp, ++i );
    }

    return result;
}

// vim:ts=4:sw=4:noet
