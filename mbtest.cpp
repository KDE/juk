//Zack - Pizza license - you agree to eat pizza if you modify this file.
//Compile with :
//g++ -Wall -g -lmusicbrainz -L$QTDIR/lib -L$KDEDIR/lib -I$QTDIR/include -I$KDEDIR/include -lqt-mt -lkdecore musicbrainzquery.cpp mbtest.cpp -o mbtest
//First create mocs of course:
//moc mbtest.cpp -o mbtest.moc
//moc musicbrainzquery.h -o musicbrainzquery.moc
//then "./test some_file.mp3" will identify (or at least try to) the file


#include <kapplication.h>
#include <kdebug.h>
#include <qobject.h>
#include "musicbrainzquery.h"

class TestMB : public QObject {
  Q_OBJECT
public:
  TestMB( const QString& file ) : QObject(0, "hello") {
    QStringList l;
    l<<file;
    MusicBrainzQuery *query = new MusicBrainzQuery( MusicBrainzQuery::File ,
                                                    l );
    connect( query, SIGNAL(done(const MusicBrainzQuery::TrackList&)),
             SLOT(slotTrack(const MusicBrainzQuery::TrackList&)) );
    query->start();
  }
public slots:
  void slotTrack( const MusicBrainzQuery::TrackList& res ) {
    for( MusicBrainzQuery::TrackList::ConstIterator itr = res.begin();
         itr != res.end(); ++itr ) {
      kdDebug() <<"Album     = "<< (*itr).album <<endl;
      kdDebug() <<"Artist    = "<< (*itr).artist << endl;
      kdDebug() <<"Id        = "<< (*itr).id <<endl;
      kdDebug() <<"Name      = "<< (*itr).name <<endl;
      kdDebug() <<"Artist id = "<< (*itr).artistId <<endl;
      kdDebug() <<"Song Num  = "<< (*itr).num <<endl;
    }
    kapp->quit();
  }
};

#include "mbtest.moc"

int
main( int argc, char **argv )
{
  KApplication app( argc, argv, "test" );

  if ( argc != 2 ) {
    kdDebug()<<"Usage = "<<argv[0]<<" some_file.mp3"<<endl;
    exit(1);
  }

  TestMB mb(argv[1]);

  return app.exec();
}




