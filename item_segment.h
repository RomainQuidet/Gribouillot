/**
 *  @brief
 *  @details
 *  @author    Ludovic A. 
 *  @date      2015 /2016/2017/2018
 *  @bug       No known bugs
 *  @copyright GNU Public License v3
 */

#ifndef ITEM_SEGMENT_H
#define ITEM_SEGMENT_H

#include <QDomDocument>
#include <QGraphicsLineItem>
#include <QXmlStreamWriter>

#include "gribouillotitem.h"

class Item_segment : public QGraphicsLineItem
{
public:
    enum { Type = SEGMENT };
    int type() const
    {
        // Enable the use of qgraphicsitem_cast with this item.
        return SEGMENT;
    }

    explicit Item_segment(QColor penColor, int penWidth, QVector<QPointF> points);
    explicit Item_segment(QDomElement e);
    ~Item_segment(){}

    void newPen(QColor penColor, int penWidth);
    void serialize2xml(QXmlStreamWriter* w);//override item_line::serialize
    QString status(qreal scale, QString scaleUnit);//override item_line::status

protected:
    QRectF boundingRect() const Q_DECL_OVERRIDE;
    QPainterPath shape() const Q_DECL_OVERRIDE;
    void paint (QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget = 0) Q_DECL_OVERRIDE;

private:
    QPolygonF selectionPolygon;

    void createSelectionPolygon(int penWidth);

};

#endif // ITEM_SEGMENT_H
