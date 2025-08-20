#include "LyricsWidget.h"
#include <QPainter>
#include <QVBoxLayout>

LyricsWidget::LyricsWidget(QWidget *parent)
    : QWidget(parent)
    , _player(nullptr)
    , _currentTime(0)
    , _currentIndex(0)
    , _lastIndex(0)
    , _offset(0)
    , _currentTextWidth(0)
    , _timer(this)
    , _scrollTimer(this)
    , _offTime(3000) {
    connect(&_timer, &QTimer::timeout, this, QOverload<>::of(&LyricsWidget::update));
    connect(&_scrollTimer, &QTimer::timeout, this, [this](){
        if(_offset>0) --_offset;
    });
    _currentFont = font();
    _otherFont = font();
    _currentFont.setPointSize(18);
    _currentFont.setBold(true);
    _otherFont.setPointSize(16);
    _colorH = QColor(153, 255, 204);
    _colorL = QColor(170,170,255,170);
    _timer.start(1000/60);
    _scrollTimer.start(500/24);
}

void LyricsWidget::SetLyrics(const QList<QString> lyric, const QList<int>time ) {
    QMap<qint64, QString> lyrics;
    for(int i = 0; i < lyric.size(); i++) {
        lyrics.insert(time[i], lyric[i]);
    }
    _lyrics = lyrics;
    _times = time;
    _lastIndex = 0;
    update();
}
void LyricsWidget::SetMediaPlayer(QMediaPlayer *player) {
    _player = player;
}

void LyricsWidget::updateIndex(qint64 position){
    _currentTime = position;
    for (int i = 0; i < _lyrics.size(); ++i) {
        if (_lyrics.keys().at(i) > position) {
            _currentIndex = i - 1;
            break;
        }
    }
}

void LyricsWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    if (_lyrics.isEmpty()) {
        return;
    }
    updateIndex(_player->position());
    if(_currentIndex != _lastIndex){
        _lastIndex = _currentIndex;
        _offset = 32;
    }
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QColor(170, 170, 255)); 

    int y = height() / 2;
    int lineHeight = 32;
    // float progress = GetProgress();
    // int p_width = static_cast<int>(static_cast<float>(width()) * progress);

    painter.setFont(_currentFont);
    if (_currentIndex != -1) {
        DrawProgressLyric(painter, _lyrics.value(_times.at(_currentIndex)), 0, y + _offset, width(), lineHeight);
    }

    painter.setFont(_otherFont);
    painter.setPen(_colorL);
    for (int i = _currentIndex - 1, posY = y - lineHeight + _offset; i >= 0 && posY > 0; --i, posY -= lineHeight) {
        painter.drawText(0, posY, width(), lineHeight, Qt::AlignCenter, _lyrics.value(_times.at(i)));
    }
    for (int i = _currentIndex + 1, posY = y + lineHeight + _offset; i < _lyrics.size() && posY < height(); ++i, posY += lineHeight) {
        painter.drawText(0, posY, width(), lineHeight, Qt::AlignCenter, _lyrics.value(_times.at(i)));
    }
}

void LyricsWidget::DrawProgressLyric(QPainter& painter, const QString& text, int x, int y, int width, int height) {
    if (_currentIndex == -1 || _currentIndex >= _times.size()) return;
    double progress = GetProgress();
    
    QFontMetrics fm(_currentFont);
    //QRect textRect = fm.boundingRect(text);
    int textWidth = fm.horizontalAdvance(text);
    int progressWidth = static_cast<int>(textWidth * progress);
    
    int textX = x + (width - textWidth) / 2;
    // int textY = y;
    
    painter.setPen(_colorL);
    painter.drawText(textX, y, textWidth, height, Qt::AlignCenter, text);
    
    if (progressWidth > 0) {
        painter.save();
        //painter.setClipRect(textX, y, progressWidth + fm.leading(), height);
        painter.setPen(_colorH); 
        painter.drawText(textX, y, progressWidth + fm.leading(), height, Qt::AlignLeft|Qt::AlignVCenter, text);
        painter.restore();
    }
}

float LyricsWidget::GetProgress(){
    int i_pos = static_cast<int>(_player->position());
    int start = _times.at(_currentIndex);
    int end = _currentIndex+1<_times.size()-1?_times[_currentIndex+1]:_offTime+start;
    float result = static_cast<float>(i_pos - start) / static_cast<float>(end - start) * 1.2f;
    return qBound(0.0f, result, 1.0f);
}

void LyricsWidget::resizeEvent(QResizeEvent *event) {
    Q_UNUSED(event);
    update();
}

