#include "visualiserwidget.h"
#include "ui_visualiserwidget.h"
#include <QPainter>
#include <QTimer>
#include <QPainterPath>

visualiserWidget::visualiserWidget(QWidget *parent)
    : QWidget(parent)
    , _data(64)
    , _buffer(64)
    , _xP(64)
    , _yP(64)
    , _numBins(64)
    , _timer(new QTimer(this))
    , _timer2(new QTimer(this))
    , _timer3(new QTimer(this))
    , _timer4(new QTimer(this))
    , _isPlaying(false)
    , _inThisWidget(true)
    , _triangles()
    , _radius(140.0)
    , _bar_length(25.0)
    , _angleDelta(0)
    , _rotateSpeed(0)
    , _total(0)
    , _last_total(1E-5)
    , _fps(60)
    , _fixed_update_fps(60)
    , _vmode(3,true)
    , ui(new Ui::visualiserWidget)
{
    ui->setupUi(this);
    connect(_timer, &QTimer::timeout, this, [this](){
        update();
    });
    connect(_timer2, &QTimer::timeout, this, [this](){
        _isPlaying = false;
    });
    connect(_timer3, &QTimer::timeout, this, [this](){
        if(_isPlaying && _inThisWidget) _triangles.append(visualiserTriangle(width(), height(), true));
        auto it = _triangles.begin();
        while (it != _triangles.end()) {
            if (it->centery <= -70) {
                it = _triangles.erase(it);
            } else {
                ++it;
            }
        }
    });
    connect(_timer4, &QTimer::timeout, this, [this](){
        fixedUpdate();
    });
    for(int i=0;i<_numBins;i++){
        _data[i] = 0.3;
        _buffer[i] = 0.3;
    }
    _timer->start(1000 / static_cast<int>(_fps));
    _timer2->start(100);
    _timer3->start(150);
    _timer4->start(1000 / static_cast<int>(_fixed_update_fps));
}

void visualiserWidget::paintEvent(QPaintEvent *event){
    Q_UNUSED(event);
    if(_data.isEmpty()) return;
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    printTriangles(painter);
    printSpectrum(painter);
    QWidget::paintEvent(event);
}

void visualiserWidget::printSpectrum(QPainter& painter){
    int center_x = width() / 2, center_y = height() / 2;
    double angle;
    double lengthx, lengthy;
    double x1, y1, x2, y2;
    double co,si;
    if(_vmode[static_cast<int>(visualiserMode::Bar)]){
        for (int i = 0;i<_numBins; ++i) {
            angle = 2 * M_PI * i / _numBins + _angleDelta;
            co = cos(angle);si=sin(angle);
            lengthx = (_data[i] * co * _bar_length / (log(_data[i] + 2.5)));
            lengthy = (_data[i] * si * _bar_length / (log(_data[i] + 2.5)));
            x1 = center_x + co * _radius;
            y1 = center_y + si * _radius ;
            x2 = center_x + co * _radius + lengthx;
            y2 = center_y + si * _radius + lengthy;
            _xP[i] = x2 + 8*co;
            _yP[i] = y2 + 8*si;
            QPen pen(QColor::fromHsvF(static_cast<float>(i) / static_cast<float>(_numBins), 0.45f, 1.0f));
            pen.setWidth(2);
            pen.setCapStyle(Qt::RoundCap);
            painter.setPen(pen);
            painter.drawLine(QPointF(x1, y1), QPointF(x2, y2));
        }
    }
    if(_vmode[static_cast<int>(visualiserMode::Circle)]){
        for(int i=0;i<_numBins;i++){
            QPen pen(QColor::fromHsvF(static_cast<float>(i) / static_cast<float>(_numBins), 0.25f, 1.0f));
            pen.setWidth(1);
            pen.setStyle(Qt::DotLine);
            painter.setPen(pen);
            painter.drawLine(QPointF(_xP[i], _yP[i]), QPointF(_xP[(i+1)%_numBins], _yP[(i+1)%_numBins]));
        }
    }
}

void visualiserWidget::printTriangles(QPainter& painter){
    if(_vmode[static_cast<int>(visualiserMode::Triangle)]){
        for(visualiserTriangle& t:_triangles){
            QPen pen(t.color);
            double x1 = t.centerx + cos(t.angle1)*t.length1;
            double y1 = t.centery + sin(t.angle1)*t.length1;
            double x2 = t.centerx + cos(t.angle2)*t.length2;
            double y2 = t.centery + sin(t.angle2)*t.length2;
            double x3 = t.centerx + cos(t.angle3)*t.length3;
            double y3 = t.centery + sin(t.angle3)*t.length3;

            QColor fillColor = QColor(255, 255, 255);
            fillColor.setAlpha(70);  
            QPainterPath path;
            path.moveTo(x1, y1);
            path.lineTo(x2, y2);
            path.lineTo(x3, y3);
            path.closeSubpath();
            painter.fillPath(path, fillColor);

            pen.setWidth(2);
            painter.setPen(pen);
            painter.drawLine(QPointF(x1, y1), QPointF(x2, y2));
            painter.drawLine(QPointF(x2, y2), QPointF(x3, y3));
            painter.drawLine(QPointF(x3, y3), QPointF(x1, y1));
        }
    }
}

void visualiserWidget::resizeEvent(QResizeEvent *event){
    Q_UNUSED(event);
    update();
}

void visualiserWidget::setData(double* pdata, double total){
    for(int i=0;i<_numBins;i++){
        _data[i] = pdata[i]*0.15+_data[i]*0.85;
    }
    _total = total;
    _isPlaying = true;
    //reset timer
    _timer2->start(100);
}

void visualiserWidget::SetMode(visualiserMode mode, bool enabled){
    int idx = static_cast<int>(mode);
    if(idx >= 0 && idx < _vmode.size()){
        _vmode[idx] = enabled;
    }
}


void visualiserWidget::WidgetChange(int index){
    _inThisWidget = index==0;
}

void visualiserWidget::rotate(){
    if(!_isPlaying && _rotateSpeed>0){
        _rotateSpeed -= 0.0001;
    }
    else if(_isPlaying && _rotateSpeed<0.005){
        _rotateSpeed += 0.0001;
    }
    _angleDelta += _rotateSpeed>0.0001?_rotateSpeed:0;
    if(_angleDelta>=M_PI*2) _angleDelta -= M_PI*2;
}

void visualiserWidget::updateSpectrum(){
    rotate();
    if(!_isPlaying){
        for(int i=0;i<_numBins;i++)
        _data[i] = _data[i]*0.85+_buffer[i]*0.15;
    }
    else{

    }
}

void visualiserWidget::updateTriangles(){
    _last_total = std::max(_total*0.05+_last_total*0.95, 1E-5);
    double _chg =std::min(_total/(_last_total+0.00001)*5, 20.0);
    _chg = std::max(_chg, 1.0);
    double speed_scale = _chg;
    double const_speed = 0.25;
    Q_UNUSED(const_speed);
    for(auto& t:_triangles){
        t.centery -= (t.speed/8)* speed_scale;
        t.angle1 += 0.008 * t.rotateDirection * (1+speed_scale/10);
        t.angle2 += 0.01 * t.rotateDirection * (1+speed_scale/10);
        t.angle3 += 0.012 * t.rotateDirection * (1+speed_scale/10);
    }
}

void visualiserWidget::fixedUpdate(){
    updateSpectrum();
    updateTriangles();
}


visualiserWidget::~visualiserWidget()
{
    delete ui;
}
