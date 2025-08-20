#include "bettertableview.h"
#include "ui_bettertableview.h"
#include <QScroller>
#include <QScrollBar>
#include <QEasingCurve>

BetterTableView::BetterTableView(QWidget *parent)
    : QTableView(parent)
    , _scrollAnimation(QSharedPointer<QPropertyAnimation>::create(verticalScrollBar(), "value", this))
    , _targetScrollValue(0)
    , ui(new Ui::BetterTableView)
{
    ui->setupUi(this);

    _scrollAnimation->setDuration(120);
    _scrollAnimation->setEasingCurve(QEasingCurve::OutQuad);
}

BetterTableView::~BetterTableView()
{
    delete ui;
}

void BetterTableView::wheelEvent(QWheelEvent* event){
    // QScroller* scroller(QScroller::scroller(viewport()));
    // if(!scroller) return QTableView::wheelEvent(event);
    // if (scroller->state() == QScroller::Inactive) {
    //     scroller->handleInput(QScroller::InputPress, event->position(), static_cast<qint64>(event->timestamp()));
    // }
    // QPointF delta(0, -(event->angleDelta().y()));
    // scroller->handleInput(QScroller::InputMove, event->position() + delta, static_cast<qint64>(event->timestamp()));
    // scroller->handleInput(QScroller::InputRelease, event->position() + delta, static_cast<qint64>(event->timestamp()));

    // QScroller* scroller = QScroller::scroller(viewport());
    // QScrollBar* vScrollBar = verticalScrollBar();

    // scroller->stop();

    // QPointF currentPos(0,vScrollBar->value());
    
    // int delta = event->angleDelta().y();
    // QPointF newPos = currentPos - QPointF(0, delta);

    // qDebug() << "currentPos:" << currentPos <<" newPos:" << newPos << " delta:" << delta;
    
    // scroller->scrollTo(newPos, 500);

    QScrollBar* vScrollBar = verticalScrollBar();
    
    int delta = event->angleDelta().y();

    _targetScrollValue = vScrollBar->value() - delta;
    
    _targetScrollValue = qBound(vScrollBar->minimum(), _targetScrollValue, vScrollBar->maximum());

    _scrollAnimation->stop();
    _scrollAnimation->setStartValue(vScrollBar->value());
    _scrollAnimation->setEndValue(_targetScrollValue);
    _scrollAnimation->start();
}

