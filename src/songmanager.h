#pragma once
#include <QString>
#include <QVector>
#include <QObject>
#include <QSharedPointer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QHash>
#include <QSet>
#include <memory>
#include <QDir>
#include <QFile>

enum class SourceType{
    Local,
    Network
};

enum class PlayListType{
    History,
    Favorites,
    Search,
    Bilibili,
    UserPlaylist,
    Local
};

class SongItem{
public:
    QString hash,
            title,
            artist,
            album,
            path;
    int duration = 0;//milliseconds
    SourceType source = SourceType::Local;
    QString url,
            lrcUrl,
            imgUrl,
            fileName;
    QJsonObject ToObject() const {
        QJsonObject obj;
        obj.insert("hash", hash);
        obj.insert("title", title);
        obj.insert("artist", artist);
        obj.insert("album", album);
        obj.insert("path", path);
        obj.insert("duration", duration);
        obj.insert("source", static_cast<int>(source));
        return obj;
    }
    SongItem(){}
    SongItem(const QJsonObject& itemObject){
        hash = itemObject["hash"].toString();
        title = itemObject["title"].toString();
        artist = itemObject["artist"].toString();
        album = itemObject["album"].toString();
        path = itemObject["path"].toString();
        duration = itemObject["duration"].toInt();
        source = static_cast<SourceType>(itemObject["source"].toInt());
    }
};

using SongPtr = QSharedPointer<SongItem>;

class PlayList{
public:
    QSet<QString> songHashes;
    QVector<SongPtr> songVector;
    PlayListType type = PlayListType::History;

public:
    PlayList(PlayListType listType = PlayListType::History) : type(listType) {}
    bool IsExist(const QString &hash) const {
        return songHashes.contains(hash);
    }
    bool AddSong(const SongPtr &song) {
        if(!song || IsExist(song->hash)) return false;
        songHashes.insert(song->hash);
        songVector.append(song);
        return true;
    }
    bool RemoveSong(const QString &hash) {
        if (!IsExist(hash)) return false;
        songHashes.remove(hash);
        for (auto it = songVector.begin(); it != songVector.end(); ++it) {
            if ((*it)->hash == hash) {
                songVector.erase(it);
                break;
            }
        }
        return true;
    }
    qsizetype ListCount() const {
        return songHashes.size();
    }
    int AddPosition(const QString &hash)const {
        if (!IsExist(hash)) return static_cast<int>(songVector.size());
        for (qsizetype i = 0; i < songVector.size(); ++i) {
            if (songVector[i]->hash == hash) return static_cast<int>(i);
        }
        return -1;
    }
};

using ListPtr = QSharedPointer<PlayList>;

