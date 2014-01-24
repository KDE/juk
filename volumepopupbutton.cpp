/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "volumepopupbutton.h"
#include "slider.h"

#include <KLocale>
#include <KVBox>
#include <KIcon>

#include <QAction>
#include <QLabel>
#include <QMenu>
#include <QToolBar>
#include <QWheelEvent>
#include <QWidgetAction>

#include "playermanager.h"
#include "juk.h"

VolumePopupButton::VolumePopupButton( QWidget * parent )
    : QToolButton( parent )
{
    m_volumeBeforeMute = 0.0;

    //create the volume popup
    m_volumeMenu = new QMenu( this );

    KVBox *mainBox = new KVBox( this );

    m_volumeLabel= new QLabel( mainBox );
    m_volumeLabel->setAlignment( Qt::AlignHCenter );

    KHBox *sliderBox = new KHBox( mainBox );
    m_volumeSlider = new VolumeSlider( 100, sliderBox, false );
    m_volumeSlider->setFixedHeight( 170 );
    mainBox->setMargin( 0 );
    mainBox->setSpacing( 0 );
    sliderBox->setSpacing( 0 );
    sliderBox->setMargin( 0 );
    mainBox->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );
    sliderBox->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );

    PlayerManager *player = JuK::JuKInstance()->playerManager();

    QWidgetAction *sliderActionWidget = new QWidgetAction( this );
    sliderActionWidget->setDefaultWidget( mainBox );

    connect( m_volumeSlider, SIGNAL(volumeChanged(float)), player, SLOT(setVolume(float)) );

    QToolBar *muteBar = new QToolBar( QString(), mainBox );
    muteBar->setContentsMargins( 0, 0, 0, 0 );
    muteBar->setIconSize( QSize( 16, 16 ) );

    m_muteAction = new QAction( KIcon( "audio-volume-muted" ), QString(), muteBar );
    m_muteAction->setToolTip( i18n( "Mute/Unmute" ) );
    m_muteAction->setCheckable( true );
    m_muteAction->setChecked( player->muted() );

    connect( m_muteAction, SIGNAL(toggled(bool)), player, SLOT(setMuted(bool)) );
    connect( player, SIGNAL(mutedChanged(bool)), this, SLOT(muteStateChanged(bool)) );

    m_volumeMenu->addAction( sliderActionWidget );
    muteBar->addAction( m_muteAction );

    // set correct icon and label initially
    volumeChanged( player->volume() );

    connect( player, SIGNAL(volumeChanged(float)), this, SLOT(volumeChanged(float)) );
}

void
VolumePopupButton::refresh()
{
    volumeChanged( JuK::JuKInstance()->playerManager()->volume() );
}

void
VolumePopupButton::volumeChanged( float newVolume )
{
    if (!JuK::JuKInstance()->playerManager()->muted())
    {
        m_volumeBeforeMute = newVolume;
    }

    if ( newVolume <= 0.0001 )
        setIcon( KIcon( "audio-volume-muted" ) );
    else if ( newVolume < 0.34 )
        setIcon( KIcon( "audio-volume-low" ) );
    else if ( newVolume < 0.67 )
        setIcon( KIcon( "audio-volume-medium" ) );
    else
        setIcon( KIcon( "audio-volume-high" ) );

    m_volumeLabel->setText( QString::number( int( newVolume * 100 ) ) + '%' );

    if( newVolume != m_volumeSlider->value() )
        m_volumeSlider->setValue( newVolume * 100 );

    //make sure to uncheck mute toolbar when moving slider
    if ( newVolume > 0 )
        m_muteAction->setChecked( false );

    setToolTip( i18n( "Volume: %1% %2", int( 100 * newVolume ),
                      ( m_muteAction->isChecked() ? i18n( "(muted)" ) : "" ) ) );
}

void
VolumePopupButton::muteStateChanged( bool muted )
{
    if ( muted )
    {
        const float volume = JuK::JuKInstance()->playerManager()->volume();
        setIcon( KIcon( "audio-volume-muted" ) );
        setToolTip( i18n( "Volume: %1% %2", int( 100 * volume ), i18n( "(muted)" ) ) );
    }
    else
    {
        JuK::JuKInstance()->playerManager()->setVolume( m_volumeBeforeMute );
    }

    m_muteAction->setChecked( muted );
}

void
VolumePopupButton::mouseReleaseEvent( QMouseEvent * event )
{
    if( event->button() == Qt::LeftButton )
    {
        if ( m_volumeMenu->isVisible() )
            m_volumeMenu->hide();
        else
        {
            const QPoint pos( 0, height() );
            m_volumeMenu->exec( mapToGlobal( pos ) );
        }
    }
    else if( event->button() == Qt::MidButton )
    {
        muteStateChanged( JuK::JuKInstance()->playerManager()->mute() );
    }

    QToolButton::mouseReleaseEvent( event );
}

void
VolumePopupButton::wheelEvent( QWheelEvent * event )
{
    event->accept();
    PlayerManager *player = JuK::JuKInstance()->playerManager();
    float volume = qBound( 0.0, player->volume() + float( event->delta() ) / 4000.0, 1.0 );
    player->setVolume( volume );
    volumeChanged( volume );
}

#include "volumepopupbutton.moc"

