/******************************************************************************
 * QSkinny - Copyright (C) 2016 Uwe Rathmann
 * This file may be used under the terms of the QSkinny License, Version 1.0
 *****************************************************************************/

#include "QskScaleRenderer.h"
#include "QskSkinlet.h"
#include "QskSGNode.h"
#include "QskTickmarksNode.h"
#include "QskTextNode.h"
#include "QskGraphicNode.h"
#include "QskTextOptions.h"
#include "QskGraphic.h"
#include "QskControl.h"
#include "QskFunctions.h"

#include <qstring.h>
#include <qfontmetrics.h>

static QSGNode* qskRemoveTraillingNodes( QSGNode* node, QSGNode* childNode )
{
    QskSGNode::removeAllChildNodesFrom( node, childNode );
    return nullptr;
}

static inline void qskInsertRemoveChild( QSGNode* parentNode,
    QSGNode* oldNode, QSGNode* newNode, bool append )
{
    if ( newNode == oldNode )
        return;

    if ( oldNode )
    {
        parentNode->removeChildNode( oldNode );
        if ( oldNode->flags() & QSGNode::OwnedByParent )
            delete oldNode;
    }

    if ( newNode )
    {
        if ( append )
            parentNode->appendChildNode( newNode );
        else
            parentNode->prependChildNode( newNode );
    }
}

void QskScaleRenderer::setOrientation( Qt::Orientation orientation )
{
    m_orientation = orientation;
}

void QskScaleRenderer::setBoundaries( const QskIntervalF& boundaries )
{
    m_boundaries = boundaries;
}

void QskScaleRenderer::setTickmarks( const QskScaleTickmarks& tickmarks )
{
    m_tickmarks = tickmarks;
}

void QskScaleRenderer::setTickColor( const QColor& color )
{
    m_tickColor = color;
}

void QskScaleRenderer::setTickWidth( qreal width )
{
    m_tickWidth = width;
}

void QskScaleRenderer::setFont( const QFont& font )
{
    m_font = font;
}

void QskScaleRenderer::setTextColors( const QskTextColors& textColors )
{
    m_textColors = textColors;
}

void QskScaleRenderer::setColorFilter( const QskColorFilter& colorFilter )
{
    m_colorFilter = colorFilter;
}

QSGNode* QskScaleRenderer::updateScaleNode(
    const QskSkinnable* skinnable, const QRectF& tickmarksRect,
    const QRectF& labelsRect, QSGNode* node )
{
    enum Role
    {
        Ticks = 1,
        Labels = 2
    };

    if ( node == nullptr )
        node = new QSGNode();

    {
        QSGNode* oldNode = QskSGNode::findChildNode( node, Ticks );
        QSGNode* newNode = nullptr;

        if ( !tickmarksRect.isEmpty() )
        {
            newNode = updateTicksNode( skinnable, tickmarksRect, oldNode );
            if ( newNode )
                QskSGNode::setNodeRole( newNode, Ticks );
        }

        qskInsertRemoveChild( node, oldNode, newNode, false );
    }

    {
        QSGNode* oldNode = QskSGNode::findChildNode( node, Labels );
        QSGNode* newNode = nullptr;

        if ( !labelsRect.isEmpty() )
        {
            newNode = updateLabelsNode( skinnable, tickmarksRect, labelsRect, oldNode );
            if ( newNode )
                QskSGNode::setNodeRole( newNode, Labels );
        }

        qskInsertRemoveChild( node, oldNode, newNode, true );
    }

    return node;
}

QSGNode* QskScaleRenderer::updateTicksNode(
    const QskSkinnable*, const QRectF& rect, QSGNode* node ) const
{
    if ( rect.isEmpty() )
        return nullptr;

    auto ticksNode = static_cast< QskTickmarksNode* >( node );

    if( ticksNode == nullptr )
        ticksNode = new QskTickmarksNode;

    ticksNode->update( m_tickColor, rect, m_boundaries,
        m_tickmarks, m_tickWidth, m_orientation );

    return ticksNode;
}

