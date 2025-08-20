#ifndef BETTERTABLEVIEW_H
#define BETTERTABLEVIEW_H

#include <QWidget>
#include <QTableView>
#include <QWheelEvent>
#include <QPropertyAnimation>
#include <QSharedPointer>

namespace Ui {
class BetterTableView;
}

class BetterTableView : public QTableView
{
    Q_OBJECT

public:
    explicit BetterTableView(QWidget *parent = nullptr);
    ~BetterTableView();

protected:
    void wheelEvent(QWheelEvent *event) override;

protected:
    QSharedPointer<QPropertyAnimation> _scrollAnimation;
    int _targetScrollValue;

private:
    Ui::BetterTableView *ui;
};

#endif // BETTERTABLEVIEW_H
