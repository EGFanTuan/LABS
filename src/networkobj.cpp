#include "networkobj.h"
#include <QUrl>
#include <QUrlQuery>
#include <QTimer>
#include <QEventLoop>
#include <QStandardPaths>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPixmap>
#include <QDateTime>
#include <QDir>
#include <QProcess>
#include <QCoreApplication>
#include <QCryptographicHash>
#include "taglib.h"


NetworkObj::NetworkObj(QObject* parent):QObject(parent),manager(),request(),bili_request(){
    request.setRawHeader("User-Agent",header.toUtf8());
    QSslConfiguration sslconfig = request.sslConfiguration();
    sslconfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(sslconfig);

    bili_request.setRawHeader("User-Agent",header.toUtf8());
    bili_request.setRawHeader("Referer", "https://www.bilibili.com/");
    QSslConfiguration bili_sslconfig = bili_request.sslConfiguration();
    bili_sslconfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    bili_request.setSslConfiguration(bili_sslconfig);

    ffmpegPath = QCoreApplication::applicationDirPath() + QDir::separator() + "ffmpeg.exe";
}

QUrl NetworkObj::PraseUrl(QString keyword,int page_limit, int count_limit){
    QUrl url("https://mobiles.kugou.com/api/v3/search/song");
    QUrlQuery query;
    query.addQueryItem("format", "json");
    query.addQueryItem("keyword", keyword);
    query.addQueryItem("page", QString::number(page_limit));
    query.addQueryItem("pagesize", QString::number(count_limit));
    query.addQueryItem("showtype", "1");
    url.setQuery(query);
    qDebug()<<"search url = " << url;
    return url;
}

QUrl NetworkObj::PraseSrcUrl(QString hash){
    QUrl url("https://m.kugou.com/app/i/getSongInfo.php");
    QUrlQuery query;
    query.addQueryItem("hash", hash);
    query.addQueryItem("cmd", "playInfo");
    url.setQuery(query);
    qDebug()<<"song info url = " << url;
    return url;
}

QByteArray NetworkObj::GetUrlkData(QUrl url,bool picOpti, QNetworkRequest* customRequest){
    Q_UNUSED(picOpti);
    if(customRequest){
        request = *customRequest;
    }
    request.setUrl(url);
    QNetworkReply* reply = manager.get(request);
    QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> ptr(reply);
    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;

    connect(&timer, &QTimer::timeout, &loop, [&](){
        reply->abort();
        qDebug() << "Time out with Url = "<<url;
        loop.quit();
    });
    connect(reply, &QNetworkReply::finished, &loop, [&](){
        loop.quit();
    });

    timer.start(15000);
    loop.exec();

    QByteArray data;
    if(reply->error() == QNetworkReply::NoError){
        data = reply->readAll();
    }
    else{
        qDebug() << reply->errorString();
        qDebug() <<"Reply error with Url = "<<url;
    }
    return data;
}