QSGNode* QskScaleRenderer::updateLabelsNode(
    const QskSkinnable* skinnable, const QRectF& tickmarksRect,
    const QRectF& labelsRect, QSGNode* node ) const
{
    if ( labelsRect.isEmpty() || tickmarksRect.isEmpty() )
        return nullptr;

    const auto ticks = m_tickmarks.majorTicks();
    if ( ticks.isEmpty() )
        return nullptr;

    if( node == nullptr )
        node = new QSGNode;

    const QFontMetricsF fm( m_font );

    const qreal length = ( m_orientation == Qt::Horizontal )
        ? tickmarksRect.width() : tickmarksRect.height();
    const qreal ratio = length / m_boundaries.width();

    auto nextNode = node->firstChild();

    for ( auto tick : ticks )
    {
        enum LabelNodeRole
        {
            TextNode = 1,
            GraphicNode = 2
        };

        const auto label = labelAt( tick );
        if ( label.isNull() )
            continue;

        const qreal tickPos = ratio * ( tick - m_boundaries.lowerBound() );

        if ( label.canConvert< QString >() )
        {
            const auto text = label.toString();
            if ( text.isEmpty() )
                continue;

            QRectF r;
            Qt::Alignment alignment;

            if( m_orientation == Qt::Horizontal )
            {
                const auto w = qskHorizontalAdvance( fm, text );

                auto pos = tickmarksRect.x() + tickPos - 0.5 * w;
                pos = qBound( labelsRect.left(), pos, labelsRect.right() - w );

                r = QRectF( pos, labelsRect.y(), w, labelsRect.height() );

                alignment = Qt::AlignLeft;
            }
            else
            {
                const auto h = fm.height();

                auto pos = tickmarksRect.bottom() - ( tickPos + 0.5 * h );

                /*
                    when clipping the label we can expand the clip rectangle
                    by the ascent/descent margins, as nothing gets painted there
                    anyway.
                 */
                const qreal min = labelsRect.top() - ( h - fm.ascent() );
                const qreal max = labelsRect.bottom() + fm.descent();
                pos = qBound( min, pos, max );

                r = QRectF( labelsRect.x(), pos, labelsRect.width(), h );

                alignment = Qt::AlignRight;
            }

            if ( nextNode && QskSGNode::nodeRole( nextNode ) != TextNode )
            {
                nextNode = qskRemoveTraillingNodes( node, nextNode );
            }

            if ( nextNode == nullptr )
            {
                nextNode = new QskTextNode;
                QskSGNode::setNodeRole( nextNode, TextNode );
                node->appendChildNode( nextNode );
            }

            auto textNode = static_cast< QskTextNode* >( nextNode );
            textNode->setTextData( skinnable->owningControl(), text, r, m_font,
                QskTextOptions(), m_textColors, alignment, Qsk::Normal );

            nextNode = nextNode->nextSibling();
        }
        else if ( label.canConvert< QskGraphic >() )
        {
            const auto graphic = label.value< QskGraphic >();
            if ( graphic.isNull() )
                continue;

            QRectF r;

            const auto h = fm.height();
            const auto w = graphic.widthForHeight( h );

            Qt::Alignment alignment;

            if( m_orientation == Qt::Horizontal )
            {
                auto pos = tickmarksRect.x() + tickPos - 0.5 * w;
                pos = qBound( labelsRect.left(), pos, labelsRect.right() - w );

                r = QRectF( pos, labelsRect.y(), w, h );
                alignment = Qt::AlignHCenter | Qt::AlignBottom;
            }
            else
            {
                auto pos = tickmarksRect.bottom() - ( tickPos + 0.5 * h );
                pos = qBound( labelsRect.top(), pos, labelsRect.bottom() - h );

                r = QRectF( labelsRect.right() - w, pos, w, h );
                alignment = Qt::AlignRight | Qt::AlignVCenter;
            }

            if ( nextNode && QskSGNode::nodeRole( nextNode ) != GraphicNode )
            {
                nextNode = qskRemoveTraillingNodes( node, nextNode );
            }

            if ( nextNode == nullptr )
            {
                nextNode = new QskGraphicNode;
                QskSGNode::setNodeRole( nextNode, GraphicNode );
                node->appendChildNode( nextNode );
            }

            auto graphicNode = static_cast< QskGraphicNode* >( nextNode );

            QskSkinlet::updateGraphicNode(
                skinnable->owningControl(), graphicNode,
                graphic, m_colorFilter, r, alignment );

            nextNode = nextNode->nextSibling();
        }
    }

    qskRemoveTraillingNodes( node, nextNode );

    return node;
}

QVariant QskScaleRenderer::labelAt( qreal pos ) const
{
    return QString::number( pos, 'g' );
}

QSizeF QskScaleRenderer::boundingLabelSize() const
{
    const auto ticks = m_tickmarks.majorTicks();
    if ( ticks.isEmpty() )
        return QSizeF( 0.0, 0.0 );

    const QFontMetricsF fm( m_font );

    qreal maxWidth = 0.0;
    const qreal h = fm.height();

    for ( auto tick : ticks )
    {
        qreal w = 0.0;

        const auto label = labelAt( tick );
        if ( label.isNull() )
            continue;

        if ( label.canConvert< QString >() )
        {
            w = qskHorizontalAdvance( fm, label.toString() );
        }
        else if ( label.canConvert< QskGraphic >() )
        {
            const auto graphic = label.value< QskGraphic >();
            if ( !graphic.isNull() )
            {
                w = graphic.widthForHeight( h );
            }
        }

        maxWidth = qMax( w, maxWidth );
    }

    return QSizeF( maxWidth, h );
}
