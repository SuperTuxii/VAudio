//
// Created by SuperTuxi on 10.06.25.
//

#ifndef VERTICALSCROLLAREA_H
#define VERTICALSCROLLAREA_H

#include <QScrollArea>

class VerticalScrollArea : public QScrollArea
{
    Q_OBJECT
public:
    explicit VerticalScrollArea(QWidget *parent = nullptr, int minimumWidth = 0);

    bool eventFilter(QObject *o, QEvent *e) override;
private:
    int minimumWidth;
};

#endif // VERTICALSCROLLAREA_H
