#ifndef PROCESSOBJ_H
#define PROCESSOBJ_H

#include <QObject>
#include <QProcess>
#include <QDebug>
#include <QString>
#include <qfile.h>
#include <QMediaDevices>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QAudioInput>
#include <QAudioDevice>
#include <QTableWidget>
#include <QJsonDocument>
#include <QMediaMetaData>
#include <QAudioBuffer>
#include <QVariant>
#include <queue>
#include <QAudioBufferOutput>
#include "networkobj.h"
#include "include/fftw/fftw3.h"


class ProcessObj : public QObject
{
    Q_OBJECT
private:
    static constexpr int _Npoint = 2048;
    double* _magnitudeArr, *_bandArr;

    QMediaPlayer* _player;
    QMediaPlayer* _loader;
    QAudioDevice _device;
    QAudioOutput* _audio;
    NetworkObj* _networkObj;
    QAudioBufferOutput* _bufferReceriver;
    SongManager* _songManager;

    //TODO: add more settings
    static const QString _dataPath;//data path
    qint64 _sliderPos;
    //TODO: move _justSetup to settings
    bool // _fromList,
        _justSetup;
    //QVector<SongPtr> _searchResult;

    //TODO: new msg system
    QTimer* msgTimer;
    std::queue<std::pair<int,QString>> msgque;
public:
    bool currentLove,iniF,iniN;//iniN iniF for msg....
    QString currentSongPath,currentListName,currentSongHash;
    PlayListType currentListType;
    static const QString cachePath, songManageFilePath, settingsFilePath;
    int currentIndex,currentPlayMode,lyricIndex,currentBgIndex,currentBlur;
    float currentVolume,savedVolume;
    QString searchtext,backgroundPath;
    QJsonDocument settings, songManageFile;
    QMediaMetaData metaData;
    QPixmap currentCover;
    BiliInfo biliInfo;
    QString biliDownloadRename;
    
    QList<QString> lyric;
    QList<int> lyricTime;

public: 
    explicit ProcessObj(QObject *parent = nullptr);
    void SaveSongList();//try to load
    void LoadSongList();
    void SaveSettings();
    void LoadSettings();

    [[deprecated("Use _songManager->LoadSearchCache instead")]]
    void ListToJson();//before saving
    void SettingToJson();

    //download according to index, add tags, return timeEpoch
    QString SaveSong(int index);
    //rename file according to index, return new path
    QString Rename(QString Epoch, QString target);
    //download m4a for bilibili.com, convert to mp3, add tags, return timeEpoch
    QString SaveBiliSong(int index);
    QString LoadSourceFromNet(PlayListType type, QString hash, int index);

    [[deprecated("Use OpenLL instead")]]
    void OpLst(int index, PlayListType type, QString listName = QString());//open song from list
    [[deprecated("Use OpenLL instead")]]
    void OpLoc(QString path);//open song from local
    //@param hash: the index of song if from list(both net and local), other wise local path
    bool OpenLL(QVariant hash, PlayListType type, QString listName = QString(), QString msg = QString());
    [[deprecated("Use OpenLL instead")]]
    void PreLoad(QString path);//check src from loc or net
    //AfterLoad1&2
    void LoadLyric();//load lyric(not set), emit LoadLyricFinished
    void CheckLoveState();//check if current song is in love list
    ListPtr GetPlayList(const PlayListType type, const QString listName = QString())const;
    SongPtr GetSong(const QString &hash) const;

    double Level2Volume(double level);
    double Volume2Level(double volume);

    void Start();
    void Pause();

    [[deprecated("Use _songManager->LoadSearchCache instead")]]
    void SearchToList();//convert search result to list
    void Initialize();//init all things

    void ChangeLove(int index = -1);
    void SetVolume(double volume, bool level = false);
    void SetSliderPos(qint64 pos);
    void SentMsg(int type, QString msg);//sent msg, emit ShMsg

    qint64 GetSliderPos();
    QMediaPlayer* GetPlayer();


    //lambda
    [[deprecated("Use LoadMetaData instead")]]
    QList<QString> GetInfo(QMediaMetaData dat);//get info from metaData
    void DoFFTSpectrum(const float* data, int sampleCount);
signals:
    void PlayModeChanged(int mode);//update play mode btn
    void MetaDataLoaded(bool flag);
    void FreshTable(PlayListType type, QString listName = QString());//fresh table
    void LoadImgFinised();//set img
    void LoadLyricFinished();//set lyric
    void LoveStateUpdate(bool isLove);//update love btn
    void ShMsg(int type, QString msg);//show msg
    void HdMsg();//hide msg
    void SpectrumCalculated(double* bandArr, double total);//pass bandArr to visualiser

public slots:
    void SwitchNext();//switch to next song manually
    void SwitchPrev();//same as above
    void AutoSwitch(QMediaPlayer::MediaStatus status);//auto switch
    void PlayOrPause();//play to pause || pause to play
    void SwitchPlayMode();//switch play mode

    [[deprecated("Use LoadMetaData")]]
    void AfterLoad1(QMediaPlayer::MediaStatus status);//fresh metaData and calculate Hash, called while changing song
    [[deprecated("Use LoadVisualData")]]
    void AfterLoad2(bool flag);//fresh table, load and check all things, then play

    void LoadMetaData(QMediaPlayer::MediaStatus status);//load metaData
    void LoadVisualData(bool successful);//load visual data, emit LoadImgFinised, LoadLyricFinished

    void Search(QString keyword);//search
    void SearchBili(QString bid);//search bili
    
    void ClearLyric();//clear lyric(and lyricTime)

    void SpectrumToBand();//convert spectrum to band
private:
    //lambda
    [[deprecated("Job done by _ApplySettingsFromLoad()")]]
    void _DefaultSettings();//set default settings
    void _ApplySettingsFromLoad(bool mark);//apply settings from load
    QString _FileHash(QString path) const;
    const static char _sfc = 'K';//suffix for repeated name
};
#endif
 // PROCESSOBJ_H
