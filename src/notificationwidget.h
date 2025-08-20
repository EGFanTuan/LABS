#ifndef NOTIFICATIONWIDGET_H
#define NOTIFICATIONWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QPixmap>

class NotificationWidget : public QWidget {
    Q_OBJECT

public:
    explicit NotificationWidget(const QString &text, int type, QWidget *parent = nullptr);
    QLabel *iconLabel;
    QLabel *textLabel;
    QTimer fadeTimer;
    int m_wt,m_wp,m_h;

public slots:
    void startFadeOut();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
};

#endif // NOTIFICATIONWIDGET_H
