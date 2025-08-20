#include "processobj.h"
#include <QStandardPaths>
#include <QJsonArray>
#include <QJsonObject>
#include <QEvent>
#include <QEventLoop>
#include <QCryptographicHash>
#include <QDir>
#include <QRandomGenerator>
#include "include/taglib/tag.h"
#include "include/taglib/mpegfile.h"
#include "include/taglib/fileref.h"
#include "include/taglib/id3v2tag.h"
#include "include/taglib/id3v2frame.h"
#include "include/taglib/attachedpictureframe.h"
#include <fstream>
#include <filesystem>
#include <QDateTime>
#include <QTimer>
#include <QRegularExpression>


const QString ProcessObj::_dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)+"/LABS";
const QString ProcessObj::cachePath = ProcessObj::_dataPath + "/cache";
const QString ProcessObj::songManageFilePath = ProcessObj::_dataPath + "/smFile.json";
const QString ProcessObj::settingsFilePath = ProcessObj::_dataPath + "/settings.json";

ProcessObj::ProcessObj(QObject *parent)
    : QObject{parent}
    ,_magnitudeArr{new double[_Npoint/2]}
    ,_bandArr{new double[64]}
    ,_player{new QMediaPlayer(this)}
    ,_loader{new QMediaPlayer(this)}
    ,_device{QMediaDevices::defaultAudioOutput()}
    ,_audio{new QAudioOutput(this)}
    ,_networkObj{new NetworkObj(this)}
    ,_bufferReceriver{new QAudioBufferOutput(this)}
    ,_songManager{new SongManager(this)}
    ,msgTimer{new QTimer(this)}
{
    _justSetup = true;//!play while setup
    _audio->setDevice(_device);
    _player->setAudioOutput(_audio);
    _player->setAudioBufferOutput(_bufferReceriver);

    //connect//
    //visualiser & buffer
    connect(_bufferReceriver, &QAudioBufferOutput::audioBufferReceived, this, [&](const QAudioBuffer &buffer){
        //find out what buffer contains
        //and how it varies with time
        //then we can use it to do some fft
        //and get the frequency
        //and do some visual effect
        //sounds cool huh?
        if(buffer.format().sampleFormat()!=QAudioFormat::SampleFormat::Float) return;
        const float* data = buffer.constData<float>();
        DoFFTSpectrum(data,static_cast<int>(buffer.sampleCount()));
    });
    //sync loader
    connect(_loader,&QMediaPlayer::mediaStatusChanged, this,[&](){
        qDebug()<<_loader->mediaStatus();
        qDebug()<<currentSongPath;
        if(_loader->mediaStatus()==QMediaPlayer::LoadedMedia) LoadMetaData(_loader->mediaStatus());
    });
    connect(this,&ProcessObj::MetaDataLoaded,this,&ProcessObj::LoadVisualData);
    //msg system
    //TODO: new msg system
    connect(msgTimer,&QTimer::timeout,this,[&](){
        msgque.pop();
        if(!msgque.empty()){
            msgTimer->start(3000);
            emit ShMsg(msgque.front().first,msgque.front().second);
        }
        else emit HdMsg();
    });
    connect(_songManager,&SongManager::ListModified,this,[&](PlayListType type, const QString &listName){
        emit FreshTable(type, listName);
    });

    //init//
    currentListType = PlayListType::History;
    if(!QFile::exists(settingsFilePath) || !QFile::exists(songManageFilePath)){
        Initialize();
        iniF = true;
    }
    else iniF = false;//for msg
    iniN = true;//for msg
    //load songlist if there is or create new
    LoadSongList();
    //load settings if there is or use default
    LoadSettings();//apply settings inside
    _audio->setVolume(currentVolume);
    savedVolume = currentVolume;
    if(currentIndex!=-1){
        OpenLL(currentIndex, currentListType, currentListName);
    }
    else{
        _justSetup = false;//no media will be loaded
    }

}

void ProcessObj::LoadSettings(){
    bool mark=true;
    if(!QFile::exists(settingsFilePath)) mark=false;
    else{
        QFile file(settingsFilePath);
        if(!file.open(QIODevice::ReadOnly)){
            qDebug() << "Failed to open settings file";
            SentMsg(2,"OOPS,看起来配置加载失败了:(");
            iniN = false;
            mark=false;
        }
        settings = QJsonDocument::fromJson(file.readAll());
        file.close();
    }
    _ApplySettingsFromLoad(mark);
}
void ProcessObj::LoadSongList(){
    if(!_songManager->LoadFromDisk(songManageFilePath)){
        qDebug() << "Failed to open songlist file";
        SentMsg(2,"OOPS,看起来歌单加载失败了:(");
        iniN = false;
        return;
    }
}
void ProcessObj::SaveSettings(){
    SettingToJson();
    QFile file(settingsFilePath);
    if(!file.open(QIODevice::WriteOnly)){
        qDebug() << "Failed to open settings file";
        return;
    }
    file.write(settings.toJson());
    file.close();
}
void ProcessObj::SaveSongList(){
    _songManager->SaveToDisk(songManageFilePath);
}

