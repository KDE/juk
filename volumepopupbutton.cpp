/**
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>
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

#include "volumepopupbutton.h"
#include "slider.h"

#include <KLocalizedString>
#include <QIcon>

#include <QAction>
#include <QLabel>
#include <QMenu>
#include <QToolBar>
#include <QWheelEvent>
#include <QWidgetAction>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "playermanager.h"
#include "juk.h"

VolumePopupButton::VolumePopupButton( QWidget * parent )
    : QToolButton( parent )
{
    m_volumeBeforeMute = 0.0;

    //create the volume popup
    m_volumeMenu = new QMenu( this );

    auto mainWidget = new QWidget( this );
    mainWidget->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Preferred );

    auto mainBox = new QVBoxLayout( mainWidget );
    mainBox->setContentsMargins( 0 ,  0 ,  0 ,  0 );
    mainBox->setSpacing( 0 );

    m_volumeLabel = new QLabel;
    m_volumeLabel->setAlignment( Qt::AlignHCenter );
    mainBox->addWidget( m_volumeLabel );

    auto sliderBox = new QWidget;
    sliderBox->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Preferred );

    auto sliderBoxLayout = new QHBoxLayout( sliderBox );
    sliderBoxLayout->setSpacing( 0 );
    sliderBoxLayout->setContentsMargins( 0 ,  0 ,  0 ,  0 );
    mainBox->addWidget(sliderBox);

    m_volumeSlider = new VolumeSlider( 100, sliderBox, false );
    m_volumeSlider->setFixedHeight( 200 ); // FIXME HiDPI
    sliderBoxLayout->addWidget(m_volumeSlider);

    PlayerManager *player = JuK::JuKInstance()->playerManager();

    QWidgetAction *sliderActionWidget = new QWidgetAction( this );
    sliderActionWidget->setDefaultWidget( mainWidget );

    connect( m_volumeSlider, SIGNAL(volumeChanged(float)), player, SLOT(setVolume(float)) );

    m_muteAction = new QAction( QIcon::fromTheme(QStringLiteral("audio-volume-muted")), QString(), this );
    m_muteAction->setToolTip( i18n( "Mute/Unmute" ) );
    m_muteAction->setCheckable( true );
    m_muteAction->setChecked( player->muted() );

    connect( m_muteAction, SIGNAL(toggled(bool)), player, SLOT(setMuted(bool)) );
    connect( player, SIGNAL(mutedChanged(bool)), this, SLOT(muteStateChanged(bool)) );

    m_volumeMenu->addAction( sliderActionWidget );
    m_volumeMenu->addAction( m_muteAction );

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
        setIcon( QIcon::fromTheme(QStringLiteral("audio-volume-muted")) );
    else if ( newVolume < 0.34 )
        setIcon( QIcon::fromTheme(QStringLiteral("audio-volume-low")) );
    else if ( newVolume < 0.67 )
        setIcon( QIcon::fromTheme(QStringLiteral("audio-volume-medium")) );
    else
        setIcon( QIcon::fromTheme(QStringLiteral("audio-volume-high")) );

    m_volumeLabel->setText( i18n( "%1%" , int( newVolume * 100 ) ) );

    if( newVolume != m_volumeSlider->value() )
        m_volumeSlider->setValue( newVolume * 100 );

    //make sure to uncheck mute toolbar when moving slider
    if ( newVolume > 0 )
        m_muteAction->setChecked( false );

    const KLocalizedString tip = m_muteAction->isChecked() ? ki18n( "Volume: %1% (muted)" ) : ki18n( "Volume: %1%" );
    setToolTip( tip.subs( int( 100 * newVolume ) ).toString() );
}

void
VolumePopupButton::muteStateChanged( bool muted )
{
    if ( muted )
    {
        const float volume = JuK::JuKInstance()->playerManager()->volume();
        setIcon( QIcon::fromTheme(QStringLiteral("audio-volume-muted")) );
        setToolTip( i18n( "Volume: %1% (muted)", int( 100 * volume ) ) );
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
    float volume = qBound( 0.0f, player->volume() + float( event->angleDelta().y() ) / 4000.0f, 1.0f );
    player->setVolume( volume );
    volumeChanged( volume );
}
