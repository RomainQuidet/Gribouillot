/**
 *  @brief     Implements a concept of drawing layer à la Gimp in Gribouillot
 *  @details   A GribouillotLayer is composed of:
 *             -> an interface as a tab page added to the MainWindow
 *             -> a list of GraphicsItems added to the main scene
 *  @author    Ludovic A. 
 *  @date      2015/2016/2017/2018
 *  @bug       No known bugs
 *  @copyright GNU Public License v3
 */

#include <QtWidgets>
#include <QDomDocument>
#include <QGraphicsItem>
#include <QXmlStreamWriter>
#include <QFile>
#include <QtDebug>

#include "dlg_centerofmass.h"
#include "dlg_changelayername.h"
#include "dlg_circleradius.h"
#include "dlg_pointweight.h"
#include "gribouillotlayer.h"
#include "main.h"
#include "gribouillot.h"
#include "ui_gribouillotlayer.h"

//A class static variable to provide each layer a unique name
int GribouillotLayer::layer_index = 0;


/**
 * @brief   create a new layer with default label based on "index"
 */
GribouillotLayer::GribouillotLayer(QMainWindow *parent) :
    QWidget(parent)
{
    initLayer();
    label = "layer_"+QString::number(layer_index);

}

/**
 * @brief   create a new layer from an XML file
 */
GribouillotLayer::GribouillotLayer(QString path, QMainWindow *parent) :
    QWidget(parent)
{
    QFileInfo layerFileInfo = QFileInfo(path);

    initLayer();
    label = layerFileInfo.baseName();

    if ( layerFileInfo.isFile() )
        loadXML(path);

}


GribouillotLayer::~GribouillotLayer()
{
    delete ui;
    //delete the layer items
    while (!itemsList.isEmpty())
        delete itemsList.takeFirst();
}


/**
 * @brief   factorize code for new layer
 */
void GribouillotLayer::initLayer()
{
    ui = new Ui::GribouillotLayer;
    ui->setupUi(this);
    ++layer_index;

    areItemsVisible = true;
    visibilityBtt = new QPushButton(this);
    visibilityBtt->setFlat(true);
    visibilityBtt->setAttribute(Qt::WA_TranslucentBackground);
    visibilityBtt->setIcon(QIcon(QPixmap(":/Resources/Icons/visibility-on.png")));

    //Connect the new layer to some mainWindow slots
    connect (this, &GribouillotLayer::newLayerName, static_cast<Gribouillot*>(parent()), &Gribouillot::doChangeLayerName);
    connect (this, &GribouillotLayer::deleteLayer, static_cast<Gribouillot*>(parent()), &Gribouillot::doDeleteLayer);
    connect (this, &GribouillotLayer::addItemToScene, static_cast<Gribouillot*>(parent()), &Gribouillot::doAddItemToScene);
    connect (visibilityBtt, &QPushButton::clicked, this, &GribouillotLayer::toggleVisibility);

}


QString GribouillotLayer::getLabel()
{
    return label;
}

QPushButton* GribouillotLayer::getVisibilityBtt()
{
    return visibilityBtt;
}


/**
 * @brief   Pop-up a dialog to set a new label for the layer.
 * @details KeepOldFile: change label but dont delete the corresponding file on disk.
 */
QString GribouillotLayer::newLabel(bool keepOldFile)
{
    Dlg_changeLayerName dialog(label, this);
    if (dialog.exec() == QDialog::Accepted)
    {
        QString newLabel = dialog.getLayerName();
        if ( !newLabel.isEmpty() )
        {
            if (!keepOldFile)
            {
                QFile file(label+".xml");
                if ( file.exists() )
                    file.rename(label+".xml", newLabel+".xml");
            }
            /* else
             * We do nothing and leave old file on disk.
             * Necessary when saving 2 tabs with the same label.
             */

            label = newLabel;
        }
    }

    return label;
}




/**
 * @brief   Add a graphic item to the layer.
 */
void GribouillotLayer::addItemToLayer(QGraphicsItem *item)
{
    item->setFlags( item->flags()
                    | QGraphicsItem::ItemIsSelectable);

    item->setOpacity((qreal)ui->opacitySlider->value()/100);
    item->setVisible(areItemsVisible);

    //qDebug() << "adding item to the itemslist + signal" << item;

    itemsList.append(item);
    //Tell the scene
    emit addItemToScene(item);

}




