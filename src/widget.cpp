#include "widget.h"
#include "./ui_widget.h"
#include "lyricswidget.h"
#include "visualiserwidget.h"
#include <QLabel>
#include <QLayout>
#include <QFileDialog>
#include <QPushButton>
#include <QStandardPaths>
#include <QStandardItemModel>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QScrollBar>
#include <QRandomGenerator>
#include <QShortcut>
#include <QShortcutEvent>
#include <QWidget>
#include <QWindow>
#include <QScroller>
#include <qt_windows.h>
#include <Windowsx.h>
#include <dwmapi.h>


Widget::Widget(QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::Window | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint)
    , themeManager(this)
    , mainObj(this)
    , titleList(),supportImg()
    , bgPixmap(nullptr), bufferedPixmap(nullptr)
    , dragPosition(0, 0)
    , edge(Edge::None)
    , isResizing(false)
    , bgRedraw(true)
    , click_pos(-1ll)
    , ui(new Ui::Widget)
{
    titleList = {"Lost Among Beautiful Songs",
                 "Lazy Afternoon Beats Suite",
                 "Leaky Air Balloons Society",
                 "Lemonade And Blueberry Scones",
                 "Lunar Among Beneathing Stars",
                 "Laser Applications in Basic Sciences",
                 "Lunar Astronauts Building Snowmen",
                 "Luminous Astronomy-Based Scenery"};
    supportImg = {":/icon/source/C++.png",
                   ":/icon/source/python.png",
                   ":/icon/source/java.png",
                   ":/icon/source/php.png"};

    QFont font1;
    font1.setFamily("楷体");font1.setPointSize(16);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint);

    

    //init//
    ui->setupUi(this);
    ui->volumeSlider->SetRange(0.0, 100.0);
    ui->volumeSlider->SetValue(static_cast<double>(mainObj.Volume2Level(mainObj.currentVolume)));
    ui->bgAlphaSlider->setSliderPosition(mainObj.currentBlur);
    ui->mainStack->setCurrentIndex(0);
    ui->liteStack->setCurrentIndex(1);
    ui->liteStack->setAttribute(Qt::WA_TransparentForMouseEvents);
    ui->titleTextLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    //lyricsArea
    LyricsArea = new LyricsWidget(this);
    ui->lrcWidget->layout()->addWidget(LyricsArea);
    LyricsArea->SetMediaPlayer(mainObj.GetPlayer());
    //msg
    ui->infoImg->setPixmap(QPixmap(":/icon/source/info-circle.png").scaled(56,56));
    ui->infoText->setPlainText("正在处理设置、歌单、作业、DDL...");
    //coverLabel
    ui->coverLabel->setPixmap(mainObj.currentCover);
    //time label
    ui->timeLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    //backgroundLabel
    SetBackground();
    //visualiser
    ui->visualiser->SetMode(visualiserMode::Triangle, false);
    ui->mainPage_1->SetMode(visualiserMode::Bar, false);
    ui->mainPage_1->SetMode(visualiserMode::Circle, false);

    //setStyleSheet//
    this->setWindowTitle("LABS");
    this->setStyleSheet("background-color: #ffffff;");
    //msg
    ui->infoText->setStyleSheet("QTextEdit {"
                                "border: none;"
                                "background: rgba(55, 55, 55, 0%);"
                                "font-size: 19px;"
                                "font-family: 楷体;"
                                "padding: 0px;"
                                "color: rgba(255,204,255,90%);"
                                "}");
    ui->infoImg->setStyleSheet("QLabel {"
                               "background: transparent;"
                               "}");
    //lyricsArea
    themeManager.SetAllPropertiesInTheme(LyricsArea,"transparent_widget");
    //searchText
    themeManager.SetAllPropertiesInTheme(ui->searchTextEdit,"search_text_edit");
    themeManager.SetAllPropertiesInTheme(ui->biliSearchTextEdit,"search_text_edit");
    themeManager.SetAllPropertiesInTheme(ui->biliDownloadRenameEdit,"search_text_edit");
    // widgets||pages
    themeManager.SetAllPropertiesInTheme(ui->widget, "main_widget"); 
    themeManager.SetAllPropertiesInTheme(this, "main_widget");
    themeManager.SetAllPropertiesInTheme(ui->mainTitleWidget,"title_widget");
    themeManager.SetAllPropertiesInTheme(ui->sideWidget,"side_widget");
    themeManager.SetAllPropertiesInTheme(ui->litePage1,"transparent_widget");
    themeManager.SetAllPropertiesInTheme(ui->litePage2,"transparent_widget");
    themeManager.SetAllPropertiesInTheme(ui->mainPage_1,"transparent_widget");
    themeManager.SetAllPropertiesInTheme(ui->mainPage_2,"transparent_widget");
    themeManager.SetAllPropertiesInTheme(ui->mainPage_3,"transparent_widget");
    themeManager.SetAllPropertiesInTheme(ui->mainPage_4,"transparent_widget");
    themeManager.SetAllPropertiesInTheme(ui->mainPage_5,"transparent_widget");
    themeManager.SetAllPropertiesInTheme(ui->mainStack,"transparent_widget");
    themeManager.SetAllPropertiesInTheme(ui->liteStack,"transparent_widget");
    //controls
    themeManager.SetAllPropertiesInTheme(ui->progressBar,"progress_bar");
    themeManager.SetAllPropertiesInTheme(ui->volumeSlider,"volume_slider");
    ui->bgAlpahLabel->setText("背景模糊->");
    themeManager.SetAllPropertiesInTheme(ui->bgAlphaSlider,"background_alpha_slider");
    themeManager.SetAllPropertiesInTheme(ui->timeLabel,"time_label");  
    themeManager.SetAllPropertiesInTheme(ui->muteButton,"mute_btn"); 
    if(mainObj.currentVolume<0.01){
        ThemeManager::SetComponentProperties(ui->muteButton,"btn_state","muted");
    }
    else{
        ThemeManager::SetComponentProperties(ui->muteButton,"btn_state","unmuted");
    }
    themeManager.SetAllPropertiesInTheme(ui->toHomePage,"home_page_btn");
    themeManager.SetAllPropertiesInTheme(ui->toSearchPage,"search_page_btn");
    themeManager.SetAllPropertiesInTheme(ui->toBiliSearchPage,"bili_search_page_btn");
    // themeManager.SetAllPropertiesInTheme(ui->biliInfoLabel, "bili_label");
    // themeManager.SetAllPropertiesInTheme(ui->biliPicLabel, "bili_label");
    themeManager.SetAllPropertiesInTheme(ui->toLovePage,"love_page_btn");
    themeManager.SetAllPropertiesInTheme(ui->toHistoryPage,"history_page_btn");
    themeManager.SetAllPropertiesInTheme(ui->openFromLocal,"open_from_local_btn");
    themeManager.SetAllPropertiesInTheme(ui->pauseOrPlay,"play_btn");
    themeManager.SetAllPropertiesInTheme(ui->nextSong,"next_song_btn");
    themeManager.SetAllPropertiesInTheme(ui->prevSong,"prev_song_btn");
    themeManager.SetAllPropertiesInTheme(ui->searchButton,"search_btn");
    themeManager.SetAllPropertiesInTheme(ui->biliSearchButton,"bili_search_btn");
    themeManager.SetAllPropertiesInTheme(ui->switchBg,"switch_background_btn");
    themeManager.SetAllPropertiesInTheme(ui->timeImgLabel,"time_image_label");
    themeManager.SetAllPropertiesInTheme(ui->loveButton,"love_btn");
    themeManager.SetAllPropertiesInTheme(ui->changePlayMode,"play_mode_btn");
    themeManager.SetAllPropertiesInTheme(ui->exitBtn,"exit_btn");
    //consider about put all calls in a array
    //and use a loop to call them inside themeManager

    SetLabelStyle(ui->pBImg,[&](){return supportImg[QRandomGenerator::global()->generate()%supportImg.size()];}());
    //titleText(left and right)
    //haven't implemented in ThemeManager
    titleText = ui->titleTextLabel;
    titleText->setStyleSheet("QLabel {"
                             "color: black;"
                             "background-color: transparent;"
                             "padding-left: 5px;"
                             "}");
    ui->titleTextLabel->setAlignment(Qt::AlignLeft);
    ui->titleTextLabel->setTextFormat(Qt::TextFormat::RichText);
    ui->titleTextLabel->setText(GenerateTitle());
    ui->bgAlpahLabel->setStyleSheet("QLabel {"
                                    "color: rgba(170, 170, 255, 70%);"
                                    "font-size: 12px;"
                                    "font-family: 楷体;"
                                    "}");
    ui->poweredByLabel->setTextFormat(Qt::TextFormat::RichText);
    ui->poweredByLabel->setText(R"(<p style="text-align: right;"><span style="color: rgb(255, 187, 150);font-size: 24px; font-family: Verdana;"><u><em><strong>Powered By -></strong></em></u></span></p>)");
    //table
    InitTableAttribute(ui->historyTableView);
    InitTableAttribute(ui->loveTableView);
    InitTableAttribute(ui->searchTableView);
    InitTableAttribute(ui->biliTableView);
    themeManager.SetAllPropertiesInTheme(ui->historyTableView,"table_list");
    themeManager.SetAllPropertiesInTheme(ui->loveTableView,"table_list");
    themeManager.SetAllPropertiesInTheme(ui->searchTableView,"table_list");
    themeManager.SetAllPropertiesInTheme(ui->biliTableView,"table_list");

    //connect signals and slots
    //mainStack
    ui->mainStack->setCurrentIndex(0);
    connect(ui->toHomePage,&QPushButton::clicked,this,[=](){
        if(ui->mainStack->currentIndex() == 0) return;
        ui->mainStack->setCurrentIndex(0);
    });
    connect(ui->toSearchPage,&QPushButton::clicked,this,[=](){
        if(ui->mainStack->currentIndex() == 1) return;
        ui->mainStack->setCurrentIndex(1);
    });
    connect(ui->toLovePage,&QPushButton::clicked,this,[=](){
        if(ui->mainStack->currentIndex() == 2) return;
        ui->mainStack->setCurrentIndex(2);
    });
    connect(ui->toHistoryPage,&QPushButton::clicked,this,[=](){
        if(ui->mainStack->currentIndex() == 3) return;
        ui->mainStack->setCurrentIndex(3);
    });
    connect(ui->toBiliSearchPage,&QPushButton::clicked,this,[=](){
        if(ui->mainStack->currentIndex() == 4) return;
        ui->mainStack->setCurrentIndex(4);
    });
    //liteStack
    connect(&mainObj,&ProcessObj::ShMsg,this,&Widget::SentMsg);
    connect(&mainObj,&ProcessObj::HdMsg,this,&Widget::HideMsg);
    //play control
    connect(ui->pauseOrPlay,&QPushButton::clicked,&mainObj,&ProcessObj::PlayOrPause);
    connect(ui->nextSong,&QPushButton::clicked,&mainObj,&ProcessObj::SwitchNext);
    connect(ui->prevSong,&QPushButton::clicked,&mainObj,&ProcessObj::SwitchPrev);
    connect(ui->changePlayMode,&QPushButton::clicked,&mainObj,&ProcessObj::SwitchPlayMode);
    connect(ui->loveButton,&QPushButton::clicked,&mainObj,&ProcessObj::ChangeLove);
    connect(mainObj.GetPlayer(),&QMediaPlayer::mediaStatusChanged,&mainObj,&ProcessObj::AutoSwitch);
    connect(ui->volumeSlider,&QSlider::valueChanged,this,[&](){
        mainObj.SetVolume(ui->volumeSlider->GetValue(), true);
        if(mainObj.currentVolume<0.01){
            ThemeManager::SetComponentProperties(ui->muteButton,"btn_state","muted");
        }
        else{
            ThemeManager::SetComponentProperties(ui->muteButton,"btn_state","unmuted");
        }
    });
    connect(ui->muteButton,&QPushButton::clicked,this,[&](){
        if(mainObj.currentVolume<0.01){
            mainObj.SetVolume(mainObj.savedVolume);
            ui->volumeSlider->SetValue(mainObj.Volume2Level(mainObj.savedVolume));
            ThemeManager::SetComponentProperties(ui->muteButton,"btn_state","unmuted");
        }
        else{
            mainObj.savedVolume = mainObj.currentVolume;
            mainObj.SetVolume(0.0);
            ui->volumeSlider->SetValue(0.0);
            ThemeManager::SetComponentProperties(ui->muteButton,"btn_state","muted");
        }
    });
    connect(mainObj.GetPlayer(),&QMediaPlayer::playingChanged,this,[&](){
        if(mainObj.GetPlayer()->isPlaying()){
            ThemeManager::SetComponentProperties(ui->pauseOrPlay,"btn_state","playing");
        }
        else{
            ThemeManager::SetComponentProperties(ui->pauseOrPlay,"btn_state","paused");
        }
    });
    connect(&mainObj,&ProcessObj::PlayModeChanged,this,[&](int mode){
        switch(mode){
        case 0:
            ThemeManager::SetComponentProperties(ui->changePlayMode,"btn_state","loop");
            break;
        case 1:
            ThemeManager::SetComponentProperties(ui->changePlayMode,"btn_state","single");
            break;
        case 2:
            ThemeManager::SetComponentProperties(ui->changePlayMode,"btn_state","random");
            break;
        case 3:
            ThemeManager::SetComponentProperties(ui->changePlayMode,"btn_state","list");
            break;
        }
    });
    connect(&mainObj,&ProcessObj::LoveStateUpdate,this,[&](bool isLove){
        if(isLove){
            ThemeManager::SetComponentProperties(ui->loveButton,"btn_state","loved");
        }
        else{
            ThemeManager::SetComponentProperties(ui->loveButton,"btn_state","unloved");
        }
    });
    connect(mainObj.GetPlayer(), &QMediaPlayer::positionChanged, this, [&](qint64 position) {
        QString currentTime = FormatTime(position);
        QString totalTime = FormatTime(mainObj.GetPlayer()->duration());
        ui->timeLabel->setText(currentTime + " / " + totalTime);
    });
    connect(mainObj.GetPlayer(), &QMediaPlayer::durationChanged, this, [&](qint64 duration) {
        QString totalTime = FormatTime(duration);
        QString currentTime = FormatTime(mainObj.GetPlayer()->position());
        ui->timeLabel->setText(currentTime + " / " + totalTime);
    });
    connect(mainObj.GetPlayer(),&QMediaPlayer::positionChanged,this,[&](qint64 pos){
        if(!ui->progressBar->isSliderDown()){
            double length = static_cast<double>(mainObj.GetPlayer()->duration());
            ui->progressBar->SetValue(static_cast<double>(pos) / length);
        }
    });
    connect(ui->progressBar,&QSlider::sliderReleased,this,[&](){
        qint64 pos = static_cast<qint64>
        (static_cast<double>(mainObj.GetPlayer()->duration())*ui->progressBar->GetPercentage());
        mainObj.GetPlayer()->setPosition(pos);
    });
    connect(ui->progressBar,&BetterSlider::m_sliderPressed,this,[&](){
        //click_pos = 
    });
    connect(ui->progressBar,&BetterSlider::m_sliderMoved,this,[&](int position){
        Q_UNUSED(position);

    });
    //table
    connect(ui->historyTableView,&QTableView::doubleClicked,this,[&](const QModelIndex &index){
        mainObj.OpenLL(index.row(),PlayListType::History);
    });
    connect(ui->loveTableView,&QTableView::doubleClicked,this,[&](const QModelIndex &index){
        mainObj.OpenLL(index.row(),PlayListType::Favorites);
    });
    connect(ui->searchTableView,&QTableView::doubleClicked,this,[&](const QModelIndex &index){
        mainObj.OpenLL(index.row(),PlayListType::Search);
    });
    connect(ui->biliTableView,&QTableView::doubleClicked,this,[&](const QModelIndex &index){
        QString rename = ui->biliDownloadRenameEdit->text();
        if(!rename.isEmpty()) mainObj.biliDownloadRename = rename;
        mainObj.OpenLL(index.row(),PlayListType::Bilibili);
        mainObj.biliDownloadRename.clear();
    });
    connect(&mainObj,&ProcessObj::FreshTable,this,&Widget::FreshTables);
    //cover
    connect(&mainObj,&ProcessObj::LoadImgFinised,this,[&](){
        QPixmap pixmap = mainObj.currentCover;
        QLabel* label = ui->coverLabel;
        label->setAlignment(Qt::AlignCenter);
        QPixmap circularCover = CreateCircularCover(pixmap, QSize(250, 250));
        label->setPixmap(circularCover);
        // QPixmap pixmap = mainObj.currentCover;
        // QLabel* label = ui->coverLabel;
        // label->setAlignment(Qt::AlignmentFlag::AlignCenter);
        // int targetWidth, targetHeight;
        // Widget::FitPicSize(label->width(), label->height(), pixmap.width(), pixmap.height(), targetWidth, targetHeight);
        // QSize size(targetWidth, targetHeight);
        // QPixmap scaledPixmap = pixmap.scaled(targetWidth, targetHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        // // Create a circular mask
        // QPixmap circularPixmap(size);
        // circularPixmap.fill(Qt::transparent);  // Fill the background with transparent color
        // QPainter painter(&circularPixmap);
        // painter.setRenderHint(QPainter::Antialiasing);
        // QPainterPath path;
        // int radiusReduction = 0; // 减少的半径值
        // QRectF ellipseRect(0, 0, size.width() - radiusReduction, size.height() -  radiusReduction);
        // path.addEllipse(ellipseRect);
        // painter.setClipPath(path);
        // painter.drawPixmap(0, 0, scaledPixmap);  // Draw the pixmap with circular clipping
        // label->setPixmap(circularPixmap);
    });
    //visualiser
    connect(&mainObj,&ProcessObj::SpectrumCalculated,this,[&](double* bandArr, double total){
        ui->visualiser->setData(bandArr, total);
        ui->mainPage_1->setData(bandArr, total);
    });
    connect(ui->mainStack,&QStackedWidget::currentChanged,this,[&](int index){
        ui->visualiser->WidgetChange(index);
        ui->mainPage_1->WidgetChange(index);
    });
    //lyricsArea
    connect(&mainObj,&ProcessObj::LoadLyricFinished,this,[&](){
        LyricsArea->SetLyrics(mainObj.lyric,mainObj.lyricTime);
    });
    //other controls
    connect(ui->openFromLocal,&QPushButton::clicked,this,[&](){
        QString path = QFileDialog::getOpenFileName(this,"Select a music~",QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),"*.mp3 *.wav *.m4a *.flac");
        if(path.isEmpty()) return;
        mainObj.OpenLL(path, PlayListType::Local);
    });
    connect(ui->searchButton,&QPushButton::clicked,this,[&](){
        mainObj.Search(ui->searchTextEdit->text());
    });
    connect(ui->biliSearchButton,&QPushButton::clicked,this,[&](){
        QString bid = ui->biliSearchTextEdit->text();
        if(bid.isEmpty()) return;
        mainObj.SearchBili(bid);
    });
    connect(ui->switchBg,&QPushButton::clicked,this,[&](){
        QString path = QFileDialog::getOpenFileName(this,"Select a background~",QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),"*.jpg *.png");
        if(!path.isEmpty()){
            bgPath = path;
            mainObj.backgroundPath = path;
        }
        else if(QFile::exists(bgPath)) return;
        else{
            bgPath = ":/bg/source/bgf{index}.jpg";
            bgPath.replace("{index}",QString::number(QRandomGenerator::global()->generate()%7));
        }
        SetBackground(bgPath);
        update();
    });
    connect(ui->bgAlphaSlider,&QSlider::valueChanged,this,[&](){
        mainObj.currentBlur = ui->bgAlphaSlider->value();
        bgRedraw = true;    
        update();
    });
    connect(ui->exitBtn,&QPushButton::clicked,this,[&](){
        mainObj.SaveSettings();
        mainObj.SaveSongList();
        QApplication::quit();
    });
    connect(ui->mainTitleWidget, &titleWidget::doubleClicked,this,[&](QMouseEvent* event){
        if (event->button() == Qt::LeftButton) {
            if(isFullScreen()) return;
            if (isMaximized()) {
                showNormal();
                SetFrameless(true);
            }
            else {
                SetFrameless(false);
                showMaximized();
            }
        }
        else if(event->button() == Qt::RightButton) {
            showMinimized();
        }
        event->accept();
    });
    connect(ui->mainTitleWidget, &titleWidget::pressed, this, [&](QMouseEvent* event) {
        if (event->button() == Qt::LeftButton) {
            if(QWindow *window = windowHandle()) {
                if(!isMaximized() && !isFullScreen()) {
                    window->startSystemMove();
                    event->accept();
                }
            }
        }
    });
    connect(ui->mainTitleWidget, &titleWidget::moved, this, [&](QMouseEvent* event) {
        Q_UNUSED(event);
        // if (event->buttons() & Qt::LeftButton) {
        //     move((event->globalPosition() - dragPosition).toPoint());
        //     event->accept();
        // }
    });
    //shortcuts
    //ctrl+enter switch to full screen
    QShortcut* fullScreenShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return), this);
    connect(fullScreenShortcut, &QShortcut::activated, this, [&]() {
        if (isFullScreen()) {
            showNormal();
            SetFrameless(true); 
        } else {
            SetFrameless(false);
            showFullScreen();
        }
    });

    //final setup
    //const int shadowWidth = 10;
    // const MARGINS shadowMargins = {shadowWidth, shadowWidth, shadowWidth, shadowWidth};
    // ::DwmExtendFrameIntoClientArea(reinterpret_cast<HWND>(winId()), &shadowMargins);
    SetFrameless(true);
    FreshTables(PlayListType::Favorites);
    FreshTables(PlayListType::History);
    FreshTables(PlayListType::Search);
    FreshTables(PlayListType::Bilibili);
    ui->timeLabel->setText("00:00 / 00:00");
    if(mainObj.iniF) mainObj.SentMsg(0,"哇新玩家，欢迎欢迎!");
    else if(mainObj.iniN) mainObj.SentMsg(0,"欢迎你的到来!");//.......
    emit mainObj.PlayModeChanged(mainObj.currentPlayMode);
    mainObj.CheckLoveState();
}

