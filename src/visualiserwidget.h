#ifndef VISUALISERWIDGET_H
#define VISUALISERWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QTime>
#include <QRandomGenerator>
#include <QVector>

namespace Ui {
class visualiserWidget;
}

enum class visualiserMode {
    Bar = 0,
    Circle = 1,
    Triangle = 2
};

struct visualiserTriangle{
    double centerx, centery;
    double direction, speed;
    double angle1, angle2, angle3;
    double length1, length2, length3;
    int moveDirection, rotateDirection;
    QColor color;
    visualiserTriangle(double cx, double cy, bool fromButtom = false){
        centerx = cx;
        centery = cy;
        if(fromButtom){
            centerx = QRandomGenerator::global()->bounded(0,static_cast<int>(cx));
            centery = cy+30;
            moveDirection = centerx >= cx/2 ? -1 : 1; // move left or right
        }
        direction = QRandomGenerator::global()->bounded(M_PI*2);
        rotateDirection = QRandomGenerator::global()->bounded(2) ? 1 : -1; // rotate left or right
        angle1 = QRandomGenerator::global()->bounded(M_PI*2);
        angle2 = QRandomGenerator::global()->bounded(M_PI*2);
        angle3 = QRandomGenerator::global()->bounded(M_PI*2);
        length1 = QRandomGenerator::global()->bounded(15, 35);
        length2 = QRandomGenerator::global()->bounded(15, 35);
        length3 = QRandomGenerator::global()->bounded(15, 35);
        speed = static_cast<double>(static_cast<float>(QRandomGenerator::global()->bounded(20,28))/10.f);
        color = QColor::fromHsvF(static_cast<float>(QRandomGenerator::global()->bounded(1.0f)), 0.4f, 1.0f, 0.5f);
    }
};

class visualiserWidget : public QWidget
{
    Q_OBJECT
private:
    QVector<double> _data, _buffer, _xP, _yP;
    const int _numBins;
    QTimer* _timer, *_timer2, *_timer3, *_timer4;
    bool _isPlaying, _inThisWidget;
    QVector<visualiserTriangle> _triangles;
    double _radius, _bar_length, _angleDelta, _rotateSpeed, _total, _last_total;
    double  _fps, _fixed_update_fps;
    QColor _barColor, _circleColor, _triangleColor;
    QVector<bool> _vmode;

public:
    explicit visualiserWidget(QWidget *parent = nullptr);
    ~visualiserWidget();
    void setData(double* data, double total);
    void WidgetChange(int index);
    void SetMode(visualiserMode mode, bool enabled = true);
protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void printTriangles(QPainter& painter);
    void printSpectrum(QPainter& painter);
    void rotate();
    void updateSpectrum();
    void updateTriangles();
    void fixedUpdate();

private:
    Ui::visualiserWidget *ui;
};

#endif // VISUALISERWIDGET_H
