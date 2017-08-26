// Copyright Frerich Raabe <raabe@kde.org>.
// This notice was added by Michael Pyne <michael.pyne@kdemail.net>
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "tagguesser.h"
#include <QTest>
#include <QDir>

class TagGuesserTest : public QObject
{
    Q_OBJECT

private slots:
    void testGuesser_data();
    void testGuesser();

private:
    void add(const QString &filename, const QString &title,
             const QString &artist, const QString &track,
             const QString &comment, const QString &album = QString());
};


void TagGuesserTest::testGuesser_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<QString>("title");
    QTest::addColumn<QString>("artist");
    QTest::addColumn<QString>("track");
    QTest::addColumn<QString>("comment");
    QTest::addColumn<QString>("album");

    add("/home/frerich/Chemical Brothers - (01) - Block rockin' beats [Live].mp3",
        "Block Rockin' Beats", "Chemical Brothers", "01", "Live");
    add("/home/frerich/Chemical Brothers - (01) - Block rockin' beats (Live).mp3",
        "Block Rockin' Beats", "Chemical Brothers", "01", "Live");
    add("/home/frerich/Chemical Brothers - (01) - Block rockin' beats.mp3",
        "Block Rockin' Beats", "Chemical Brothers", "01", QString());
    add("/home/frerich/Chemical Brothers - [01] - Block rockin' beats [Live].mp3",
        "Block Rockin' Beats", "Chemical Brothers", "01", "Live");
    add("/home/frerich/Chemical Brothers - [01] - Block rockin' beats (Live).mp3",
        "Block Rockin' Beats", "Chemical Brothers", "01", "Live");
    add("/home/frerich/Chemical Brothers - [01] - Block rockin' beats.mp3",
        "Block Rockin' Beats", "Chemical Brothers", "01", QString());
    add("/home/frerich/Chemical Brothers - 01 - Block rockin' beats [Live].mp3",
        "Block Rockin' Beats", "Chemical Brothers", "01", "Live");
    add("/home/frerich/Chemical Brothers - 01 - Block rockin' beats (Live).mp3",
        "Block Rockin' Beats", "Chemical Brothers", "01", "Live");
    add("/home/frerich/Chemical Brothers - 01 - Block rockin' beats.mp3",
        "Block Rockin' Beats", "Chemical Brothers", "01", QString());
    add("/home/frerich/(01) Chemical Brothers - Block rockin' beats [Live].mp3",
        "Block Rockin' Beats", "Chemical Brothers", "01", "Live");
    add("/home/frerich/(01) Chemical Brothers - Block rockin' beats (Live).mp3",
        "Block Rockin' Beats", "Chemical Brothers", "01", "Live");
    add("/home/frerich/(01) Chemical Brothers - Block rockin' beats.mp3",
        "Block Rockin' Beats", "Chemical Brothers", "01", QString());
    add("/home/frerich/[01] Chemical Brothers - Block rockin' beats [Live].mp3",
        "Block Rockin' Beats", "Chemical Brothers", "01", "Live");
    add("/home/frerich/[01] Chemical Brothers - Block rockin' beats (Live).mp3",
        "Block Rockin' Beats", "Chemical Brothers", "01", "Live");
    add("/home/frerich/[01] Chemical Brothers - Block rockin' beats.mp3",
        "Block Rockin' Beats", "Chemical Brothers", "01", QString());
    add("/home/frerich/01 Chemical Brothers - Block rockin' beats [Live].mp3",
        "Block Rockin' Beats", "Chemical Brothers", "01", "Live");
    add("/home/frerich/01 Chemical Brothers - Block rockin' beats (Live).mp3",
        "Block Rockin' Beats", "Chemical Brothers", "01", "Live");
    add("/home/frerich/01 Chemical Brothers - Block rockin' beats.mp3",
        "Block Rockin' Beats", "Chemical Brothers", "01", QString());
    add("/home/frerich/(Chemical Brothers) Block rockin' beats [Live].mp3",
        "Block Rockin' Beats", "Chemical Brothers", QString(), "Live");
    add("/home/frerich/(Chemical Brothers) Block rockin' beats (Live).mp3",
        "Block Rockin' Beats", "Chemical Brothers", QString(), "Live");
    add("/home/frerich/(Chemical Brothers) Block rockin' beats.mp3",
        "Block Rockin' Beats", "Chemical Brothers", QString(), QString());
    add("/home/frerich/Chemical Brothers - Block rockin' beats [Live].mp3",
        "Block Rockin' Beats", "Chemical Brothers", QString(), "Live");
    add("/home/frerich/Chemical Brothers - Block rockin' beats (Live).mp3",
        "Block Rockin' Beats", "Chemical Brothers", QString(), "Live");
    add("/home/frerich/Chemical Brothers - Block rockin' beats.mp3",
        "Block Rockin' Beats", "Chemical Brothers", QString(), QString());
    add("/home/frerich/mp3/Chemical Brothers/Dig your own hole/[01] Block rockin' beats.mp3",
        "Block Rockin' Beats", "Chemical Brothers", "01", QString(), "Dig Your Own Hole");
    add(QDir::homePath() + "/[01] Randy - Religion, religion.mp3",
        "Religion, Religion", "Randy", "01", QString(), QString());
    add(QDir::homePath() + "/(3) Mr. Doe - Punk.mp3",
        "Punk", "Mr. Doe", "3", QString(), QString());
    add("c:\\music\\mp3s\\(3) Mr. Doe - Punk.mp3",
        "Punk", "Mr. Doe", "3", QString(), QString());
}

void TagGuesserTest::testGuesser()
{
    QFETCH(QString, filename);
    QFETCH(QString, title);
    QFETCH(QString, artist);
    QFETCH(QString, track);
    QFETCH(QString, comment);
    QFETCH(QString, album);

    TagGuesser guesser(filename);

    QCOMPARE(guesser.title(), title);
    QCOMPARE(guesser.artist(), artist);
    QCOMPARE(guesser.track(), track);
    QCOMPARE(guesser.comment(), comment);
    QCOMPARE(guesser.album(), album);
}

void TagGuesserTest::add(const QString &filename, const QString &title,
                         const QString &artist, const QString &track,
                         const QString &comment, const QString &album)
{
    QTest::newRow(filename.toUtf8())
        << filename
        << title
        << artist
        << track
        << comment
        << album
    ;
}

QTEST_GUILESS_MAIN(TagGuesserTest)

// vim: set et sw=4 tw=0 sta:

#include "tagguessertest.moc"