/**
 * @brief   return true if 'item' belongs to current layer.
 */
bool GribouillotLayer::contains(QGraphicsItem *item)
{
    return itemsList.contains(item);
}


/**
 * @brief   return the selected items of the layer
 * @return
 */
QList<QGraphicsItem *> GribouillotLayer::selectedItems()
{
    QList<QGraphicsItem*> selected;
    foreach(QGraphicsItem* item, itemsList)
    {
        if (item->isSelected())
                selected << item;

    }

    return selected;
}


/**
 * @brief   Load the layer from XML file
 * @details NB: we already are in the project directory.
 */
void GribouillotLayer::loadXML(QString path)
{
    QFile file(path);
    QDomDocument doc;

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::warning(this,
                             tr("Accessing layer file"),
                             tr("Could not open the layer \"")+label+"\".");
        return;
    }

    doc.setContent(&file);
    /* GribouillotLayer element */
    QDomElement gribLayer = doc.documentElement();
        //Visibility
        QString visibility = gribLayer.attribute("Visibility");
        if (visibility == "false")
            toggleVisibility();//Visibility is ON by default
        //Opacity
        QString opacity = gribLayer.attribute("Opacity");
        ui->opacitySlider->setValue(opacity.toInt());

    /* Constructs WEIGHTED POINTS graphics items */
    QDomNodeList points = doc.elementsByTagName("Point");
    for (int i=0; i < points.size();  ++i)
        addItemToLayer( new Item_point(points.item(i).toElement()) );

    /* Constructs LINES graphics items */
    QDomNodeList lines = doc.elementsByTagName("Line");
    for (int i=0; i < lines.size();  ++i)
        addItemToLayer( new Item_line(lines.item(i).toElement()) );

    /* Constructs SEGMENTS graphics items */
    QDomNodeList segments = doc.elementsByTagName("Segment");
    for (int i=0; i < segments.size();  ++i)
        addItemToLayer( new Item_segment(segments.item(i).toElement()) );

    /* Constructs CIRCLES graphics items */
    QDomNodeList circles = doc.elementsByTagName("Circle");
    for (int i=0; i < circles.size();  ++i)
        addItemToLayer( new Item_circle(circles.item(i).toElement()) );

    /* Constructs ARCS graphics items */
    QDomNodeList arcs = doc.elementsByTagName("Arc");
    for (int i=0; i < arcs.size();  ++i)
        addItemToLayer( new Item_arc(arcs.item(i).toElement()) );

    /* Constructs SPIRALS graphics items */
    QDomNodeList spirals = doc.elementsByTagName("Spiral");
    for (int i=0; i < spirals.size();  ++i)
        addItemToLayer( new Item_spiral(spirals.item(i).toElement()) );

    /* Load PIXMAP */
    QDomNodeList pixmaps = doc.elementsByTagName("Pixmap");
    for (int i=0; i < pixmaps.size();  ++i)
        addItemToLayer( new Item_pixmap(pixmaps.item(i).toElement()) );

    return;
}


/******************* Public slots ********************/
/**
 * @brief   Save the layer as an XML file
 * @details NB: we already are in the project directory.
 */
