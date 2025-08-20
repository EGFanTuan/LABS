#ifndef TITLEWIDGET_H
#define TITLEWIDGET_H

#include <QWidget>
#include <QMouseEvent>
#include <QPoint>
#include <QStyleOption>
#include <QPainter>

namespace Ui {
class titleWidget;
}

class titleWidget : public QWidget
{
    Q_OBJECT

private:

public:
    explicit titleWidget(QWidget *parent = nullptr);
    ~titleWidget();
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent *event) override {
        Q_UNUSED(event);
        QStyleOption opt;
        opt.initFrom(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    }

signals:
    void pressed(QMouseEvent* event);
    void moved(QMouseEvent* event);
    void doubleClicked(QMouseEvent* event);


private:
    Ui::titleWidget *ui;
};

#endif // TITLEWIDGET_H