class SongManager: public QObject{
    Q_OBJECT
public:
    explicit SongManager(QObject *parent = nullptr, QString filePath = QString(), bool autoSave = false): QObject(parent) {
        Q_UNUSED(autoSave);
        if(!filePath.isEmpty()){
            _filePath = filePath;
            LoadFromDisk(filePath);
        }
    }
    ListPtr GetPlayList(PlayListType type, QString listName = QString()) const{
        if (type == PlayListType::UserPlaylist && !listName.isEmpty()) {
            if (_userPlaylists.contains(listName)) return _userPlaylists.value(listName);
        }
        else if (_playlists.contains(type)) return _playlists.value(type);
        return nullptr;
    }
    SongPtr GetSong(const QString &hash) const{
        if (_songPool.contains(hash)) return _songPool.value(hash);
        return nullptr;
    }
    QString GetHash(int index, PlayListType type, QString listName = QString()) const {
        ListPtr playlist = GetPlayList(type, listName);
        if (playlist && index >= 0 && index < playlist->ListCount()) {
            return playlist->songVector[index]->hash;
        }
        return QString();
    }
    void AddSong(const SongPtr &song, PlayListType type, const QString &userPlaylistName = QString(), bool emitSignal = true){
        if (!song || song->hash.isEmpty() || song->source!=SourceType::Local) return;
        //current we don't add network sources into the pool
        if (!_songPool.contains(song->hash)) _songPool.insert(song->hash, song);
        else _songPool[song->hash] = song; // Update existing song
        if (type == PlayListType::UserPlaylist && !userPlaylistName.isEmpty()) {
            if (!_userPlaylists.contains(userPlaylistName)) return;
            if (_userPlaylists[userPlaylistName]->AddSong(song) && emitSignal) emit ListModified(type, userPlaylistName);
            return;
        } 
        else {
            if (!_playlists.contains(type)) return;
            if (_playlists[type]->AddSong(song) && emitSignal) emit ListModified(type);
            return;
        }
    }
    void AddSongs(const QVector<SongPtr> &songs, PlayListType type, const QString &userPlaylistName = QString()){
        for (const auto &song : songs) {
            AddSong(song, type, userPlaylistName, false);
        }
        emit ListModified(type, userPlaylistName);
    }
    bool CacheExist(const QString &hash) const {
        return _songPool.contains(hash);
    }
    bool CacheAvaliable(const QString &hash) const {
        if (CacheExist(hash)) {
            return QFileInfo(_songPool[hash]->path).isFile();
        }
        return false;
    }
    void CreateUserList(const QString &listName){
        if (listName.isEmpty() || _userPlaylists.contains(listName)) return;
        ListPtr newList = QSharedPointer<PlayList>::create(PlayListType::UserPlaylist);
        _userPlaylists.insert(listName, newList);
        emit ListModified(PlayListType::UserPlaylist, listName);
    }
    void RemoveSong(const QString &hash){
        bool dirty = false;
        if(hash.isEmpty()) return;
        if (_songPool.contains(hash)) {
            _songPool.remove(hash);
            dirty = true;
        }
        for (auto &playlist : _playlists) {
            if (playlist->RemoveSong(hash)) dirty = true;
        }
        for (auto &userPlaylist : _userPlaylists) {
            if (userPlaylist->RemoveSong(hash)) dirty = true;
        }
        if (dirty) emit ListModified(PlayListType::UserPlaylist);
    }
    void RemoveFromList(const QString &hash, PlayListType type, const QString &listName = QString()){
        ListPtr playlist = GetPlayList(type, listName);
        if (playlist && playlist->RemoveSong(hash)) emit ListModified(type, listName);
    }
    bool ListContains(const QString &hash, PlayListType type, const QString &listName = QString()) const {
        ListPtr playlist = GetPlayList(type, listName);
        if (playlist) return playlist->IsExist(hash);
        return false;
    }
    qsizetype ListCount(PlayListType type, const QString &listName = QString()) const {
        ListPtr playlist = GetPlayList(type, listName);
        if (playlist) return playlist->ListCount();
        return 0;
    }
    void CleanList(PlayListType type, const QString &listName = QString()){
        ListPtr playlist = GetPlayList(type, listName);
        if (playlist) {
            playlist->songHashes.clear();
            playlist->songVector.clear();
            emit ListModified(type, listName);
        }
    }
    void UpdateItemPath(const QString &hash, const QString &newPath){
        if (hash.isEmpty() || newPath.isEmpty()) return;
        if (_songPool.contains(hash)) {
            _songPool[hash]->path = newPath;
            emit ListModified(PlayListType::UserPlaylist);
        }
    }
    void LoadSearchCache(QVector<SongPtr> songs){
        CleanList(PlayListType::Search);
        for(const auto& song : songs){
            searchCache->AddSong(song);
        }
        emit ListModified(PlayListType::Search);
    }
    int AddPosition(const QString &hash, PlayListType type, const QString &listName = QString())const {
        ListPtr playlist = GetPlayList(type, listName);
        if (playlist){
            return playlist->AddPosition(hash);
        }
        return -1;
    }

