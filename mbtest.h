//Zack - Pizza license - you agree to eat pizza if you modify this file.
//Compile with :
//g++ -Wall -g -lmusicbrainz -L$QTDIR/lib -L$KDEDIR/lib -I$QTDIR/include -I$KDEDIR/include -lqt-mt -lkdecore musicbrainzquery.cpp mbtest.cpp -o mbtest
//First create mocs of course:
//moc mbtest.cpp -o mbtest.moc
//moc musicbrainzquery.h -o musicbrainzquery.moc
//then "./test some_file.mp3" will identify (or at least try to) the file
#ifndef JUK_MBTEST_H
#define JUK_MBTEST_H

#include "musicbrainzquery.h"

class TestMB : public QObject {
  Q_OBJECT
public:
  TestMB( const QString& file );

public slots:
  void slotTrack( const MusicBrainzQuery::TrackList& res );
};

#endif // JUK_MBTEST_H
