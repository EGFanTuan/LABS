#include "titlewidget.h"
#include "ui_titlewidget.h"

titleWidget::titleWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::titleWidget)
{
    ui->setupUi(this);
}

titleWidget::~titleWidget()
{
    delete ui;
}

void titleWidget::mousePressEvent(QMouseEvent* event)
{
    emit pressed(event);
    event->accept();
}

void titleWidget::mouseMoveEvent(QMouseEvent* event)
{
    emit moved(event);
    event->accept(); 
}

void titleWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    emit doubleClicked(event);
    event->accept();
}
