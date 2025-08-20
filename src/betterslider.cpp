#include "betterslider.h"
#include "ui_betterslider.h"

BetterSlider::BetterSlider(QWidget *parent, Qt::Orientation orientation)
    : QSlider(orientation, parent)
    , minValue(0.0), maxValue(1.0), currentValue(0.0), dRange(1.0)
    , insideMinimum(0), insideMaximum(10000), insideRange(insideMaximum - insideMinimum)
    , ui(new Ui::BetterSlider)
{
    ui->setupUi(this);
    setRange(insideMinimum, insideMaximum);
    connect(this, &BetterSlider::sliderMoved, this, &BetterSlider::SliderMoved);
    connect(this, &BetterSlider::sliderPressed, this, &BetterSlider::SliderPressed);
}

void BetterSlider::SetRange(double min, double max, ValueLimit limit){
    if(min >= max) return;
    double cnt = currentValue;
    switch(limit){
        case ValueLimit::Percentage:{
            double per = GetPercentage();
            minValue = min;
            maxValue = max;
            dRange = maxValue - minValue;
            SetValue(per * dRange + minValue);
            break;
        }
        case ValueLimit::BoundLimit:{
            minValue = min;
            maxValue = max;
            dRange = maxValue - minValue;
            SetValue(qBound(minValue, cnt, maxValue));
            break;
        }
    }
}

void BetterSlider::SetValue(double value){
    value = qBound(minValue, value, maxValue);
    currentValue = value;
    setValue(GetValueInt());
}

double BetterSlider::GetPercentage(){
    return (currentValue - minValue) / (maxValue - minValue);
}

double BetterSlider::GetValue(){
    return currentValue;
}

int BetterSlider::GetValueInt(){
    double percentage = GetPercentage();
    double range = static_cast<double>(insideMaximum - insideMinimum);
    return static_cast<int>(percentage * range) + insideMinimum;
}

void BetterSlider::SliderMoved(int position){
    currentValue = minValue + 
    (static_cast<double>(value() - insideMinimum) / 
    static_cast<double>(insideMaximum - insideMinimum)) * dRange;
    emit m_sliderMoved(position);
}

void BetterSlider::SliderPressed(){
    currentValue = minValue + 
    (static_cast<double>(value() - insideMinimum) / 
    static_cast<double>(insideMaximum - insideMinimum)) * dRange;
    emit m_sliderPressed();
}

BetterSlider::~BetterSlider()
{
    delete ui;
}
