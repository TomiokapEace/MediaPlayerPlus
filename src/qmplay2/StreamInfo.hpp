/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <QMPlay2Lib.hpp>

#include <QCoreApplication>
#include <QByteArray>
#include <QVector>
#include <QPair>

extern "C" {
    #include <libavcodec/avcodec.h>
}

using QMPlay2Tag = QPair<QString, QString>;

enum QMPlay2Tags
{
    QMPLAY2_TAG_UNKNOWN = -1,
    QMPLAY2_TAG_NAME, //should be as first
    QMPLAY2_TAG_DESCRIPTION, //should be as second
    QMPLAY2_TAG_LANGUAGE,
    QMPLAY2_TAG_TITLE,
    QMPLAY2_TAG_ARTIST,
    QMPLAY2_TAG_ALBUM,
    QMPLAY2_TAG_GENRE,
    QMPLAY2_TAG_DATE,
    QMPLAY2_TAG_COMMENT,
    QMPLAY2_TAG_LYRICS,
};

class QMPLAY2SHAREDLIB_EXPORT StreamInfo
{
    Q_DECLARE_TR_FUNCTIONS(StreamInfo)
    Q_DISABLE_COPY(StreamInfo)

public:
    static QMPlay2Tags getTag(const QString &tag);
    static QString getTagName(const QString &tag);

    StreamInfo();
    StreamInfo(AVCodecParameters *codecpar);
    StreamInfo(quint32 sampleRateArg, quint8 channelsArg);
    ~StreamInfo();

    inline double getSampleAspectRatio() const
    {
        return av_q2d(params->sample_aspect_ratio);
    }

    inline double getAspectRatio() const
    {
        return getSampleAspectRatio() * params->width / params->height;
    }

    inline double getFPS() const
    {
        return av_q2d(fps);
    }

    inline int getExtraDataCapacity() const
    {
        return params->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE;
    }
    inline const QByteArray getExtraData() const
    {
        return QByteArray::fromRawData((const char *)params->extradata, params->extradata_size);
    }

    inline AVPixelFormat pixelFormat() const
    {
        return static_cast<AVPixelFormat>(params->format);
    }
    inline AVSampleFormat sampleFormat() const
    {
        return static_cast<AVSampleFormat>(params->format);
    }

    QByteArray getFormatName() const;
    void setFormat(int newFormat);

    QByteArray codec_name, title, artist;
    QByteArray codec_name_backup;
    QVector<QMPlay2Tag> other_info;
    bool is_default = true;
    bool must_decode = false;
    AVRational time_base = {1, 10000};

    // Video only
    AVRational fps = {0, 1};
    double rotation = qQNaN();
    bool spherical = false;
    bool custom_sar = false;

    AVCodecParameters *params = nullptr;

private:
    inline void resetSAR();
};
