#include "biliinfowidget.h"
#include "ui_biliinfowidget.h"
#include <QPainter>
#include <QFontMetrics>


BiliInfoWidget::BiliInfoWidget(QWidget *parent)
    : QWidget(parent)
    , _biliInfo()
    , _pixmap()
    , _scaledPixmap()
    , _picRatio(0.3f)
    , ui(new Ui::BiliInfoWidget)
{
    ui->setupUi(this);
}

BiliInfoWidget::~BiliInfoWidget()
{
    delete ui;
}

void BiliInfoWidget::UpdatePic(){
    if(_pixmap.isNull()) return;
    float widthRatio = static_cast<float>(_pixmap.width()) / (static_cast<float>(width()) * _picRatio);
    float heightRatio = static_cast<float>(_pixmap.height()) / static_cast<float>(height());
    float maxiRatio = qMax(widthRatio, heightRatio);
    if(maxiRatio <= 1.0f){
        _scaledPixmap = _pixmap;
        return;
    }
    _scaledPixmap = _pixmap.scaled(
        static_cast<int>(static_cast<float>(_pixmap.width()) / maxiRatio), 
        static_cast<int>(static_cast<float>(_pixmap.height()) / maxiRatio), 
    Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

void BiliInfoWidget::SetInfo(const BiliInfo& info)
{
    _biliInfo = info;
    if (!_biliInfo.picPath.isEmpty()) {
        _pixmap.load(_biliInfo.picPath);
        UpdatePic();
        update();
    }
}

void BiliInfoWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    QPainter painter(this);
    if(!_scaledPixmap.isNull()){
        int wid = static_cast<int>(static_cast<float>(width()) * _picRatio);
        int x = (wid - _scaledPixmap.width()) / 2;
        int y = (height() - _scaledPixmap.height()) / 2;
        painter.drawPixmap(QRect(x, y, _scaledPixmap.width(), _scaledPixmap.height()), _scaledPixmap);
    }

    int textX = static_cast<int>(static_cast<float>(width()) * _picRatio);
    const int margin = 10;
    const int spacing = 8;
    QRect infoRect(textX, 0, width() - textX, height());

    QFont defaultFont = painter.font();
    defaultFont.setPointSize(9);
    painter.setFont(defaultFont);

    int yPos = infoRect.top() + margin;
    int xPos = infoRect.left() + margin;
    int maxWidth = infoRect.width() - 2 * margin;
    
    QFont titleFont = defaultFont;
    titleFont.setBold(true);
    titleFont.setPointSize(12);
    painter.setFont(titleFont);
    painter.setPen(QColor(0, 161, 214));
    QRect titleRect(xPos, yPos, maxWidth, 0);
    titleRect = painter.boundingRect(titleRect, Qt::TextWordWrap, _biliInfo.title);
    painter.drawText(titleRect, Qt::TextWordWrap, _biliInfo.title);
    yPos = titleRect.bottom() + spacing;
    
    painter.setPen(QPen(QColor(220, 220, 220), 1));
    painter.drawLine(xPos, yPos, xPos + maxWidth, yPos);
    yPos += spacing + 2;
    
    painter.setFont(defaultFont);
    painter.setPen(QColor(170,170,255));
    int descHeight = infoRect.bottom() - yPos - margin - 30;
    QRect descRect(xPos, yPos, maxWidth, descHeight);
    painter.drawText(descRect, Qt::TextWordWrap | Qt::AlignTop, _biliInfo.desc);

    QFont pFont = defaultFont;
    pFont.setBold(true);
    painter.setFont(pFont);
    painter.setPen(QColor(251, 114, 153));
    QString pText = QString("分P数: %1").arg(_biliInfo.videos);
    QRect pRect(xPos, infoRect.bottom() - margin - 20, maxWidth, 20);
    painter.drawText(pRect, Qt::AlignLeft | Qt::AlignVCenter, pText);
}

void BiliInfoWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    UpdatePic();
    update();
}
