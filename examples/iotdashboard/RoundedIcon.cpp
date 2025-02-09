/******************************************************************************
 * Copyright (C) 2021 Edelhirsch Software GmbH
 * This file may be used under the terms of the 3-clause BSD License
 *****************************************************************************/

#include "RoundedIcon.h"

QSK_SUBCONTROL( RoundedIcon, Panel )
QSK_SUBCONTROL( RoundedIcon, PalePanel )

QSK_STATE( RoundedIcon, Bright, ( QskAspect::FirstUserState << 1 ) )

RoundedIcon::RoundedIcon( bool isBright, QQuickItem* parent )
    : QskGraphicLabel( parent )
{
    setAlignment( Qt::AlignCenter );
    setFillMode( QskGraphicLabel::Pad );

    if( isBright )
        setSkinStateFlag( Bright );

    setPanel( true );
    setPale( false );
}

void RoundedIcon::setPale( bool on )
{
    setSubcontrolProxy( QskGraphicLabel::Panel, on ? PalePanel : Panel );
}

#include "moc_RoundedIcon.cpp"
