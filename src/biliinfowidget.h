#ifndef BILIINFOWIDGET_H
#define BILIINFOWIDGET_H

#include <QWidget>
#include <QPixmap>
#include "biliinfo.h"

namespace Ui {
class BiliInfoWidget;
}

class BiliInfoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BiliInfoWidget(QWidget *parent = nullptr);
    ~BiliInfoWidget();
    void SetInfo(const BiliInfo& info);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void UpdatePic();

protected:
    BiliInfo _biliInfo;
    QPixmap _pixmap, _scaledPixmap;
    const float _picRatio;

private:
    Ui::BiliInfoWidget *ui;
};

#endif // BILIINFOWIDGET_H