void ProcessObj::ListToJson(){
    // QJsonDocument newList;
    // QJsonObject obj;
    // for(auto it = songListMap.begin();it != songListMap.end();it++){
    //     QString lstName = it.key();
    //     if(lstName=="Search") continue;
    //     QJsonArray arr;
    //     for(auto& i:it.value()->lst){
    //         QJsonArray inner;
    //         for(auto& j:i){
    //             inner.append(j);
    //         }
    //         arr.append(inner);
    //     }
    //     obj.insert(lstName,arr);
    // }
    // newList.setObject(obj);
    // songList = newList;
}
void ProcessObj::SettingToJson(){
    QJsonObject obj;
    obj.insert("volume", currentVolume);
    obj.insert("playMode", currentPlayMode);
    obj.insert("index", currentIndex); 
    obj.insert("listName", currentListName);
    obj.insert("songPath", currentSongPath);
    obj.insert("bgIndex", currentBgIndex);
    obj.insert("blur", currentBlur);
    obj.insert("bgPath", backgroundPath);
    obj.insert("listType", static_cast<int>(currentListType));
    obj.insert("songHash", currentSongHash);
    settings = QJsonDocument(obj);
}

QString ProcessObj::SaveSong(int index){
    QString pth;

    //download
    SongPtr info = _songManager->searchCache->songVector[index];
    if(info.isNull()){
        SentMsg(2,"歌曲不存在喵");
        return pth;
    }
    qint64 time = QDateTime::currentMSecsSinceEpoch();
    QString tmp = QString::number(time);
    _networkObj->DownloadSong(QUrl(info->url),_dataPath,tmp);
    _networkObj->DownloadImg(QUrl(info->imgUrl),cachePath,tmp);
    _networkObj->DownloadLrc(QUrl(info->lrcUrl),_dataPath,tmp);

    //tag init
    QString songp = QString(_dataPath + "/" + tmp + ".mp3").toUtf8();
    TagLib::MPEG::File file(TagLib::FileName(songp.toStdString().c_str()));
    if(!file.isOpen()){
        qDebug()<<"not open song";
        SentMsg(2,"ERROR IOinSaveSong!");
        return pth;
    }
    if(!file.hasID3v2Tag()){
        file.ID3v2Tag(true);
    }
    TagLib::ID3v2::Tag* tag = file.ID3v2Tag();

    //add tags
    std::ifstream imgfile((cachePath + "/" + tmp + ".jpg").toStdString(),std::ios::binary);
    if(!imgfile.is_open()){
        qDebug() << "Can't open the img file";
        SentMsg(2,"OOPS,看起来没能成功下载封面喵");
    }
    else{
        std::vector<char> imgdata((std::istreambuf_iterator<char>(imgfile)),std::istreambuf_iterator<char>());
        TagLib::ID3v2::AttachedPictureFrame *frame = new TagLib::ID3v2::AttachedPictureFrame();
        frame->setMimeType("image/jpeg");
        frame->setType(TagLib::ID3v2::AttachedPictureFrame::FrontCover);
        frame->setPicture(TagLib::ByteVector(imgdata.data(),static_cast<unsigned int>(imgdata.size())));
        tag->addFrame(frame);
    }
    tag->setArtist(TagLib::String(info->artist.toUtf8().constData(),TagLib::String::UTF8));
    tag->setAlbum(TagLib::String(info->album.toUtf8().constData(),TagLib::String::UTF8));
    tag->setTitle(TagLib::String(info->title.toUtf8().constData(),TagLib::String::UTF8));

    //save file
    file.save();
    SentMsg(0,"下载成功了喵!");
    return tmp;
}

QString ProcessObj::SaveBiliSong(int index){
    QString pth;
    QString msg;
    if(index<0 || index>=biliInfo.parts.size()){
        SentMsg(2,"视频不存在喵");
        return pth;
    }

    //download
    qint64 cid = biliInfo.parts[index].cid;
    QString bvid = biliInfo.bvid;
    qint64 time = QDateTime::currentMSecsSinceEpoch();
    QString tmp = QString::number(time);
    SentMsg(1,"骑手正在下载B站视频...");
    _networkObj->DownloadM4a(bvid, QString::number(cid), _dataPath, tmp, msg, true);
    if(!msg.isEmpty()){
        SentMsg(2,msg);
        return pth;
    }
    _networkObj->DownloadImg(QUrl(biliInfo.parts[index].first_frame), cachePath, tmp);

    //tag init
    QString songp = QString(_dataPath + QDir::separator() + tmp + ".mp3").toUtf8();
    TagLib::MPEG::File file(TagLib::FileName(songp.toStdString().c_str()));
    if(!file.isOpen()){
        qDebug()<<"not open song";
        SentMsg(2,"ERROR IOinSaveSong!");
        return pth;
    }
    if(!file.hasID3v2Tag()){
        file.ID3v2Tag(true);
    }
    TagLib::ID3v2::Tag* tag = file.ID3v2Tag();

    //add tags
    std::ifstream imgfile((cachePath + QDir::separator() + tmp + ".jpg").toStdString(),std::ios::binary);
    if(!imgfile.is_open()){
        qDebug() << "Can't open the img file";
        SentMsg(2,"OOPS,看起来没能成功下载封面喵");
    }
    else{
        std::vector<char> imgdata((std::istreambuf_iterator<char>(imgfile)),std::istreambuf_iterator<char>());
        TagLib::ID3v2::AttachedPictureFrame *frame = new TagLib::ID3v2::AttachedPictureFrame();
        frame->setMimeType("image/jpeg");
        frame->setType(TagLib::ID3v2::AttachedPictureFrame::FrontCover);
        frame->setPicture(TagLib::ByteVector(imgdata.data(),static_cast<unsigned int>(imgdata.size())));
        tag->addFrame(frame);
    }
    tag->setArtist(TagLib::String("Vup",TagLib::String::UTF8));
    tag->setAlbum(TagLib::String("Unknown",TagLib::String::UTF8));
    tag->setTitle(TagLib::String(biliInfo.parts[index].part.toUtf8().constData(),TagLib::String::UTF8));

    //save file
    file.save();
    SentMsg(0,"下载成功了喵!");
    return tmp;
}