bool GribouillotLayer::writeXML()
{
    QString filename = label+".xml";

    QFile file(filename);
    file.remove();//clean previous xml

    if (!file.open(QIODevice::ReadWrite | QIODevice::Text))
    {
        QMessageBox::warning(this,
                             "Could not open xml file",
                             "The layer \""+label+"\" can not be saved");
        return false;//failure
    }

    QXmlStreamWriter xW(&file);
    xW.setAutoFormatting(true);
    xW.writeStartDocument();

    xW.writeStartElement("GribouillotLayer");
        areItemsVisible ?
            xW.writeAttribute("Visibility", "true"):
            xW.writeAttribute("Visibility", "false");
        xW.writeAttribute("Opacity", QString::number(ui->opacitySlider->value()));

    xW.writeStartElement("GraphicsItemList");
    //qDebug()<< itemsList;
    /*
     * Because of the Diamond problem, we can not subclass the items and call
     * directly something like item->serialize2xml(). We must recast each item
     * to its subtype first.
     */
    foreach(QGraphicsItem* item, itemsList)
    {
        //Item_point
        if( item->type() == POINT_W )
        {
            Item_point* point = qgraphicsitem_cast<Item_point *>(item);
            point->serialize2xml(&xW);
        }

        //Item_line
        if( item->type() == LINE )
        {
            Item_line* line = qgraphicsitem_cast<Item_line*>(item);
            line->serialize2xml(&xW);
        }

        //Item_segment
        if( item->type() == SEGMENT )
        {
            Item_segment* segment = qgraphicsitem_cast<Item_segment*>(item);
            segment->serialize2xml(&xW);
        }

        //Item_circle
        if( item->type() == CIRCLE )
        {
            Item_circle* circle = qgraphicsitem_cast<Item_circle*>(item);
            circle->serialize2xml(&xW);
        }

        //Item_arc
        if( item->type() == ARC )
        {
            Item_arc* arc = qgraphicsitem_cast<Item_arc*>(item);
            arc->serialize2xml(&xW);
        }

        //Item_spiral
        if( item->type() == SPIRAL )
        {
            Item_spiral* spiral = qgraphicsitem_cast<Item_spiral*>(item);
            spiral->serialize2xml(&xW);
        }

        //Item_pixmap
        if( item->type() == PIXMAP )
        {
            Item_pixmap* pixmap = qgraphicsitem_cast<Item_pixmap*>(item);
            pixmap->serialize2xml(&xW);
        }
    }

    xW.writeEndElement();   //end of GraphicsItemList
    xW.writeEndElement();   //end of Layer Document

    file.close();

    return true;//success
}


/**
 * @brief   Del key pressed: delete items selected by user.
 * \return  true if at least one item was deleted
 */
int GribouillotLayer::deleteSelectedItems()
{
    int deletedItems = 0;//keep count to inform user.

    //delete the layer items selected in the view
    foreach(QGraphicsItem* item, itemsList)
    {
        if (item->isSelected())
        {
            item->scene()->removeItem(item);
            itemsList.removeOne(item);
            delete item;
            deletedItems++;
        }
    }

    return deletedItems;

}


/**
 * @brief   Space key pressed: select next item in list.
 * \return  true if at least one item was deleted
 */
void GribouillotLayer::selectNextItem()
{
    if ( !itemsList.isEmpty() )
    {
        //Select first item if none is yet selected (= 0)
        int selectedItemIndex = 0;

        for (int i = 0; i < itemsList.size(); ++i)
        {
            if (itemsList.at(i)->isSelected())
            {
                itemsList.at(i)->setSelected(false);
                selectedItemIndex = i;
            }
        }
        //Select next item, cycling through itemsList if necessary (%)
        itemsList.at((selectedItemIndex+1) % itemsList.size())
                    ->setSelected(true);
    }
}



/******************* Private slots ********************/
/**
 * @brief   Change the layer name
 */
void GribouillotLayer::on_layerNameTlBtt_clicked()
{
    emit newLayerName( newLabel(false) );
}


/**
 * @brief   Toggle visibility of the items of the layer
 */
void GribouillotLayer::toggleVisibility()
{
    areItemsVisible = !areItemsVisible;

    if (areItemsVisible)
        visibilityBtt->setIcon(QIcon(QPixmap(":/Resources/Icons/visibility-on.png")));
    else
        visibilityBtt->setIcon(QIcon(QPixmap(":/Resources/Icons/visibility-off.png")));

    for (int i = 0; i < itemsList.size(); ++i)
        itemsList.at(i)->setVisible(areItemsVisible);

}


/**
 * @brief   Change layer opacity
 */
void GribouillotLayer::on_opacitySlider_valueChanged(int value)
{
    for (int i = 0; i < itemsList.size(); ++i)
        itemsList.at(i)->setOpacity((qreal)value/100);

    ui->opacityPercentLbl->setText(QString::number(value)+" %");
}

/**
 * @brief   Confirm layer destruction
 */
void GribouillotLayer::on_deleteLayerTlBtt_clicked()
{
    int ret = QMessageBox::warning(this,
                                   tr("Confirm removal"),
                                   tr("Do you really want to delete this layer?"
                                      " The file on disk will deleted too."),
                                   QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Ok)
    {
        QString filename = label+".xml";
        QFile file(filename);
        file.remove();

        emit deleteLayer();
    }

}



/*********************** drawing function ******************************/
/**
 * @brief   draw a point from user input.
 */
void GribouillotLayer::drawPoint(QColor penColor, int penWidth, QPointF position, bool askWeight)
{
    int weight = 0;

    if(askWeight)
    {
        Dlg_pointWeight dialog(this);
        if (dialog.exec() == QDialog::Accepted)
            weight = dialog.getWeightValue();
    }

    drawPoint(penColor, penWidth, position, weight);

    if (weight != 0)
    {
        //display weight label of the new point
        (qgraphicsitem_cast<Item_point *>(itemsList.last()))->showWeight(true);
    }

}

/**
 * @brief   draw a point
 * @details called by the other drawPoint() or directly upon loading XML data.
 */
void GribouillotLayer::drawPoint(QColor penColor, int penWidth, QPointF position, int weight)
{
    addItemToLayer(new Item_point(penColor, penWidth, position, weight));
}


/**
 * @brief   toggle the display of points weights
 */
void GribouillotLayer::displayPointWeight(bool isDisplayed)
{
    foreach(QGraphicsItem* item, itemsList)
    {
        //Item_point
        if( item->type() == POINT_W )
        {
            Item_point* point = qgraphicsitem_cast<Item_point*>(item);
            point->showWeight(isDisplayed);
        }
    }
}


/**
 * @brief   Compute the center of mass of the weighted points
 * \return  The center of mass graphicsItem, so that Gribouillot can
 *          center the scene on it.
 */
Item_point* GribouillotLayer::computeCoM(QColor penColor, int penWidth)
{
    Dlg_centerOfMass CoMDialog(this);

    foreach(QGraphicsItem* item, itemsList)
    {
        if( item->type() == POINT_W )
        {
            Item_point* p = qgraphicsitem_cast<Item_point*>(item);
            CoMDialog.addPoint(p->x(), p->y(), p->getWeight());
        }
    }
    CoMDialog.finalizeLayout();


    if (CoMDialog.exec() == QDialog::Accepted)
    {
        //Add the new point to the layer
        Item_point* CoM = new Item_point(penColor, penWidth,
                                         CoMDialog.getCoMCoord());
        addItemToLayer(CoM);

        return CoM;//So that the view can be centered on the Center of M  ass.
    }

    //QDialog::Rejected
    return nullptr;
}



/* LINE DRAWINGS (LINE, SEGMENT, PARALLEL, PERPENDICULAR, BISECTION) */
/**
 * @brief   draw a segment from 2 points given as a vector.
 */
void GribouillotLayer::drawSegment(QColor penColor, int penWidth, QVector<QPointF> points)
{
    if( points[0] != points[1])
        addItemToLayer(new Item_segment(penColor, penWidth, points));

}


/**
 * @brief   draw a segment from a QLineF
 */
void GribouillotLayer::drawSegment(QColor penColor, int penWidth, QLineF segment)
{
    drawSegment(penColor, penWidth, QVector<QPointF>({segment.p1(), segment.p2()}));

}


/**
 * @brief   draw a pseudo-infinite line from the 2 given points.
 * @details A QGraphicsLineItem is only defined as a segment between 2 points. To extend
 *          it nicely, we find the intersections of the extended segment with a huge virtual
 *          rectangle of 100 000 x 100 000 pixels around the drawing area.
 */
void GribouillotLayer::drawLineFromSegment(QColor penColor, int penWidth, QVector<QPointF> positions)
{
    if(positions[0] == positions[1])
        return;

    QLineF segment = QLineF(positions[0], positions[1]);

    QPointF topLeft(-50000, -50000);
    QPointF topRight(50000, -50000);
    QPointF bottomLeft(-50000, 50000);
    QPointF bottomRight(50000, 50000);
    QLineF left(topLeft, bottomLeft);
    QLineF right(topRight, bottomRight);
    QLineF top(topLeft, topRight);
    QLineF bottom(bottomLeft, bottomRight);

    QPointF newEnds[2];
    qreal angle = segment.angleTo( QLineF(0,0,1,1) );

    if ( angle < 90 || (angle > 180 && angle < 270))
    {
        //qDebug() << "intersect top or bottom";
        if ( segment.intersects(top, &newEnds[0]) == QLineF::NoIntersection)
            newEnds[0] = positions[0];
        if ( segment.intersects(bottom, &newEnds[1]) == QLineF::NoIntersection)
            newEnds[1] = positions[1];
    }
    else
    {
        //qDebug() << "intersect right or left";
        if ( segment.intersects(left, &newEnds[0]) == QLineF::NoIntersection)
            newEnds[0] = positions[0];
        if ( segment.intersects(right, &newEnds[1]) == QLineF::NoIntersection)
            newEnds[1] = positions[1];
    }

    addItemToLayer(new Item_line(penColor, penWidth, {newEnds[0], newEnds[1]}));

}


/**
 * @brief   draw a pseudo-infinite line from the given segment.
 * @details Overloaded function written for convenience.
 */
void GribouillotLayer::drawLineFromSegment(QColor penColor, int penWidth, QLineF line)
{
    if(!line.isNull())
        drawLineFromSegment( penColor, penWidth, QVector<QPointF>({line.p1(), line.p2()}) );

}


/**
 * @brief   draw a pseudo-infinite screen-horizontal line.
 */
void GribouillotLayer::drawHorizontal(QColor penColor, int penWidth, QPointF point)
{
    QVector<QPointF> points = {point, point+QPointF(1, 0)};
    drawLineFromSegment(penColor, penWidth, points);
}


/**
 * @brief   draw a pseudo-infinite screen-horizontal line.
 */
void GribouillotLayer::drawVertical(QColor penColor, int penWidth, QPointF point)
{
      QVector<QPointF> points = {point, point+QPointF(0, 1)};
      drawLineFromSegment(penColor, penWidth, points);
}


/**
 * @brief   draw a parallel from currently selected line/segment and a given point
 */
void GribouillotLayer::drawParallel(QColor penColor, int penWidth, QPointF point, QLineF58 selectedLine)
{
    /*
     * To build the unitVector _centered on zero_, we can not use the
     * QLineF::unitVector() function which is not centered on zero!
     * The proper functions to use are dx() and dy()
     */
    QLineF unitVector = QLineF(0, 0, selectedLine.dx(), selectedLine.dy());
    QLineF parallelVector = unitVector.translated(point);

    drawLineFromSegment(penColor, penWidth, parallelVector);
}


/**
 * @brief   draw a perpendicular from currently selected line/segment and a given point
 */
void GribouillotLayer::drawPerpendicular(QColor penColor, int penWidth, QPointF point, Item_segment* selectedSegment)
{
    QLineF selectedLineF = selectedSegment->line();

    if (!selectedLineF.isNull())
    {
        QLineF unitVector = QLineF(0, 0, selectedLineF.dx(), selectedLineF.dy());
        QLineF normalVector = unitVector.normalVector();
        QLineF perpendicularVector = normalVector.translated(point);

        drawLineFromSegment(penColor, penWidth, perpendicularVector);

        /*
         * At this point the new perpendicular line would be automatically selected by Scene
         * if we didn't setEnabled(false). It would give perpendiculars of perpendiculars of
         * perpendiculars etc while we want the user to be able to draw several perpendiculars
         * to the one same line.
         */
        itemsList.last()->setEnabled(false);//Must be done here because of signal race condition
    }

}


/**
 * @brief   draw the bisection of currently selected segment
 */
void GribouillotLayer::drawBisection(QColor penColor, int penWidth, QLineF58 selectedLine)
{
    if( !selectedLine.isNull() )
    {
        QLineF halfSegment(selectedLine.center(), selectedLine.p2());
        QLineF bisectionVector = halfSegment.normalVector();

        drawLineFromSegment(penColor, penWidth, bisectionVector);

    }

}



/* CIRCLES DRAWINGS  (FROM RADIUS, FROM DIAMETER, ETC)*/


/**
 * @brief   draw a circle from saved XML data
 */
void GribouillotLayer::drawCircle(QColor penColor, int penWidth, QPointF center, qreal radius)
{
    addItemToLayer(new Item_circle(penColor, penWidth, center, radius));

}


/**
 * @brief   draw a circle from a selected segment taken as radius and the position of the center.
 * @details the position of the center is defined by a user click on one side of the segment.
 * @return  the position of the center in scene coordinates
 */
QPointF GribouillotLayer::drawCircleFromRadius(QColor penColor, int penWidth,
                                               QPointF position, QLineF58 selectedRadius)
{
    QPointF center = QPointF();
    QPointF p1 = selectedRadius.p1();
    QPointF p2 = selectedRadius.p2();

    if( !selectedRadius.isNull() )
    {
        QLineF clickToP1(p1, position);
        QLineF clickTop2(p2, position);

        center = clickToP1.length() < clickTop2.length() ? p1 : p2;

        drawCircle(penColor, penWidth, center, selectedRadius.length());
    }

    return center;
}


/**
 * @brief   draw a circle from its center and a point on its outline
 * @return  the position of the center in scene coordinates
 */
QPointF GribouillotLayer::drawCircleFromRadius(QColor penColor, int penWidth, QVector<QPointF> positions)
{
    QPointF center = positions[0];

    if (positions[0] != positions[1])
    {
        //positions[0] = center, positions[1] = a random point
        qreal radius = QLineF(center, positions[1]).length();

        drawCircle(penColor, penWidth, center, radius);
    }

    return center;

}


/**
 * @brief   draw a circle from its center and the radius given by user (opens a dialog)
 * @return  the position of the center in scene coordinates
 */
QPointF GribouillotLayer::drawCircleFromRadius(QColor penColor, int penWidth, QPointF center,
                                               qreal scale, QString scaleUnit)
{
    Dlg_circleRadius dialog(this, scaleUnit);
    qreal radius = 0;

    if (dialog.exec() == QDialog::Accepted)
        radius = dialog.getRadiusValue();

    if (radius != 0 && scale != 0)
    {
        /**
         * convert radius value from kilometers to pixels, scale being the km/px scale.
         * radius(px) = radius(km)*1/(km/px) = radius(km)*px/km = px
         */
        radius *= 1.0/scale;
        drawCircle(penColor, penWidth, center, radius);

    }

    return center;

}


/**
 * @brief   draw a circle from a selected segment taken as diameter
 * \return  the position of the center in scene coordinates
 */
QPointF GribouillotLayer::drawCircleFromDiameter(QColor penColor, int penWidth,
                                                 QLineF58 selectedDiameter)
{
    QPointF center = QPointF();

    if( !selectedDiameter.isNull() )
    {
        center = selectedDiameter.center();
        qreal radius = selectedDiameter.length()/2.0;

        drawCircle(penColor, penWidth, center, radius);
    }

    return center;
}


/**
 * @brief   draw a circle from 2 points which define a diameter.
 * \return  the position of the center in scene coordinates
 */
QPointF GribouillotLayer::drawCircleFromDiameter(QColor penColor, int penWidth, QVector<QPointF> positions)
{
    QPointF center = QPointF();

    if (positions[0] != positions[1])
    {
        //positions[0], positions[1] = 2 random points
        QLineF58 diameter(positions[0], positions[1]);
        center = diameter.center();
        qreal radius = diameter.length()/2.0;

        drawCircle(penColor, penWidth, center, radius);
    }

    return center;
}


/**
 * @brief   draw a circle from the 3 currently selected points.
 * \return  the position of the center in scene coordinates
 */
QPointF GribouillotLayer::drawCircleFromTriangle(QColor penColor, int penWidth)
{
    QPointF center = QPointF();
    QVector<QPointF> selectedPoints;

    for(int i=0;  i < itemsList.size(); ++i)
    {
        if( itemsList.at(i)->isSelected() && itemsList.at(i)->type() == POINT_W )
        {
            Item_point* point =  qgraphicsitem_cast<Item_point*>(itemsList.at(i));
            selectedPoints << point->pos();
        }
    }

    if(selectedPoints.size() == 3)
        center = drawCircleFromTriangle(penColor, penWidth, selectedPoints);

    return center;
}