QList<QMap<QString,QString>> NetworkObj::GetInfo(QString keyword,int page_limit,int count_limit){
    QList<QMap<QString,QString>>result;
    QByteArray data = GetUrlkData(PraseUrl(keyword,page_limit,count_limit));
    if(data.isNull())
    {
        qDebug()<<"No data replied";
        return result;
    }
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray array = doc.object().value("data").toObject().value("info").toArray();
    qDebug()<<"found "<<array.size()<<" songs";
    for(int i=0;i<array.size();i++){
        QMap<QString,QString> info;
        QString hash = array.at(i).toObject().value("hash").toString();
        QByteArray data2 = GetUrlkData(PraseSrcUrl(hash));
        QJsonObject obj2 = QJsonDocument::fromJson(data2).object();
        QString url = obj2.value("url").toString().toUtf8();
        if(url.isEmpty()) continue;
        QString lrcUrl = GetLrcUrl(hash);
        info.insert("hash",hash);
        info.insert("songname",array[i].toObject().value("songname").toString().toUtf8());
        info.insert("album_name",array[i].toObject().value("album_name").toString().toUtf8());
        info.insert("author_name",obj2.value("author_name").toString().toUtf8());
        info.insert("timeLength",QString::number(obj2.value("timeLength").toInt()));
        info.insert("fileName",obj2.value("fileName").toString().toUtf8());

        info.insert("url",url);
        info.insert("lrcUrl",lrcUrl.toUtf8());
        info.insert("imgUrl",obj2.value("album_img").toString().replace("{size}",obj2.value("fileSize").toString()).toUtf8());

        result.append(info);
    }

    return result;
}
QVector<SongPtr> NetworkObj::SearchInfo(QString keyword, int page_limit, int count_limit, SearchAPI api){
    Q_UNUSED(api);
    QVector<SongPtr> result;
    QByteArray data = GetUrlkData(PraseUrl(keyword, page_limit, count_limit));
    if(data.isNull()){
        qDebug() << "No data replied";
        return result;
    }
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray array = doc.object().value("data").toObject().value("info").toArray();
    for(int i=0;i<array.size();i++){
        SongPtr song = QSharedPointer<SongItem>::create();
        QString hash = array.at(i).toObject().value("hash").toString().toUtf8();
        QByteArray data2 = GetUrlkData(PraseSrcUrl(hash));
        QJsonObject obj2 = QJsonDocument::fromJson(data2).object();
        QString lrcUrl = GetLrcUrl(hash);
        song->hash = hash;
        song->title = array[i].toObject().value("songname").toString().toUtf8();
        song->album = array[i].toObject().value("album_name").toString().toUtf8();
        song->artist = obj2.value("author_name").toString().toUtf8();
        song->duration = obj2.value("timeLength").toInt() * 1000;
        song->fileName = obj2.value("fileName").toString().toUtf8();

        song->url = obj2.value("url").toString().toUtf8();
        song->lrcUrl = lrcUrl.toUtf8();
        song->imgUrl = obj2.value("album_img").toString().replace("{size}",obj2.value("fileSize").toString()).toUtf8();

        song->source = SourceType::Network;

        if(song->url.isEmpty()) continue;
        result.append(song);
    }
    return result;
}

BiliInfo NetworkObj::GetInfoViaBid(QString bid, QString& message){
    BiliInfo result;
    QUrl url("https://api.bilibili.com/x/web-interface/wbi/view");
    QUrlQuery query;
    query.addQueryItem("bvid", bid);
    url.setQuery(query);
    QByteArray data(GetUrlkData(url, false, &bili_request));
    if(data.isNull()){
        message = "Failed to get video info";
        return result;
    }
    QJsonObject videoInfo(QJsonDocument::fromJson(data).object());
    int errorCode = videoInfo.value("code").toInt();
    if(errorCode != 0){
        message = videoInfo.value("message").toString();
        return result;
    }

    QJsonObject rdata(videoInfo.value("data").toObject());
    result.bvid = rdata.value("bvid").toString();
    result.pic = rdata.value("pic").toString();
    result.title = rdata.value("title").toString();
    result.desc = rdata.value("desc").toString();
    result.videos = rdata.value("videos").toInt();
    result.cid = rdata.value("cid").toInteger();

    QJsonArray partsArray = rdata.value("pages").toArray();
    for(int i=0;i<partsArray.size();i++){
        PartInfo part;
        QJsonObject item(partsArray.at(i).toObject());
        part.part = item.value("part").toString();
        part.first_frame = item.value("first_frame").toString();
        part.cid = item.value("cid").toInteger();
        part.page = item.value("page").toInt();
        QString hashStr = QString::number(part.cid) + result.bvid;
        part.hash = QCryptographicHash::hash(hashStr.toUtf8(), QCryptographicHash::Sha256).toHex();
        result.parts.append(part);
    }

    return result;
}

QString NetworkObj::GetLrcUrl(QString hash){
    QString rstr = "http://krcs.kugou.com/search?ver=1&man=yes&client=mobi&keyword=&duration=&hash={music_id}&album_audio_id=";
    rstr.replace("{music_id}",hash);
    QByteArray data = GetUrlkData(QUrl(rstr));
    QJsonObject obj = QJsonDocument::fromJson(data).object();
    QString id = obj.value("candidates").toArray().at(0).toObject().value("id").toString();
    QString accesskey = obj.value("candidates").toArray().at(0).toObject().value("accesskey").toString();
    QString bstr = "https://lyrics.kugou.com/download?ver=1&client=pc&id={id}&accesskey={accesskey}&fmt=lrc&charset=utf8";
    bstr.replace("{id}",id);
    bstr.replace("{accesskey}",accesskey);
    return bstr;
}

