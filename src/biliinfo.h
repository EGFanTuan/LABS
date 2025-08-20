#pragma once
#include <QObject>
#include <QVector>

class PartInfo{
public:
    explicit PartInfo(): part(""), first_frame(""), cid(0), page(0) {

    }

public:
    QString part, first_frame, hash;
    qint64 cid, page;
};

class BiliInfo{
public:
    explicit BiliInfo(): bvid(""), pic(""), title(""), desc(""), picPath(""), videos(0), cid(0) {

    }

public:
    // video info
    QString bvid, pic, title, desc, picPath;
    qint64 videos, cid;
    // parts
    QVector<PartInfo> parts;
};

