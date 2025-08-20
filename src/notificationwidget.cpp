#include "notificationwidget.h"
#include "ui_notificationwidget.h"
#include <QPainter>

NotificationWidget::NotificationWidget(const QString &text, int type, QWidget *parent)
    : QWidget(parent), iconLabel(new QLabel(this)), textLabel(new QLabel(this)), fadeTimer(this),
      m_wt(110),m_wp(32),m_h(32)
    {
    iconLabel->setScaledContents(false);
    iconLabel->setFixedSize(m_wp,m_h);
    if(type==0){
        iconLabel->setPixmap(QPixmap(":/icon/source/check-circle.png").scaled(m_wp,m_h,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    }
    else if(type==1){
        iconLabel->setPixmap(QPixmap(":/icon/source/info-circle.png").scaled(m_wp,m_h,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    }
    else{
        iconLabel->setPixmap(QPixmap(":/icon/source/exclamation-circle.png").scaled(m_wp,m_h,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    }
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setStyleSheet("background-color: transparent;");
    textLabel->setStyleSheet("background-color: rgba(107,107,155,30);"
                             "border-radius: 3px;"
                             "font-size: 18px;"
                             "font_family: 楷体;");
    //textLabel->setFixedSize(m_wt,m_h);
    textLabel->setText(text);
    textLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(iconLabel);
    layout->addWidget(textLabel);
    layout->setSpacing(8);
    layout->setContentsMargins(0,0,0,0);
    //layout->setSizeConstraint(QLayout::SetFixedSize);

    fadeTimer.setSingleShot(true);
    connect(&fadeTimer, &QTimer::timeout, this, &NotificationWidget::deleteLater);
}

void NotificationWidget::startFadeOut() {
    fadeTimer.start(5000); // Display the message for 5 seconds
}

void NotificationWidget::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);
    QPainter painter(this);
    painter.setOpacity(0.5); // Set the opacity for fade effect
}