void Widget::SentMsg(int type, QString msg){
    QLabel* iconLabel = ui->infoImg;
    int m_wp = 56,m_h = 56;
    if(type==0){
        iconLabel->setPixmap(QPixmap(":/icon/source/check-circle.png").scaled(m_wp,m_h,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    }
    else if(type==1){
        iconLabel->setPixmap(QPixmap(":/icon/source/info-circle.png").scaled(m_wp,m_h,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    }
    else{
        iconLabel->setPixmap(QPixmap(":/icon/source/exclamation-circle.png").scaled(m_wp,m_h,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    }
    ui->infoText->setPlainText(msg);
    ui->liteStack->setCurrentIndex(1);
    qDebug()<<"to 1";
}
void Widget::HideMsg(){
    ui->pBImg->setPixmap(QPixmap([&](){return supportImg[QRandomGenerator::global()->generate()%supportImg.size()];}()));
    ui->liteStack->setCurrentIndex(0);
    qDebug() << "to 0";
}

void Widget::FreshTables(PlayListType type, QString listName){
    QWidget* ptr = this;
    QTableView* tableView;
    if(type==PlayListType::Favorites){
        tableView = ui->loveTableView;
    }
    else if(type==PlayListType::History){
        tableView = ui->historyTableView;
    }
    else if(type==PlayListType::Search){
        tableView = ui->searchTableView;
    }
    else if(type==PlayListType::Bilibili){
        return FreshBiliTable();
    }
    else return;
    auto vec = mainObj.GetPlayList(type, listName)->songVector;
    QStandardItemModel* model = new QStandardItemModel(static_cast<int>(vec.size()), 4, ptr);
    QStringList headers = {"标题", "歌手", "专辑", "时长"};
    model->setHorizontalHeaderLabels(headers);
    for(int i=0;i<vec.size();i++){
        SongPtr song = vec[i];
        QStandardItem* item = new QStandardItem(song->title);
        model->setItem(i, 0, item);
        item = new QStandardItem(song->artist);
        model->setItem(i, 1, item);
        item = new QStandardItem(song->album);
        model->setItem(i, 2, item);
        QString duration = QTime(0,0).addMSecs(song->duration).toString();
        item = new QStandardItem(duration);
        model->setItem(i, 3, item);
    }
    tableView->setModel(model);
}

void Widget::FreshBiliTable(){
    QWidget* ptr = this;
    QTableView* tableView = ui->biliTableView;
    QStringList headers = {"标题", "cid", "序号"};
    QStandardItemModel* model = new QStandardItemModel(static_cast<int>(mainObj.biliInfo.parts.size()), 3, ptr);
    model->setHorizontalHeaderLabels(headers);
    for(int i=0;i<mainObj.biliInfo.parts.size();i++){
        const auto& item = mainObj.biliInfo.parts[i];
        model->setItem(i, 0, new QStandardItem(item.part));
        model->setItem(i, 1, new QStandardItem(QString::number(item.cid)));
        model->setItem(i, 2, new QStandardItem(QString::number(item.page)));
    }
    tableView->setModel(model);
    ui->biliInfoArea->SetInfo(mainObj.biliInfo);
    // QPixmap pic(mainObj.biliInfo.picPath);
    // ui->biliPicLabel->setPixmap(pic);
    // ui->biliPicLabel->setAlignment(Qt::AlignCenter);
    // ui->biliPicLabel->setScaledContents(false);
    
    // QString title = QString("<b style='color:#00a1d6; font-size:14pt;'>标题:</b> %1")
    //                 .arg(mainObj.biliInfo.title.toHtmlEscaped());
    // QString desc = QString("<b style='color:#6d757a; font-size:10pt;'>简介:</b><br>"
    //                     "<div style='color:#222; font-size:9pt; margin-left:10px;'>%1</div>")
    //                 .arg(mainObj.biliInfo.desc.toHtmlEscaped().replace("\n", "<br>"));
    // QString videos = QString("<b style='color:#fb7299;'>分p数:</b> <span style='font-weight:bold; color:#f25d8e;'>%1</span>")
    //                 .arg(mainObj.biliInfo.videos);
    // ui->biliInfoLabel->setTextFormat(Qt::RichText);
    // ui->biliInfoLabel->setText(QString("<div style='line-height:1.4;'>%1<br>%2<br>%3</div>")
    //                         .arg(title, desc, videos));

}

void Widget::resizeEvent(QResizeEvent *event) {
    UpdateBackgroundSize();
    //UpdateWindowRegion(true);
    QWidget::resizeEvent(event);
}
void Widget::changeEvent(QEvent *event) {
    if (event->type() == QEvent::WindowStateChange) {
        if (isMinimized()) {
            ui->titleTextLabel->setText(GenerateTitle());
            ui->pBImg->setPixmap(
                QPixmap([&]()->QString {return supportImg[QRandomGenerator::global()->generate()%supportImg.size()];}()));
            ui->visualiser->WidgetChange(-1);
            ui->mainPage_1->WidgetChange(-1);
        }
        else{
            ui->visualiser->WidgetChange(ui->mainStack->currentIndex());
            ui->mainPage_1->WidgetChange(ui->mainStack->currentIndex());
            qDebug() << "Window state changed:";
            qDebug() << "isMaximized:" << isMaximized();
            qDebug() << "windowState:" << windowState();
            qDebug() << "geometry:" << geometry();
            qDebug() << "availableGeometry:" << QGuiApplication::primaryScreen()->availableGeometry();
        }
    }
    QWidget::changeEvent(event);
}

Widget::~Widget(){
    mainObj.SaveSettings();
    mainObj.SaveSongList();
    delete ui;
}

void Widget::InitTableAttribute(QTableView *table){
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    QScroller::grabGesture(table->viewport(), QScroller::LeftMouseButtonGesture);
    QScroller* scroller = QScroller::scroller(table->viewport());
    QScrollerProperties properties = scroller->scrollerProperties();
    properties.setScrollMetric(QScrollerProperties::DecelerationFactor, 0.1);
    properties.setScrollMetric(QScrollerProperties::MaximumVelocity, 0.9);
    properties.setScrollMetric(QScrollerProperties::OvershootDragResistanceFactor, 0.85);
    properties.setScrollMetric(QScrollerProperties::SnapPositionRatio, 0.5);
    scroller->setScrollerProperties(properties);
    table->viewport()->setAttribute(Qt::WA_OpaquePaintEvent, false);
    table->setShowGrid(false);
    table->verticalHeader()->setVisible(false);
    QHeaderView *header = table->horizontalHeader();
    header->setResizeContentsPrecision(3);
    header->setSectionResizeMode(QHeaderView::Stretch);
}
void Widget::SetLabelStyle(QLabel* label,const QString& path){
        label->setPixmap(QPixmap(path));
        label->setScaledContents(true);
}
QString Widget::FormatTime(qint64 ms){
    // int seconds = (ms / 1000) % 60;
    // int minutes = (ms / (1000 * 60)) % 60;
    // return QString("%1:%2").arg(minutes, 2, 10, QChar('0'))
    //     .arg(seconds, 2, 10, QChar('0'));
    int ims = static_cast<int>(ms);
    int seconds = (ims / 1000) % 60;
    int minutes = (ims / (1000 * 60)) % 60;
    int hours = (ims / (1000 * 60 * 60)) % 24;
    if(hours>0){
        return QString("%1:%2:%3").arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    }
    else{
        return QString("%1:%2").arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    }
    return QString();
}
QString Widget::GenerateTitle(){
    QString body =  titleList[QRandomGenerator::global()->generate()%titleList.size()];
    QString format("<p><span style='color: rgb(204, 255, 153); font-size: 23px;'><sup><sub><strong>LABS</strong></sub></sup></span>"
                   "<span style='font-size: 14px;color: rgb(64, 64, 64)'><sub>For</sub></span>"
                   "<span style='color: rgb(255, 204, 255); font-size: 26px;"
                   "'><strong>{BODY}:)</strong></span></p>");
    format.replace("{BODY}",body);
    return format;
}
void Widget::UpdateBackgroundSize() {
    // float w = static_cast<float>(width()) / static_cast<float>(picWidth);
    // float h = static_cast<float>(height()) / static_cast<float>(picHeight);
    // float maxi = qMax(w,h);
    // currentHei = static_cast<int>(static_cast<float>(picHeight) * maxi);
    // currentWid = static_cast<int>(static_cast<float>(picWidth) * maxi);
    FitPicSize(width(), height(), picWidth, picHeight, currentWid, currentHei);
    bgRedraw = true;
}

void Widget::FitPicSize(int cw, int ch, int pw, int ph, int &tw, int &th){
    float w = static_cast<float>(cw) / static_cast<float>(pw);
    float h = static_cast<float>(ch) / static_cast<float>(ph);
    float maxi = qMax(w,h);
    tw = static_cast<int>(static_cast<float>(pw) * maxi);
    th = static_cast<int>(static_cast<float>(ph) * maxi);
}
void Widget::SetBackground(QString path){
    //backgroundLabel = new QLabel(this);
    //check if file mainObj.background is exist
    if(!path.isEmpty()) bgPath = path;
    else if(QFile::exists(mainObj.backgroundPath)) bgPath = mainObj.backgroundPath;
    else{
        bgPath = ":/bg/source/bgf{index}.jpg";
        bgPath.replace("{index}",QString::number(3));
    }
    bgPixmap = new QPixmap(bgPath);
    originalWid = bgPixmap->width();
    originalHei = bgPixmap->height();
    picWidth = originalWid;
    picHeight = originalHei;
    UpdateBackgroundSize();
    update();
}

void Widget::FitBackground(int cw, int ch, int pw, int ph, int &tw, int &th){
    float pich = static_cast<float>(ph), picw = static_cast<float>(pw);
    float sh = pich / static_cast<float>(ch), sw = picw / static_cast<float>(cw);
    if(sh <= sw){
        th = ch;
        tw = static_cast<int>(picw / sh);
    }
    else{
        tw = cw;
        th = static_cast<int>(pich / sw);
    }
}


Widget::Edge Widget::AtEdge(const QPoint& pos){
    if(pos.y() <= edgeWidth){
        if(pos.x() <= edgeWidth) return Edge::LeftTop;
        if(pos.x() >= width() - edgeWidth) return Edge::RightTop;
        return Edge::Top;
    }
    return Edge::None;
}


bool Widget::nativeEvent(const QByteArray &eventType, void *message, qintptr *result) {
    MSG* param = static_cast<MSG*>(message);
    // Q_UNUSED(eventType);

    switch(param->message){
        case WM_NCHITTEST: {
            if(isFullScreen() || isMaximized()){
                *result = HTCLIENT;
                return false;
            }
            RECT rect;
            GetWindowRect(reinterpret_cast<HWND>(this->winId()), &rect);
            LONG x = GET_X_LPARAM(param->lParam);
            LONG y = GET_Y_LPARAM(param->lParam);
            int nx = x - rect.left;
            int ny = y - rect.top;
            int w = rect.right - rect.left;
            int h = rect.bottom - rect.top;
            *result = HTCLIENT;
            if(ny>=0 && ny<=edgeWidth){
                if(nx>=0 && nx<=edgeWidth) *result = HTTOPLEFT;
                else if(nx>=w-edgeWidth && nx<=w) *result = HTTOPRIGHT;
                else *result = HTTOP;
            }
            else if(ny>=h-edgeWidth && ny<=h){
                if(nx>=0 && nx<=edgeWidth) *result = HTBOTTOMLEFT;
                else if(nx>=w-edgeWidth && nx<=w) *result = HTBOTTOMRIGHT;
                else *result = HTBOTTOM;
            }
            else if(nx>=0 && nx<=edgeWidth) *result = HTLEFT;
            else if(nx>=w-edgeWidth && nx<=w) *result = HTRIGHT;

            if(*result == HTCLIENT) return false;
            return true;
        }
    }

    return QWidget::nativeEvent(eventType, message, result);
}

void Widget::SetFrameless(bool frameless){
    #ifdef Q_OS_WIN
    if (HWND hwnd = reinterpret_cast<HWND>(winId())) {
        long int style = (::GetWindowLong(hwnd, GWL_STYLE));
        if(frameless) ::SetWindowLong(hwnd, GWL_STYLE, style | WS_THICKFRAME);
        else ::SetWindowLong(hwnd, GWL_STYLE, style ^ WS_THICKFRAME);
    }
    #endif
}

void Widget::SetWStyles(long int styles){
    #ifdef Q_OS_WIN
    if (HWND hwnd = reinterpret_cast<HWND>(winId())) {
        long int style = (::GetWindowLong(hwnd, GWL_STYLE));
        ::SetWindowLong(hwnd, GWL_STYLE, style | styles);
    }
    #endif
}

void Widget::PaintBackground(QPaintEvent* event, bool effectOn){
    Q_UNUSED(event);
    if(bgPixmap == nullptr) return;
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    QRectF targetRect(0, 0, currentWid, currentHei);
    QRectF sourceRect(0, 0, originalWid, originalHei);
    auto ptr = bgPixmap;
    if(effectOn && mainObj.currentBlur > 0) ptr = RenderToBuffer();
    painter.drawPixmap(targetRect, *ptr, sourceRect);
    
}

// void Widget::ApplyEffect(QPixmap* pixmap){
//     if (!scene) EffectSetup();
//     if (!pixmapItem) {
//         pixmapItem = scene->addPixmap(*pixmap);
//         pixmapItem->setGraphicsEffect(blurEffect);
//     } else {
//         pixmapItem->setPixmap(*pixmap);
//     }
// }

// void Widget::EffectSetup(){
//     if (blurEffect && scene) return;
//     if (scene) { delete scene; scene = nullptr; }
//     if (blurEffect) { delete blurEffect; blurEffect = nullptr; }
//     blurEffect = new QGraphicsBlurEffect(this);
//     scene = new QGraphicsScene(this);
//     if (bufferedPixmap) { delete bufferedPixmap; bufferedPixmap = nullptr; }
// }

QPixmap* Widget::RenderToBuffer(bool check){
    if (check && !bgRedraw) {
        return bufferedPixmap ? bufferedPixmap : bgPixmap;
    }
    bgRedraw = false;
    if (!bgPixmap) return nullptr;
    if (mainObj.currentBlur == 0) return bgPixmap;
    //if (!scene || !blurEffect) EffectSetup();
    const qreal radius = static_cast<qreal>(mainObj.currentBlur);
    const qreal scale = 0.25;
    QSize smallSize = bgPixmap->size() * scale;
    if (smallSize.width() < 1) smallSize.setWidth(1);
    if (smallSize.height() < 1) smallSize.setHeight(1);
    QImage smallImg = bgPixmap->toImage().scaled(smallSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QPixmap smallPix = QPixmap::fromImage(smallImg);
    QGraphicsScene tmpScene;
    QGraphicsPixmapItem item(smallPix);
    QGraphicsBlurEffect tmpEffect;
    tmpEffect.setBlurRadius(qMax<qreal>(0.5, radius * scale * 0.9));
    item.setGraphicsEffect(&tmpEffect);
    tmpScene.addItem(&item);
    QImage result(smallSize, QImage::Format_ARGB32_Premultiplied);
    result.fill(Qt::transparent);
    QPainter painter(&result);
    tmpScene.render(&painter, QRectF(0,0, result.width(), result.height()),
                    QRectF(0,0, smallPix.width(), smallPix.height()));
    painter.end();
    QPixmap finalPixmap = QPixmap::fromImage(result.scaled(bgPixmap->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    if (!bufferedPixmap) bufferedPixmap = new QPixmap(finalPixmap.size());
    *bufferedPixmap = finalPixmap;
    return bufferedPixmap;
}

void Widget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    PaintBackground(event, true);
    QWidget::paintEvent(event);
}

QPixmap Widget::CreateCircularCover(const QPixmap &pixmap, const QSize &targetSize) {
    if (pixmap.isNull() || targetSize.isEmpty())
        return QPixmap();
    QPixmap scaledPixmap = pixmap.scaled(targetSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

    QPixmap circularPixmap(targetSize);
    circularPixmap.fill(Qt::transparent);
    QPainter painter(&circularPixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPainterPath path;
    path.addEllipse(QRectF(0, 0, targetSize.width(), targetSize.height()));
    painter.setClipPath(path);

    QPoint offset((targetSize.width() - scaledPixmap.width()) / 2,
                  (targetSize.height() - scaledPixmap.height()) / 2);
    painter.drawPixmap(offset, scaledPixmap);
    return circularPixmap;
}



