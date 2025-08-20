#ifndef BETTERSLIDER_H
#define BETTERSLIDER_H

#include <QWidget>
#include <QSlider>

namespace Ui {
class BetterSlider;
}

class BetterSlider : public QSlider
{
    Q_OBJECT

public:
    enum class ValueLimit{
        Percentage,
        BoundLimit
    };

public:
    explicit BetterSlider(QWidget *parent = nullptr, Qt::Orientation orientation = Qt::Horizontal);
    ~BetterSlider();
    void SetRange(double min, double max, ValueLimit limit = ValueLimit::BoundLimit);
    void SetValue(double value);
    double GetPercentage();
    double GetValue();

protected:
    int GetValueInt();
    void SliderMoved(int position);
    void SliderPressed();
    // void ValueChanged(int value);

private:
    double minValue, maxValue, currentValue, dRange;
    int insideMinimum, insideMaximum, insideRange;
    Ui::BetterSlider *ui;

signals:
    void m_sliderMoved(int position);
    void m_sliderPressed();
};

#endif // BETTERSLIDER_H