    void InitStore(){
        _songPool.clear();
        _playlists.clear();
        _userPlaylists.clear();
        _playlists.insert(PlayListType::History, QSharedPointer<PlayList>::create(PlayListType::History));
        _playlists.insert(PlayListType::Favorites, QSharedPointer<PlayList>::create(PlayListType::Favorites));
        searchCache = QSharedPointer<PlayList>::create(PlayListType::Search);
        _playlists.insert(PlayListType::Search, searchCache);
        // _playlists.insert(PlayListType::Search, QSharedPointer<PlayList>::create(PlayListType::Search));
        // _playlists.insert(PlayListType::Bilibili, QSharedPointer<PlayList>::create(PlayListType::Bilibili));
    }
    bool LoadFromDisk(QString filePath){
        InitStore();
        if (filePath.isEmpty() || !QFileInfo(filePath).isFile()) return false;
        QFile file(filePath);
        QJsonDocument doc;
        if (file.open(QIODevice::ReadOnly)) {
            doc = QJsonDocument::fromJson(file.readAll());
            file.close();
        }
        else return false;
        if (doc.isNull() || !doc.isObject()) return true;
        QJsonObject rootObj = doc.object();
        if (rootObj["version"].toInt() != 1) return true;
        QJsonArray playlistsArray = rootObj["playlists"].toArray();
        for (const QJsonValue &value : playlistsArray) {
            if (!value.isObject()) continue;
            QJsonObject playlistObj = value.toObject();
            PlayListType type = static_cast<PlayListType>(playlistObj["listType"].toInt());
            switch(type){
                case PlayListType::History:
                case PlayListType::Favorites:{
                    QVector<SongPtr> songs;
                    QJsonArray songsArray = playlistObj["data"].toArray();
                    for (const QJsonValue &songValue : songsArray) {
                        if (songValue.isObject()) {
                            QJsonObject songObj = songValue.toObject();
                            SongPtr song(QSharedPointer<SongItem>::create(songObj));
                            if (song) songs.append(song);
                        }
                    }
                    AddSongs(songs, type);
                    break;
                }
                case PlayListType::UserPlaylist:{
                    QVector<SongPtr> songs;
                    QJsonArray songsArray = playlistObj["data"].toArray();
                    for (const QJsonValue &songValue : songsArray) {
                        if (songValue.isObject()) {
                            QJsonObject songObj = songValue.toObject();
                            SongPtr song(QSharedPointer<SongItem>::create(songObj));
                            if (song) songs.append(song);
                        }
                    }
                    AddSongs(songs, type, playlistObj["listName"].toString());
                    break;
                }
                default:break;
            }
        }
        return true;
    }
    void SaveToDisk(QString filePath) const{
        QJsonDocument doc;
        QJsonObject rootObj;
        rootObj["version"] = 1;
        QJsonArray playlistsArray;
        for(PlayListType type:_playlists.keys()){
            switch(type){
                case PlayListType::History:
                case PlayListType::Favorites:{
                    QJsonObject playlistObj;
                    playlistObj["listType"] = static_cast<int>(type);
                    playlistObj["listName"] = "";
                    QJsonArray songsArray;
                    for(const auto& item : _playlists[type]->songVector){
                        songsArray.append(item->ToObject());
                    }
                    playlistObj["data"] = songsArray;
                    playlistsArray.append(playlistObj);
                    break;
                }
                default:break;
            }
        }
        for(QString listName : _userPlaylists.keys()){
            QJsonObject playlistObj;
            playlistObj["listType"] = static_cast<int>(PlayListType::UserPlaylist);
            playlistObj["listName"] = listName;
            QJsonArray songsArray;
            for(const auto& item : _userPlaylists[listName]->songVector){
                songsArray.append(item->ToObject());
            }
            playlistObj["data"] = songsArray;
            playlistsArray.append(playlistObj);
        }
        rootObj["playlists"] = playlistsArray;
        doc.setObject(rootObj);
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(doc.toJson());
            file.close();
        }
    }
    ~SongManager() {
        if (_autoSave) {
            SaveToDisk(_filePath);
        }
    }

signals:
    void ListModified(PlayListType type, const QString &listName = QString());

public:
    ListPtr searchCache;

private:
    //it 'holds' song items
    QHash<QString, SongPtr> _songPool;
    QVector<SongPtr> _biliCache;

    QHash<PlayListType, ListPtr> _playlists;
    QHash<QString, ListPtr> _userPlaylists;

    bool _autoSave = false;
    QString _filePath;

};