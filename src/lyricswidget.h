#ifndef LYRICSWIDGET_H
#define LYRICSWIDGET_H

#include <QWidget>
#include <QMap>
#include <QTimer>
#include <QMediaPlayer>

class LyricsWidget : public QWidget {
    Q_OBJECT

public:
    LyricsWidget(QWidget *parent = nullptr);
    void SetLyrics(const QList<QString> lyric, const QList<int>time);//
    void SetMediaPlayer(QMediaPlayer* player);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    float GetProgress();
    void DrawProgressLyric(QPainter& painter, const QString& text, int x, int y, int width, int height);

private slots:
    void updateIndex(qint64 position);

private:
    QMap<qint64, QString> _lyrics;
    QMediaPlayer* _player;
    qint64 _currentTime;
    int _currentIndex, _lastIndex, _offset, _currentTextWidth;
    QTimer _timer, _scrollTimer;
    QList<int> _times;
    QFont _currentFont, _otherFont;
    const int _offTime;
    QColor _colorH, _colorL;
};



#endif // LYRICSWIDGET_H
