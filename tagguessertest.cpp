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
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kapplication.h>
#include <qdir.h>
#include <iostream>

#include <stdlib.h>

using std::cout;
using std::endl;

void check( const QString &filename, const QString &title,
            const QString &artist, const QString &track,
            const QString &comment, const QString &album = QString::null )
{
    cout << "Checking " << filename.latin1() << "...";
    TagGuesser guesser( filename );
    if ( guesser.title() != title ) {
        cout << "Error: In filename " << filename.latin1() << ", expected title " << title.latin1() << ", got title " << guesser.title().latin1() << endl;
        exit( 1 );
    }
    if ( guesser.artist() != artist ) {
        cout << "Error: In filename " << filename.latin1() << ", expected artist " << artist.latin1() << ", got artist " << guesser.artist().latin1() << endl;
        exit( 1 );
    }
    if ( guesser.track() != track ) {
        cout << "Error: In filename " << filename.latin1() << ", expected track " << track.latin1() << ", got track " << guesser.track().latin1() << endl;
        exit( 1 );
    }
    if ( guesser.comment() != comment ) {
        cout << "Error: In filename " << filename.latin1() << ", expected comment " << comment.latin1() << ", got comment " << guesser.comment().latin1() << endl;
        exit( 1 );
    }
    if ( guesser.album() != album ) {
        cout << "Error: In filename " << filename.latin1() << ", expected album " << album.latin1() << ", got album " << guesser.album().latin1() << endl;
        exit( 1 );
    }
    cout << "OK" << endl;
}

int main( int argc, char **argv )
{
    KAboutData aboutData("tagguessertest", "tagguessertest", "0.1");
    KCmdLineArgs::init(argc, argv, &aboutData);
    KApplication app;
    check( "/home/frerich/Chemical Brothers - (01) - Block rockin' beats [Live].mp3",
            "Block Rockin' Beats", "Chemical Brothers", "01", "Live" );
    check( "/home/frerich/Chemical Brothers - (01) - Block rockin' beats (Live).mp3",
            "Block Rockin' Beats", "Chemical Brothers", "01", "Live" );
    check( "/home/frerich/Chemical Brothers - (01) - Block rockin' beats.mp3",
            "Block Rockin' Beats", "Chemical Brothers", "01", QString::null );
    check( "/home/frerich/Chemical Brothers - [01] - Block rockin' beats [Live].mp3",
            "Block Rockin' Beats", "Chemical Brothers", "01", "Live" );
    check( "/home/frerich/Chemical Brothers - [01] - Block rockin' beats (Live).mp3",
            "Block Rockin' Beats", "Chemical Brothers", "01", "Live" );
    check( "/home/frerich/Chemical Brothers - [01] - Block rockin' beats.mp3",
            "Block Rockin' Beats", "Chemical Brothers", "01", QString::null );
    check( "/home/frerich/Chemical Brothers - 01 - Block rockin' beats [Live].mp3",
            "Block Rockin' Beats", "Chemical Brothers", "01", "Live" );
    check( "/home/frerich/Chemical Brothers - 01 - Block rockin' beats (Live).mp3",
            "Block Rockin' Beats", "Chemical Brothers", "01", "Live" );
    check( "/home/frerich/Chemical Brothers - 01 - Block rockin' beats.mp3",
            "Block Rockin' Beats", "Chemical Brothers", "01", QString::null );
    check( "/home/frerich/(01) Chemical Brothers - Block rockin' beats [Live].mp3",
            "Block Rockin' Beats", "Chemical Brothers", "01", "Live" );
    check( "/home/frerich/(01) Chemical Brothers - Block rockin' beats (Live).mp3",
            "Block Rockin' Beats", "Chemical Brothers", "01", "Live" );
    check( "/home/frerich/(01) Chemical Brothers - Block rockin' beats.mp3",
            "Block Rockin' Beats", "Chemical Brothers", "01", QString::null );
    check( "/home/frerich/[01] Chemical Brothers - Block rockin' beats [Live].mp3",
            "Block Rockin' Beats", "Chemical Brothers", "01", "Live" );
    check( "/home/frerich/[01] Chemical Brothers - Block rockin' beats (Live).mp3",
            "Block Rockin' Beats", "Chemical Brothers", "01", "Live" );
    check( "/home/frerich/[01] Chemical Brothers - Block rockin' beats.mp3",
            "Block Rockin' Beats", "Chemical Brothers", "01", QString::null );
    check( "/home/frerich/01 Chemical Brothers - Block rockin' beats [Live].mp3",
            "Block Rockin' Beats", "Chemical Brothers", "01", "Live" );
    check( "/home/frerich/01 Chemical Brothers - Block rockin' beats (Live).mp3",
            "Block Rockin' Beats", "Chemical Brothers", "01", "Live" );
    check( "/home/frerich/01 Chemical Brothers - Block rockin' beats.mp3",
            "Block Rockin' Beats", "Chemical Brothers", "01", QString::null );
    check( "/home/frerich/(Chemical Brothers) Block rockin' beats [Live].mp3",
            "Block Rockin' Beats", "Chemical Brothers", QString::null, "Live" );
    check( "/home/frerich/(Chemical Brothers) Block rockin' beats (Live).mp3",
            "Block Rockin' Beats", "Chemical Brothers", QString::null, "Live" );
    check( "/home/frerich/(Chemical Brothers) Block rockin' beats.mp3",
            "Block Rockin' Beats", "Chemical Brothers", QString::null, QString::null );
    check( "/home/frerich/Chemical Brothers - Block rockin' beats [Live].mp3",
            "Block Rockin' Beats", "Chemical Brothers", QString::null, "Live" );
    check( "/home/frerich/Chemical Brothers - Block rockin' beats (Live).mp3",
            "Block Rockin' Beats", "Chemical Brothers", QString::null, "Live" );
    check( "/home/frerich/Chemical Brothers - Block rockin' beats.mp3",
            "Block Rockin' Beats", "Chemical Brothers", QString::null, QString::null );
    check( "/home/frerich/mp3/Chemical Brothers/Dig your own hole/[01] Block rockin' beats.mp3",
            "Block Rockin' Beats", "Chemical Brothers", "01", QString::null, "Dig Your Own Hole");
    check( QDir::homeDirPath() + "/[01] Randy - Religion, religion.mp3",
            "Religion, Religion", "Randy", "01", QString::null, QString::null );
    check( QDir::homeDirPath() + "/(3) Mr. Doe - Punk.mp3",
            "Punk", "Mr. Doe", "3", QString::null, QString::null );
    check( "c:\\music\\mp3s\\(3) Mr. Doe - Punk.mp3",
            "Punk", "Mr. Doe", "3", QString::null, QString::null );
    cout << "All OK" << endl;
    return 0;
}
