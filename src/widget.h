#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPixmap>
#include <QGraphicsBlurEffect>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QPushButton>
#include "processobj.h"
#include "include/fftw/fftw3.h"
#include "thememanager.h"
#include "include/ffmpeg/libavcodec/codec.h"
#include "lyricswidget.h"
#include "notificationwidget.h"
#include <QPoint>

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT
protected:
    enum class Edge {
        None,
        Left,
        Right,
        Top,
        Buttom,
        LeftTop,
        LeftButtom,
        RightTop,
        RightButtom
    };

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

public slots:
    void FreshTables(PlayListType type, QString listName = QString());//fresh content of table
    void SentMsg(int type, QString msg);//change msg content and img
    void HideMsg();//hide msg||switch to label

protected:
    //lambda functions
    void SetBackground(QString path = QString());
    void UpdateBackgroundSize();//update currentHei and currentWid
    void InitTableAttribute(QTableView* table);
    //component width/height, pic width/height, target width/height
    static void FitPicSize(int cw, int ch, int pw, int ph, int &tw, int &th);
    static QPixmap CreateCircularCover(const QPixmap &pixmap, const QSize &targetSize);
    //void SetButtonStyle(QPushButton* button, const QString& path)=0;//removed //use themeManager.SetComponentProperties
    void SetLabelStyle(QLabel* label, const QString& path);
    QString FormatTime(qint64 ms);//provide a str for timer above the progress bar
    QString GenerateTitle();//select a random title
    Edge AtEdge(const QPoint& pos);
    void FreshBiliTable();
    //void UpdateCursor(Edge edge);
    void SetFrameless(bool frameless = true);
    void UpdateWindowRegion(bool enable);
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    [[deprecated("Use FitPicSize instead")]]
    void FitBackground(int cw, int ch, int pw, int ph, int &tw, int &th);
    void SetWStyles(long int styles);
    void PaintBackground(QPaintEvent* event, bool effectOn = false);
    // void EffectSetup();
    // void ApplyEffect(QPixmap* pixmap);
    QPixmap* RenderToBuffer(bool check = true);
    
protected:
    LyricsWidget* LyricsArea;
    ThemeManager themeManager;
    ProcessObj mainObj;
    QList<QString> titleList, supportImg;
    //QLabel *backgroundLabel;
    QLabel* titleText;
    QPixmap* bgPixmap, *bufferedPixmap;
    QString bgPath;
    // QGraphicsBlurEffect* blurEffect;
    // QGraphicsScene* scene;
    // QGraphicsPixmapItem* pixmapItem;
    int picWidth,picHeight,currentWid,currentHei,originalWid,originalHei;
    QPointF dragPosition; // for dragging the window
    const int edgeWidth = 10;
    const int cornerRadius = 17; // for rounded corners
    Edge edge;
    QRect startGeometry;
    bool isResizing, bgRedraw;
    qint64 click_pos;
    
    


private:
    Ui::Widget *ui;
};
#endif // WIDGET_H