QString ProcessObj::Rename(QString Epoch, QString target){
    if(Epoch.isEmpty()) return QString();
    QString filename = target.isEmpty() ? Epoch : target;
    QString suffix;
    while(QFileInfo(filename+suffix+".mp3").isFile() || QFileInfo(filename+suffix+".lrc").isFile()){
        suffix += _sfc;
    }
    filename.replace(QRegularExpression("[/\\\\?%*:|\"<>]"),"_"); // replace invalid characters
    QDir dir(_dataPath);
    QString oldLrc = dir.filePath(Epoch + ".lrc"),
        oldSong = dir.filePath(Epoch + ".mp3"),
        newLrc = dir.filePath(filename + suffix + ".lrc"),
        newSong = dir.filePath(filename + suffix + ".mp3");
    if(newSong.size()>=255 || newLrc.size()>=255){
        SentMsg(2,"文件名过长喵, 请尝试更短的名字");
        return QString();
    }
    if(QFile::exists(oldLrc)){
        QFile::rename(oldLrc,newLrc);
    }
    if(QFile::exists(oldSong)){
        QFile::rename(oldSong,newSong);
    }
    return newSong;
}

void ProcessObj::PreLoad(QString hash){
    Q_UNUSED(hash);
    // if(currentListName=="Search"){
    //     if(hash.isEmpty()){
    //         qDebug()<<"unable to download";
    //         SentMsg(2,"欸? 这首歌要钱喵, 我没有马内喵;w;");
    //         return;
    //     }
    //     else{
    //         QString ep = SaveSong();
    //         QString npath = Rename(ep);
    //         currentListName = "History";
    //         OpLoc(npath);
    //     }
    // }
    // else if(currentListName=="Bili"){
    //     QString ep = SaveBiliSong();
    //     if(ep.isEmpty()){
    //         qDebug()<<"unable to download";
    //         SentMsg(2,"QAQ下载失败了");
    //         return;
    //     }
    //     QString npath = Rename(ep, biliInfo.parts[currentIndex].part);
    //     currentListName = "History";
    //     OpLoc(npath);
    // }
    // else if(!QFileInfo(path).isFile()){
    //     qDebug()<<"file not exist!";
    //     SentMsg(2,"本地文件丢失! 那我帮你移出列表啦~");
    //     songListMap[currentListName]->RemoveSong(currentIndex);
    //     return;
    // }
    // else if(QUrl::fromLocalFile(currentSongPath)==_loader->source()){
    //     emit FinLoad1();
    // }
    // else _loader->setSource(QUrl::fromLocalFile(currentSongPath));
}
void ProcessObj::AfterLoad1(QMediaPlayer::MediaStatus status){
    Q_UNUSED(status);
    // if(status == QMediaPlayer::LoadedMedia ){
    //     metaData = _loader->metaData();
    //     qDebug()<<metaData[QMediaMetaData::Title]<<metaData[QMediaMetaData::ContributingArtist]<<metaData[QMediaMetaData::AlbumTitle]<<metaData[QMediaMetaData::Duration];
    //     QFile file(currentSongPath);
    //     if(!file.open(QIODevice::ReadOnly)){
    //         qDebug() << "Can't open the file";
    //         SentMsg(2,"ERROR AL1!");
    //         currentSongHash = "";
    //         return;
    //     }
    //     QByteArray data = file.readAll();
    //     file.close();
    //     QByteArray hash = QCryptographicHash::hash(data,QCryptographicHash::Sha256);
    //     currentSongHash = hash.toHex();
    //     emit FinLoad1();
    // }
    // else if (status == QMediaPlayer::InvalidMedia || status == QMediaPlayer::NoMedia){
    //     SentMsg(2,"ERROR AL1!");
    //     qDebug()<<"fail in loading media phase1";
    // }
}
void ProcessObj::AfterLoad2(bool successful){
    Q_UNUSED(successful);
    // Q_UNUSED(successful);
    // if(!_fromList){
    //     currentListName = "History";
    //     if(songListMap["History"]->Contain(currentSongHash)!=-1){
    //         songListMap["History"]->UpdatePath(songListMap["History"]->Contain(currentSongHash),currentSongPath);
    //     }
    //     else{
    //         QList<QString> info = GetInfo(metaData);
    //         songListMap["History"]->AppendSong(info);
    //         SentMsg(1,"听过的歌又+1了欸-w-");
    //         qDebug()<<info;
    //     }
    //     currentIndex = 0;
    //     emit FreshTable(currentListName);
    // }
    // else emit FreshTable(currentListName);
    // QVariant coverArt1 = metaData[QMediaMetaData::ThumbnailImage];
    // QVariant coverArt2 = metaData[QMediaMetaData::CoverArtImage];
    // if (coverArt1.isValid()) {
    //     currentCover = coverArt1.value<QPixmap>();
    // }
    // else if (coverArt2.isValid()) {
    //     currentCover = coverArt2.value<QPixmap>();
    // }
    // if(currentCover.isNull()) {
    //     SentMsg(1,"OOPS, 加载封面失败了, 那就看看猫猫吧~");
    //     currentCover = QPixmap(":/icon/source/cat3.png");
    // }
    // _player->setSource(currentSongPath);
    // if(!_justSetup) _player->play();
    // emit LoadImgFinised();
    // LoadLyric();
    // emit LoadLyricFinished();
    // _justSetup = false;
    // CheckLoveState();
}
void ProcessObj::LoadLyric(){
    ClearLyric();
    QString path = currentSongPath;
    QFileInfo inf(path);
    QString suf = inf.suffix().toLower();
    QStringList formats = {"mp3","flac","wav","m4a"};
    QString lrcPath = path;
    bool mark = true;
    if(formats.contains(suf)){
        lrcPath.replace(suf,"lrc");
    }
    else{
        qDebug()<<"unsupported format";
        mark = false;
    }
    if(!QFile::exists(lrcPath)){
        qDebug()<<"no lyric file in same dir";
        mark = false;
    }
    QFile file(lrcPath);
    QSet<int>timeSet;
    if(!file.open(QIODevice::ReadOnly)){
        qDebug() << "Can't open the lrc file";
        mark = false;
    }
    if(!mark) SentMsg(1,"找不到歌词喵-w- 音乐是用来听的嘛~");
    QTextStream in(&file);
    QRegularExpression rx("\\[(\\d{2}):(\\d{2}\\.\\d{1,3})\\]");
    if (mark){
        while (!in.atEnd()){
            QString line = in.readLine();
            QRegularExpressionMatch match = rx.match(line);
            if (match.hasMatch()){
                int minutes = match.captured(1).toInt() * 60;
                float seconds = match.captured(2).toFloat();
                int time = static_cast<int>((static_cast<float>(minutes) + seconds) * 1000.f); // ms
                QString lyricword = line.mid(match.capturedEnd()).trimmed();
                if (!timeSet.contains(time)){
                    lyricTime.append(time);
                    lyric.append(lyricword);
                    timeSet.insert(time);
                }
            }
        }
    }
    if(lyric.size()<=0){
        SentMsg(1, "Lrc解析为空或我看不懂-w-");
        lyric.append("歌词自在心中——沃兹基");
        lyricTime.append(0);
    }
    lyric.append("");
    lyricTime.append(INT_MAX);
}
void ProcessObj::ClearLyric(){
    lyricIndex = 0;
    lyric.clear();
    lyricTime.clear();
}

void ProcessObj::OpLoc(QString path){
    Q_UNUSED(path);
    // _player->stop();
    // _fromList = false;
    // currentSongPath = path;
    // PreLoad(path);
}
void ProcessObj::OpLst(int index, PlayListType type, QString listName){
    Q_UNUSED(type);
    Q_UNUSED(listName);
    Q_UNUSED(index);
    // if(listName == "Bili"){
    //     if(index<0 || index>=biliInfo.parts.size()){
    //         SentMsg(2,"视频不存在喵");
    //         return;
    //     }
    //     _fromList = true;
    //     currentIndex = index;
    //     currentListName = listName;
    //     currentSongPath = "Bili";
    //     //stop optimize
    //     return PreLoad(currentSongPath);
    // }
    // else if(index == -1 || index>=songListMap[listName]->lst.size()){
    //     _player->pause();
    //     qDebug()<<"should stop";
    //     SentMsg(1,"歌单到头了呢, 补充一些吧~");
    //     return;
    // }

    // _fromList = true;
    // QString path = songListMap[listName]->lst[index][5];
    // if(listName!="Search" && listName!="Bili" && !QFile::exists(path)){
    //     qDebug()<<"file not exist! so i will remove it";
    //     SentMsg(2,"丢失本地文件喵，那我帮你移出列表啦~");
    //     songListMap[listName]->RemoveSong(index);
    //     emit FreshTable(listName);
    //     return;
    // }
    // _player->stop();
    // currentIndex = index;
    // currentListName = listName;
    // currentSongPath = path;
    // PreLoad(currentSongPath);
}

bool ProcessObj::OpenLL(QVariant hash, PlayListType type, QString listName, QString msg){
    Q_UNUSED(msg);
    if(hash.isNull()) return false;
    ListPtr playlist = _songManager->GetPlayList(type, listName);
    QString targetPath = hash.toString();
    qDebug() << "in targetPath: " << targetPath;
    int targetIndex = hash.toInt();
    QString targetHash;
    PlayListType targetType = type;
    QString targetListName = listName;
    switch(type){
        case PlayListType::Local:{
            if(targetPath.isEmpty() || !QFileInfo(targetPath).isFile()) return false;
            targetHash = _FileHash(targetPath);
            if(targetHash.isEmpty()) return false;
            targetIndex = _songManager->AddPosition(targetHash, PlayListType::History);
            if(targetIndex<0) return false;
            targetType = PlayListType::History;
            targetListName = "History";
            break;
        }
        case PlayListType::Search:{
            targetHash = _songManager->GetHash(targetIndex,PlayListType::Search);
            if(targetHash.isEmpty()){
                SentMsg(2,"歌曲不存在喵");
                return false;
            }
            targetPath = LoadSourceFromNet(type, targetHash, targetIndex);
            targetIndex = _songManager->AddPosition(targetHash, PlayListType::History);
            targetType = PlayListType::History;
            targetListName = "History";
            break;
        }
        case PlayListType::Bilibili:{
            if(targetIndex < 0 || targetIndex >= biliInfo.parts.size()){
                SentMsg(2,"视频不存在喵");
                return false;
            }
            
            targetHash = biliInfo.parts[targetIndex].hash;
            targetPath = LoadSourceFromNet(type, targetHash, targetIndex);
            targetIndex = _songManager->AddPosition(targetHash, PlayListType::History);
            targetType = PlayListType::History;
            targetListName = "History";
            break;
        }
        case PlayListType::UserPlaylist:
        case PlayListType::Favorites:
        case PlayListType::History:{
            if(!playlist) return false;
            if(targetIndex < 0 || targetIndex >= playlist->songVector.size()){
                SentMsg(2,"歌曲不存在喵");
                return false;
            }
            targetHash = playlist->songVector[targetIndex]->hash;
            targetPath = _songManager->CacheAvaliable(targetHash) ? 
                         _songManager->GetSong(targetHash)->path : "";
            qDebug() << "get target path: "<< targetPath;
            if(targetPath.isEmpty()){
                SentMsg(2,"本地文件丢失! 那我帮你移出列表啦~");
                //playlist->RemoveSong(targetHash);
                _songManager->RemoveSong(targetHash);
                return false;
            }
            targetType = type;
            targetListName = listName;
            break;
        }
    }
    if(!QFileInfo(targetPath).isFile()) {
        return false;
    }
    currentSongPath = targetPath;
    currentSongHash = targetHash;
    currentListName = targetListName;
    currentListType = targetType;
    currentIndex = targetIndex;
    qDebug() << "loaded real path: "<<currentSongPath;
    _loader->setSource(QUrl::fromLocalFile(currentSongPath));
    _player->setSource(QUrl::fromLocalFile(currentSongPath));
    if(!_justSetup) _player->play();
    _justSetup = false;
    CheckLoveState();
    return true;
    // switch(type){
    //     case PlayListType::Local:{
    //         currentSongPath = hash.toString();
    //         currentSongHash = "";
    //         currentListName = "History";
    //         break;
    //     }
    //     case PlayListType::Bilibili:{
    //         currentIndex = hash.toInt();
    //         currentSongHash = biliInfo.parts[currentIndex].hash;
    //         currentListName = "Bili";
    //         break;
    //     }
    //     case PlayListType::Search:{
    //         currentIndex = hash.toInt();
    //         currentSongHash = _songManager->GetSearchSongInfo(currentIndex)->hash;
    //         currentListName = "Search";
    //         break;
    //     }
    //     case PlayListType::History:{
    //         currentIndex = hash.toInt();
    //         currentSongHash = playlist->orderedHashes[currentIndex];
    //         currentListName = "History";
    //         break;
    //     }
    //     case PlayListType::Favorites:{
    //         currentIndex = hash.toInt();
    //         currentSongHash = playlist->orderedHashes[currentIndex];
    //         currentListName = "Favorites";
    //         break;
    //     }
    //     case PlayListType::UserPlaylist:{
    //         currentIndex = hash.toInt();
    //         currentSongHash = playlist->orderedHashes[currentIndex];
    //         currentListName = listName;
    //         break;
    //     }
    // }
    // switch(type){
    //     case PlayListType::Local:{
    //         currentListType = PlayListType::History;
    //         break;
    //     }
    //     case PlayListType::Bilibili:
    //     case PlayListType::Search:{
    //         currentListType = type;
    //         currentSongPath = LoadSourceFromNet();
    //         if(currentSongPath.isEmpty()){
    //             SentMsg(2, "加载出错了");
    //             return;
    //         }
    //         break;
    //     }
    //     case PlayListType::History:
    //     case PlayListType::Favorites:
    //     case PlayListType::UserPlaylist:{
    //         currentSongPath = _songManager->CacheAvaliable(currentSongHash)?
    //         _songManager->GetSong(currentSongHash)->path : "";
    //         currentListType = type;
    //         if(currentSongPath.isEmpty()){
    //             SentMsg(2, "本地文件丢失! 那我帮你移出列表啦~");
    //             playlist->RemoveSong(currentSongHash);
    //             return;
    //         }
    //         break;
    //     }
    // }
}
QString ProcessObj::LoadSourceFromNet(PlayListType type, QString hash, int index){
    QString path;
    if(_songManager->CacheAvaliable(hash))
        return _songManager->GetSong(hash)->path;
    switch(type){
        case PlayListType::Search:{
            SongPtr item = _songManager->searchCache->songVector[index];
            if(item->url.isEmpty()){
                SentMsg(2,"歌曲不存在喵");
                return path;
            }
            QString ep = SaveSong(index);
            path = Rename(ep, item->fileName);
            break;
        }
        case PlayListType::Bilibili:{
            QString newFileName = biliInfo.parts[index].part;
            if(!biliDownloadRename.isEmpty()){
                if(!QFileInfo(_dataPath + QDir::separator() + biliDownloadRename + ".mp3").isFile())
                    newFileName = biliDownloadRename;
                else{
                    SentMsg(1,"文件重名了哦,换个名字试试-w-");
                    return path;
                }
            }
            biliDownloadRename.clear();
            QString ep = SaveBiliSong(index);
            if(ep.isEmpty()){
                SentMsg(2,"QAQ下载失败了");
                return path;
            }
            path = Rename(ep, newFileName);
            break;
        }
        default:return hash;
    }
    return path;    
}
void ProcessObj::LoadMetaData(QMediaPlayer::MediaStatus status){
    if(status == QMediaPlayer::LoadedMedia ){
        metaData = _loader->metaData();
        SongPtr song = QSharedPointer<SongItem>::create();
        song->hash = currentSongHash;
        song->title = metaData[QMediaMetaData::Title].toString().toUtf8();
        song->artist = metaData[QMediaMetaData::ContributingArtist].toString().toUtf8();
        song->album = metaData[QMediaMetaData::AlbumTitle].toString().toUtf8();
        song->path = currentSongPath;
        song->duration = metaData[QMediaMetaData::Duration].toInt();
        _songManager->AddSong(song, currentListType, currentListName);
        emit MetaDataLoaded(true);
    }
    else if (status == QMediaPlayer::InvalidMedia || status == QMediaPlayer::NoMedia){
        SentMsg(2,"ERROR AL1!");
        qDebug()<<"fail in loading media phase1";
    }
}
void ProcessObj::LoadVisualData(bool flag){
    Q_UNUSED(flag);
    QVariant coverArt1 = metaData[QMediaMetaData::ThumbnailImage];
    QVariant coverArt2 = metaData[QMediaMetaData::CoverArtImage];
    if (coverArt1.isValid()) {
        currentCover = coverArt1.value<QPixmap>();
    }
    else if (coverArt2.isValid()) {
        currentCover = coverArt2.value<QPixmap>();
    }
    if(currentCover.isNull()) {
        SentMsg(1,"OOPS, 加载封面失败了, 那就看看猫猫吧~");
        currentCover = QPixmap(":/icon/source/cat3.png");
    }
    emit LoadImgFinised();
    LoadLyric();
    emit LoadLyricFinished();
}

void ProcessObj::Search(QString keyword){
    SentMsg(1,"在找啦......");
    QVector<SongPtr> result = _networkObj->SearchInfo(keyword,1,20);
    if(result.size()>0){
        SentMsg(0,"找到了!来来来, 看看有没有你想要的-w-");
        _songManager->LoadSearchCache(result);
    }
    else{
        SentMsg(2,"OOPS, 信号被外星人抓走了QAQ");
    }
    emit FreshTable(PlayListType::Search);
}

void ProcessObj::SearchBili(QString bid){
    SentMsg(1,"在找啦......");
    QString msg;
    BiliInfo newInfo = _networkObj->GetInfoViaBid(bid,msg);
    if(!msg.isEmpty()){
        SentMsg(2,msg);
        return;
    }
    biliInfo = newInfo;
    _networkObj->DownloadImg(QUrl(biliInfo.pic),cachePath,biliInfo.bvid);
    biliInfo.picPath = cachePath + QDir::separator() + biliInfo.bvid + ".jpg";
    SentMsg(0,"好了-w-");
    emit FreshTable(PlayListType::Bilibili);
}

void ProcessObj::ChangeLove(int index){
    if(index == -1){

    }
    if(currentIndex == -1 || _songManager->ListCount(currentListType, currentListName) <= 0) {
        return;
    }
    index = currentIndex;
    QString hash = _songManager->GetHash(index, currentListType, currentListName);
    bool mark = false;
    if(_songManager->ListContains(hash, PlayListType::Favorites, "Favorites")){
        _songManager->RemoveFromList(hash, PlayListType::Favorites, "Favorites");
        qDebug()<<"removed from favorites list";
        SentMsg(1,"移出收藏-w-");
        mark = true;
    }
    else{
        _songManager->AddSong(_songManager->GetSong(hash), PlayListType::Favorites, "Favorites");
        qDebug()<<"added to favorites list";
        SentMsg(1,"加入收藏-w-");
    }
    if(currentListType == PlayListType::Favorites && mark){
        if(_songManager->GetPlayList(PlayListType::Favorites, "Favorites")->ListCount() <= 0){
            OpenLL(0, PlayListType::History, "History");
        }
        else SwitchNext();
    }
    CheckLoveState();
}

void ProcessObj::Initialize(){
    QDir dir = QDir(_dataPath);
    dir.mkpath(".");
    dir.mkpath("./cache");
    qDebug()<<dir;
    QFile file(settingsFilePath);
    file.open(QIODevice::WriteOnly);
    file.close();
    file.setFileName(songManageFilePath);
    file.open(QIODevice::WriteOnly);
    file.close();
}

void ProcessObj::PlayOrPause(){
    _player->isPlaying()?_player->pause():_player->play();
}
void ProcessObj::Start(){
    _player->play();
}
void ProcessObj::Pause(){
    _player->pause();
}

void ProcessObj::AutoSwitch(QMediaPlayer::MediaStatus status = QMediaPlayer::EndOfMedia){
    qsizetype listsize = _songManager->ListCount(currentListType, currentListName);
    if(listsize <= 0) return;
    if(status!=QMediaPlayer::EndOfMedia) return;
    qDebug()<<status;
    int newIndex = -1;
    switch(currentPlayMode){
        case 0://loop
            newIndex = (currentIndex+1)%static_cast<int>(listsize);
            break;
        case 1://single
            newIndex = currentIndex;
            break;
        case 2://random
            newIndex = QRandomGenerator::global()->bounded(static_cast<int>(listsize));
            break;
        case 3://once
            newIndex = currentIndex+1;
            if(newIndex>=listsize){
                newIndex = -1;
                SentMsg(1,"歌单到头了呢, 补充一些吧~");
                return;
            }
            break;
    }
    qDebug() << newIndex;
    QString cntHash = _songManager->GetHash(newIndex, currentListType, currentListName);
    QString str("接下来是... {songName} -w-");
    str.replace("{songName}",
        _songManager->GetSong(cntHash)->title);
    SentMsg(1,str);
    OpenLL(newIndex, currentListType, currentListName);
}
void ProcessObj::SwitchNext(){
    qsizetype listsize = _songManager->ListCount(currentListType, currentListName);
    if(listsize <= 0){
        qDebug()<<"list it empty";
        SentMsg(2,"列表是空的咧!o.O");
        return;
    }
    int newIndex = -1;
    if(currentPlayMode==0||currentPlayMode==1||currentPlayMode==3){
        newIndex = (currentIndex+1)%static_cast<int>(listsize);
        OpenLL(newIndex,currentListType,currentListName);
    }
    else{
        newIndex = QRandomGenerator::global()->bounded(static_cast<int>(listsize));
        OpenLL(newIndex,currentListType,currentListName);
    }
}
void ProcessObj::SwitchPrev(){
    qsizetype listsize = _songManager->ListCount(currentListType, currentListName);
    if(listsize <= 0) {
        qDebug()<<"list is empty";
        SentMsg(2,"列表是空的咧!o.O");
        return;
    }
    int newIndex = -1;
    if(currentPlayMode==0||currentPlayMode==1||currentPlayMode==3){
        newIndex = static_cast<int>((currentIndex-1+listsize)%listsize);
    }
    else{
        newIndex = QRandomGenerator::global()->bounded(static_cast<int>(listsize));
    }
    OpenLL(newIndex,currentListType,currentListName);
}
void ProcessObj::SwitchPlayMode(){
    currentPlayMode = (currentPlayMode+1)%4;
    qDebug()<<currentPlayMode;
    switch(currentPlayMode){
        case 0: SentMsg(1,"循环播放-w-"); break;
        case 1: SentMsg(1,"单曲循环-w-"); break;
        case 2: SentMsg(1,"随机播放-w-"); break;
        case 3: SentMsg(1,"顺序播放-w-"); break;
    }
    emit PlayModeChanged(currentPlayMode);
}

QMediaPlayer* ProcessObj::GetPlayer(){
    return _player;
}
void ProcessObj::SetSliderPos(qint64 pos){
    _sliderPos = pos;
}
qint64 ProcessObj::GetSliderPos(){
    return _sliderPos;
}

void ProcessObj::SetVolume(double volume, bool level){
    if(level) volume = Level2Volume(volume);
    volume = qBound(0.0, volume, 1.0);
    if(volume <= 0.01) volume = 0.0;
    _audio->setVolume(static_cast<float>(volume));
    currentVolume = static_cast<float>(volume);
}

void ProcessObj::CheckLoveState(){
    if(_songManager->ListCount(PlayListType::Favorites) <= 0) {
        currentLove = false;
        emit LoveStateUpdate(currentLove);
        return;
    }
    if(currentIndex == -1 || _songManager->ListCount(PlayListType::Favorites)<=0) {
        currentLove = false;
    }
    else if(_songManager->ListContains(currentSongHash, PlayListType::Favorites, "Favorites")){
        currentLove = true;
    }
    else{
        currentLove = false;
    }
    emit LoveStateUpdate(currentLove);
}
void ProcessObj::SentMsg(int type, QString msg){
    msgTimer->setSingleShot(true);
    if(msgque.empty()){
        msgque.push(std::make_pair(type,msg));
        msgTimer->start(3000);
        emit ShMsg(type,msg);
    }
    else{
        msgque.push(std::make_pair(type,msg));
    }
}

QList<QString> ProcessObj::GetInfo(QMediaMetaData dat){
    QString title = dat[QMediaMetaData::Title].toString();
    if(title.isEmpty()) title = _loader->source().fileName();
    QString artist = dat[QMediaMetaData::ContributingArtist].toString();
    if(artist.isEmpty()) artist = dat[QMediaMetaData::AlbumArtist].toString();
    if(artist.isEmpty()) artist = dat[QMediaMetaData::Author].toString();
    if(artist.isEmpty()) artist = dat[QMediaMetaData::Composer].toString();
    if(artist.isEmpty()) artist = "Unknown";
    QString album = dat[QMediaMetaData::AlbumTitle].toString();
    if(album.isEmpty()) album = "Unknown";
    QString duration = dat[QMediaMetaData::Duration].toString();
    QList<QString> info = {0,currentSongHash,title,artist,album,currentSongPath,duration};
    return info;
}

