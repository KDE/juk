#include "tagguesser.h"
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kapplication.h>
#include <iostream>

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
            "Block rockin' beats", "Chemical Brothers", "01", "Live" );
    check( "/home/frerich/Chemical Brothers - (01) - Block rockin' beats (Live).mp3",
            "Block rockin' beats", "Chemical Brothers", "01", "Live" );
    check( "/home/frerich/Chemical Brothers - (01) - Block rockin' beats.mp3",
            "Block rockin' beats", "Chemical Brothers", "01", QString::null );
    check( "/home/frerich/Chemical Brothers - [01] - Block rockin' beats [Live].mp3",
            "Block rockin' beats", "Chemical Brothers", "01", "Live" );
    check( "/home/frerich/Chemical Brothers - [01] - Block rockin' beats (Live).mp3",
            "Block rockin' beats", "Chemical Brothers", "01", "Live" );
    check( "/home/frerich/Chemical Brothers - [01] - Block rockin' beats.mp3",
            "Block rockin' beats", "Chemical Brothers", "01", QString::null );
    check( "/home/frerich/Chemical Brothers - 01 - Block rockin' beats [Live].mp3",
            "Block rockin' beats", "Chemical Brothers", "01", "Live" );
    check( "/home/frerich/Chemical Brothers - 01 - Block rockin' beats (Live).mp3",
            "Block rockin' beats", "Chemical Brothers", "01", "Live" );
    check( "/home/frerich/Chemical Brothers - 01 - Block rockin' beats.mp3",
            "Block rockin' beats", "Chemical Brothers", "01", QString::null );
    check( "/home/frerich/(01) Chemical Brothers - Block rockin' beats [Live].mp3",
            "Block rockin' beats", "Chemical Brothers", "01", "Live" );
    check( "/home/frerich/(01) Chemical Brothers - Block rockin' beats (Live).mp3",
            "Block rockin' beats", "Chemical Brothers", "01", "Live" );
    check( "/home/frerich/(01) Chemical Brothers - Block rockin' beats.mp3",
            "Block rockin' beats", "Chemical Brothers", "01", QString::null );
    check( "/home/frerich/[01] Chemical Brothers - Block rockin' beats [Live].mp3",
            "Block rockin' beats", "Chemical Brothers", "01", "Live" );
    check( "/home/frerich/[01] Chemical Brothers - Block rockin' beats (Live).mp3",
            "Block rockin' beats", "Chemical Brothers", "01", "Live" );
    check( "/home/frerich/[01] Chemical Brothers - Block rockin' beats.mp3",
            "Block rockin' beats", "Chemical Brothers", "01", QString::null );
    check( "/home/frerich/01 Chemical Brothers - Block rockin' beats [Live].mp3",
            "Block rockin' beats", "Chemical Brothers", "01", "Live" );
    check( "/home/frerich/01 Chemical Brothers - Block rockin' beats (Live).mp3",
            "Block rockin' beats", "Chemical Brothers", "01", "Live" );
    check( "/home/frerich/01 Chemical Brothers - Block rockin' beats.mp3",
            "Block rockin' beats", "Chemical Brothers", "01", QString::null );
    check( "/home/frerich/(Chemical Brothers) Block rockin' beats [Live].mp3",
            "Block rockin' beats", "Chemical Brothers", QString::null, "Live" );
    check( "/home/frerich/(Chemical Brothers) Block rockin' beats (Live).mp3",
            "Block rockin' beats", "Chemical Brothers", QString::null, "Live" );
    check( "/home/frerich/(Chemical Brothers) Block rockin' beats.mp3",
            "Block rockin' beats", "Chemical Brothers", QString::null, QString::null );
    check( "/home/frerich/Chemical Brothers - Block rockin' beats [Live].mp3",
            "Block rockin' beats", "Chemical Brothers", QString::null, "Live" );
    check( "/home/frerich/Chemical Brothers - Block rockin' beats (Live).mp3",
            "Block rockin' beats", "Chemical Brothers", QString::null, "Live" );
    check( "/home/frerich/Chemical Brothers - Block rockin' beats.mp3",
            "Block rockin' beats", "Chemical Brothers", QString::null, QString::null );
    check( "/home/frerich/mp3/Chemical Brothers/Dig your own hole/[01] Block rockin' beats.mp3",
            "Block rockin' beats", "Chemical Brothers", "01", QString::null, "Dig your own hole");
    cout << "All OK" << endl;
    return 0;
}