void NetworkObj::DownloadSong(QUrl url, QString path, QString filename){
    QByteArray data = GetUrlkData(url);
    if(data.isNull()){
        qDebug() <<"Fail to download song";
        return;
    }
    path += "/{name}.mp3";
    path.replace("{name}",filename);
    QFile file(path);
    if(file.open(QIODevice::WriteOnly)){
        file.write(data);
        file.close();
    }
    else{
        qDebug() << "IO error while downloading song";
    }
}

void NetworkObj::DownloadLrc(QUrl url, QString path, QString filename){
    QByteArray data = GetUrlkData(url);
    if(data.isNull()){
        qDebug() <<"Fail to download lrc";
        return;
    }
    QString rawLrc = QJsonDocument::fromJson(data).object().value("content").toString();
    QByteArray lrcdata = QByteArray::fromBase64(rawLrc.toUtf8());
    path += "/{name}.lrc";
    path.replace("{name}",filename);
    QFile file(path);
    if(file.open(QIODevice::WriteOnly)){
        file.write(lrcdata);
        file.close();
    }
    else{
        qDebug() << "IO error while downloading lrc";
    }
}

void NetworkObj::DownloadImg(QUrl url, QString path, QString filename){
    QByteArray data = GetUrlkData(url,true);
    if(data.isNull()){
        qDebug()<<"Fail to download img";
        return;
    }
    QString imgpath = path + "/{name}.jpg";
    imgpath.replace("{name}",filename);
    QPixmap img;
    img.loadFromData(data);
    img.save(imgpath);
    QFile file(imgpath);
    if(file.open(QIODevice::ReadOnly)){
        file.close();
    }
    else{
        qDebug() << "IO error while downloading img";
    }
}

void NetworkObj::DownloadM4a(QString bid, QString cid, QString path, QString filename, QString& message, bool toMp3){
    QUrl getDashUrl("https://api.bilibili.com/x/player/wbi/playurl");
    QUrlQuery query;
    query.addQueryItem("bvid", bid);
    query.addQueryItem("cid", cid);
    query.addQueryItem("fnval", "16");
    getDashUrl.setQuery(query);
    QByteArray data = GetUrlkData(getDashUrl);
    if(data.isNull()){
        message = "Failed to get dash info";
        return;
    }
    QJsonObject dashInfo(QJsonDocument::fromJson(data).object());
    if(dashInfo.value("code").toInt() != 0){
        message = dashInfo.value("message").toString();
        return;
    }
    QJsonArray audioStreams = dashInfo.value("data").toObject().value("dash").toObject().value("audio").toArray();
    if(audioStreams.isEmpty()){
        message = "No audio stream found";
        return;
    }
    int selected_audio = 0;
    for(int i=0;i<audioStreams.size();i++){
        QJsonObject item(audioStreams.at(i).toObject());
        if(item.value("id").toInt() == 30280){
            selected_audio = i;
            break;
        }
    }
    QString audioUrlStr = audioStreams.at(selected_audio).toObject().value("baseUrl").toString();
    QUrl audioUrl(audioUrlStr);
    QByteArray audioData = GetUrlkData(audioUrl, false, &bili_request);
    if(audioData.isNull()){
        message = "Failed to download audio data";
        return;
    }
    QString audioPathmfa = path + QDir::separator() + filename + ".m4a";
    QString audioPathmpt = path + QDir::separator() + filename + ".mp3";
    QFile audioFile(audioPathmfa);
    if(audioFile.open(QIODevice::WriteOnly)){
        audioFile.write(audioData);
        audioFile.close();
    }
    else{
        qDebug() << "IO error while downloading audio";
        message = "IO error while downloading audio";
        return;
    }
    if(!Convert2mp3E(audioPathmfa, audioPathmpt)){
        message = "Failed to convert audio to MP3";
        return;
    }
    qDebug() << "Audio downloaded successfully to: " << (toMp3 ? audioPathmpt : audioPathmfa);
    return;
}

bool NetworkObj::Convert2mp3E(QString target, QString output, bool deleteTarget){
    QProcess ffmpeg;
    QStringList args;
    args << "-y" << "-i" << target << "-vn" << "-ar" << "44100" << "-ac" << "2" << "-b:a" << "192k" << output;
    ffmpeg.start(ffmpegPath, args);
    ffmpeg.waitForFinished();

    if (ffmpeg.exitStatus() != QProcess::NormalExit) {
        qDebug() << "ffmpeg failed:" << ffmpeg.readAllStandardError();
        return false;
    }

    if(deleteTarget){
        QFile::remove(target);
    }

    return true;
}