ListPtr ProcessObj::GetPlayList(const PlayListType type, const QString listName)const {
    return _songManager->GetPlayList(type,listName);
}

SongPtr ProcessObj::GetSong(const QString &hash)const {
    return _songManager->GetSong(hash);
}

void ProcessObj::DoFFTSpectrum(const float* data, int sampleCount){
    //qDebug()<<"Doing FFT";
    constexpr int N = _Npoint;
    if(sampleCount<N) return;
    QVector<double> fftData(N);
    fftw_complex out[static_cast<long long unsigned>(N)];
    for(int i=0;i<N;i++){
        fftData[i] = data[i];
    }
    //do fft
    fftw_plan plan = fftw_plan_dft_r2c_1d(N,fftData.data(),out,FFTW_ESTIMATE);
    fftw_execute(plan);

    for(int i=0;i<N/2;i++){
        _magnitudeArr[i] = sqrt(out[i][0]*out[i][0]+out[i][1]*out[i][1]);
    }
    //qDebug()<<"Frequency 200: "<<_magnitudeArr[200];

    fftw_destroy_plan(plan);
    SpectrumToBand();
}

void ProcessObj::SpectrumToBand(){
    double total = 0;
    double sum = 0;
    int SP1 = 20, SP2 = 30, SP3 = 7, SP4 = 7;//8 24 192 96
    int CT1 = 1, CT2 = 2, CT3 = 3, CT4 = 4;
    int i=0, delta = 127, p=0;
    Q_UNUSED(delta);
    for(;p<SP1;++p,i+=CT1){
        sum = 0;
        for(int j=0;j<CT1;++j){
            sum += _magnitudeArr[i+j];
        }
        _bandArr[p] = sum;
    }
    for(;p<SP1+SP2;++p,i+=CT2){
        sum = 0;
        for(int j=0;j<CT2;++j){
            sum += _magnitudeArr[i+j];
        }
        _bandArr[p] = sum;
    }
    for(;p<SP1+SP2+SP3;++p,i+=CT3){
        sum = 0;
        for(int j=0;j<CT3;++j){
            sum += _magnitudeArr[i+j];
        }
        _bandArr[p] = sum;
    }
    for(;p<SP1+SP2+SP3+SP4;++p,i+=CT4){
        sum = 0;
        for(int j=0;j<CT4;++j){
            sum += _magnitudeArr[i+j];
        }
        _bandArr[p] = sum;
    }
    i=0;
    for(;i<64;i++){
        total+= _bandArr[i]*1.5;
        _bandArr[i] = _bandArr[i]<=1.5?0.3:log(_bandArr[i]);
    }
    emit SpectrumCalculated(_bandArr, total);
}

void ProcessObj::_DefaultSettings(){
    currentVolume = 0.3f;
    currentPlayMode = 0;
    currentIndex = -1;
    currentListName = "History";
    currentSongPath = "";
    currentBgIndex = 6;
    currentBlur = 0;
    backgroundPath = "";
}
void ProcessObj::_ApplySettingsFromLoad(bool mark){
    // currentVolume = _currentSettings[0].toFloat();
    // currentPlayMode = _currentSettings[1].toInt();
    // currentIndex = _currentSettings[2].toInt();
    // currentBgIndex = _currentSettings[5].toInt();
    // currentBlur = _currentSettings[6].toInt();
    // currentListName = _currentSettings[3];
    // currentSongPath = _currentSettings[4];
    // backgroundPath = _currentSettings[7];
    QJsonObject obj= settings.object();
    currentVolume = static_cast<float>(mark&&obj.contains("volume")?(obj["volume"].toDouble()):0.3f);
    currentPlayMode = mark&&obj.contains("playMode")?(obj["playMode"].toInt()):0;
    currentIndex = mark&&obj.contains("index")?(obj["index"].toInt()):-1;
    currentBgIndex = mark&&obj.contains("bgIndex")?(obj["bgIndex"].toInt()):6;
    currentBlur = mark&&obj.contains("blur")?(obj["blur"].toInt()):0;
    currentListName = mark&&obj.contains("listName")?obj["listName"].toString():"History";
    currentListType = mark&&obj.contains("listType")?static_cast<PlayListType>(obj["listType"].toInt()):PlayListType::History;
    currentSongPath = mark&&obj.contains("songPath")?obj["songPath"].toString():"";
    qDebug() <<"loaded current song path: " << currentSongPath;
    backgroundPath = mark&&obj.contains("bgPath")?obj["bgPath"].toString():"";
    currentSongHash = mark&&obj.contains("songHash")?obj["songHash"].toString():"";
}


double ProcessObj::Volume2Level(double volume){
    if (volume <= 0.0) return 0.0;
    if (volume >= 1.0) return 100.0;
    return std::log10(1 + 9 * volume) * 100.0;
}

double ProcessObj::Level2Volume(double level){
    if (level <= 0.0) return 0.0;
    if (level >= 100.0) return 1.0;
    return (std::pow(10.0, level / 100.0) - 1) / 9.0;
}

QString ProcessObj::_FileHash(QString path)const {
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)){
        qDebug() << "Can't open the file";
        return "";
    }
    QByteArray data = file.readAll();
    file.close();
    QByteArray hash = QCryptographicHash::hash(data,QCryptographicHash::Sha256);
    return hash.toHex();
}





