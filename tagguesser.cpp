/**
 * Copyright (C) 2003 Frerich Raabe <raabe@kde.org>
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

#include "tagguesser.h"

#include <algorithm>
#include <utility>

#include <KConfig>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KMacroExpander>

#include <QHash>

using namespace Qt::Literals::StringLiterals;

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

    m_regExpMatch = m_regExp.match(stripped);
    return m_regExpMatch.hasMatch();
}

QString FileNameScheme::title() const
{
    if(m_titleField == -1)
        return QString();
    return m_regExpMatch.capturedTexts().at(m_titleField);
}

QString FileNameScheme::artist() const
{
    if(m_artistField == -1)
        return QString();
    return m_regExpMatch.capturedTexts().at(m_artistField);
}

QString FileNameScheme::album() const
{
    if(m_albumField == -1)
        return QString();
    return m_regExpMatch.capturedTexts().at(m_albumField);
}

QString FileNameScheme::track() const
{
    if(m_trackField == -1)
        return QString();
    return m_regExpMatch.capturedTexts().at(m_trackField);
}

QString FileNameScheme::comment() const
{
    if(m_commentField == -1)
        return QString();
    return m_regExpMatch.capturedTexts().at(m_commentField);
}

QString FileNameScheme::composeRegExp(const QString &s) const
{
    QHash<QChar, QString> substitutions;

    KConfigGroup config(KSharedConfig::openConfig(), u"TagGuesser"_s);

    substitutions[ 't' ] = config.readEntry("Title regexp", "([\\w\\s'&_,.]+)");
    substitutions[ 'a' ] = config.readEntry("Artist regexp", "([\\w\\s'&_,.]+)");
    substitutions[ 'A' ] = config.readEntry("Album regexp", "([\\w\\s'&_,.]+)");
    substitutions[ 'T' ] = config.readEntry("Track regexp", "(\\d+)");
    substitutions[ 'c' ] = config.readEntry("Comment regexp", "([\\w\\s_]+)");

    QString regExp = s.simplified()
        .replace("(", "\\(")
        .replace(")", "\\)")
        .replace("[", "\\[")
        .replace("]", "\\]");
    regExp.replace(' ', "\\s+");
    regExp = KMacroExpander::expandMacros(regExp, substitutions);
    regExp += "[^\\/]*$";
    return regExp.replace("\\\\", "\\");
}

QStringList TagGuesser::schemeStrings()
{
    QStringList schemes;

    KConfigGroup config(KSharedConfig::openConfig(), u"TagGuesser"_s);
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
    KSharedConfig::Ptr cfg = KSharedConfig::openConfig();
    KConfigGroup group(cfg, u"TagGuesser"_s);
    group.writeEntry("Filename schemes", schemes);
    cfg->sync();
}

TagGuesser::TagGuesser()
{
    for(const QString &scheme : schemeStrings())
        m_schemes << FileNameScheme(scheme);
}

TagGuesser::TagGuesser(const QString &absFileName)
  : TagGuesser()
{
    guess(absFileName);
}

void TagGuesser::guess(const QString &absFileName)
{
    m_title.clear();
    m_artist.clear();
    m_album.clear();
    m_track.clear();
    m_comment.clear();

    const auto it = std::find_if(m_schemes.cbegin(), m_schemes.cend(),
            [absFileName](const auto &scheme) { return scheme.matches(absFileName); });
    if(it == m_schemes.cend()) {
        return;
    }

    const auto &scheme = *it;
    m_title = capitalizeWords(scheme.title().replace('_', " ")).trimmed();
    m_artist = capitalizeWords(scheme.artist().replace('_', " ")).trimmed();
    m_album = capitalizeWords(scheme.album().replace('_', " ")).trimmed();
    m_track = scheme.track().trimmed();
    m_comment = scheme.comment().replace('_', " ").trimmed();
}

QString TagGuesser::capitalizeWords(const QString &s)
{
    if(s.isEmpty())
        return s;

    QString result = s;
    result[ 0 ] = result[ 0 ].toUpper();

    static const QRegularExpression wordRegExp("\\s\\w");
    int i = result.indexOf( wordRegExp );
    while ( i > -1 ) {
        result[ i + 1 ] = result[ i + 1 ].toUpper();
        i = result.indexOf( wordRegExp, ++i );
    }

    return result;
}

// vim: set et sw=4 tw=0 sta:
