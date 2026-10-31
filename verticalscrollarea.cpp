//
// Created by SuperTuxi on 10.06.25.
//

#include <QEvent>
#include <QScrollBar>

#include "verticalscrollarea.h"

VerticalScrollArea::VerticalScrollArea(QWidget *parent, const int minimumWidth) : QScrollArea(parent) {
    this->minimumWidth = minimumWidth;
    setMinimumWidth(minimumWidth);
    setWidgetResizable(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

bool VerticalScrollArea::eventFilter(QObject *o, QEvent *e) {
    // This works because QScrollArea::setWidget installs an eventFilter on the widget
    if(o && o == widget() && e->type() == QEvent::Resize)
        setMinimumWidth(std::max(minimumWidth, widget()->minimumSizeHint().width() + verticalScrollBar()->width()));

    return QScrollArea::eventFilter(o, e);
}