/**
 * @brief   draw a circle from the 3 points of the circumference chosen by user.
 * \return  the position of the center in scene coordinates
 */
QPointF GribouillotLayer::drawCircleFromTriangle(QColor penColor, int penWidth, QVector<QPointF> positions)
{
    QPointF center = QPointF();

    if (positions[0] != positions[1]
        && positions[1] != positions[2]
        && positions[0] != positions[2])
    {
        //Take 2 sides of the triangle
        QLineF58 side1(positions[0], positions[1]);
        QLineF58 side2(positions[0], positions[2]);
        //Get their 2 bisection vector
        QLineF normal1 = QLineF(side1.center(), side1.p2()).normalVector();
        QLineF normal2 = QLineF(side2.center(), side2.p2()).normalVector();
        //Find the intersection of the 2 bisections, which is the circle center
        normal1.intersects(normal2, &center);
        qreal radius = QLineF(center, positions[0]).length();

        drawCircle(penColor, penWidth, center, radius);
    }

    return center;

}



/****************** ARCS DRAWINGS  ******************/

/**
 * @brief   Draw an arc from saved XML data
 * \return  the position of the center in scene coordinates
 */
QPointF GribouillotLayer::drawArc(QColor penColor, int penWidth, QPointF center, qreal radius,
                                  qreal startAngle, qreal spanAngle)
{
    addItemToLayer( new Item_arc(center, radius, startAngle, spanAngle, penColor, penWidth) );

    return center;

}


/**
 * @brief   Draw an arc with visual help
 * @return  the position of the center in scene coordinates
 */
QPointF GribouillotLayer::drawArc(QColor penColor, int penWidth, Item_arc* arc)
{
    QPointF center = QPointF();

    if(arc != nullptr)
    {
        center = arc->scenePos();

        if( (360 - qAbs(arc->getSpanAngle())) < 1 )
        {
            //Almost closed arc, draw a circle instead...
            drawCircle(penColor, penWidth, center, arc->getRadius());
            delete arc;
        }
        else
        {
            //set the drawing color and thickness
            arc->newPen(penColor, penWidth);
            addItemToLayer(arc);
        }
    }

    return center;

}


/**
 * @brief   Draw an arc from a circle with visual help
 * \return  the position of the center in scene coordinates
 */
QPointF GribouillotLayer::drawArcFromCircle(QColor penColor, int penWidth,
                                        Item_circle* sourceCircle, Item_arc *arc)
{
    QPointF center = QPointF();

    if(arc != nullptr)
    {
        center = arc->scenePos();

        //set the drawing color and thickness
        arc->newPen(penColor, penWidth);

        /*
         * Because we want to maintain independence between layers, the source circle
         * will be destroyed if and only if it belongs to the current layer.
         */
        if (itemsList.contains(sourceCircle))
        {
            //Delete the circle from which the arc is extracted
            delete itemsList.takeAt( itemsList.indexOf(sourceCircle) );
        }

        addItemToLayer(arc);

    }

    return center;
}



/*************** SPIRAL DRAWINGS *****************/
/**
 * @brief   Draw a spiral as independent arcs
 * @details Each arc can be selected separately
 */
void GribouillotLayer::drawSpiral(QColor penColor, int penWidth, QVector<QPointF> centers)
{
    addItemToLayer( new Item_spiral(penColor, penWidth, centers) );

}



/*************** PIXMAP DRAWINGS *****************/


/**
 * @brief   Load a pixmap
 */
void GribouillotLayer::drawPixmap(bool doCopy)
{
    //calls to global functions
    QString supportedFormats = getSupportedImageFormats();
    QString defaultFilter = getDefaultImageFilter(supportedFormats);
    QString pixPath ="";

    pixPath = QFileDialog::getOpenFileName(
                    this,
                    tr("Select an image file"),
                    QDir::currentPath(),
                    supportedFormats,
                    &defaultFilter);

    if (!pixPath.isEmpty())
    {
        if( doCopy )
        {
            //copy pixmap to current folder
            QFileInfo pixFileInfo(pixPath);
            QString newPath = "./"+ pixFileInfo.fileName();
            QFile::copy(pixPath, newPath);
            pixPath = newPath;
        }

        addItemToLayer(new Item_pixmap(pixPath));
    }

}

