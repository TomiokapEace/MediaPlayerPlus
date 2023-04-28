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

#include <StreamInfo.hpp>

extern "C" {
    #include <libavutil/pixdesc.h>
}

#include <QDebug>

QMPlay2Tags StreamInfo::getTag(const QString &tag)
{
    bool ok;
    const int tagID = tag.toInt(&ok);
    if (ok && tagID >= QMPLAY2_TAG_LANGUAGE && tagID <= QMPLAY2_TAG_LYRICS)
        return (QMPlay2Tags)tagID;
    return QMPLAY2_TAG_UNKNOWN;
}
QString StreamInfo::getTagName(const QString &tag)
{
    const QMPlay2Tags tagID = getTag(tag);
    switch (tagID)
    {
        case QMPLAY2_TAG_LANGUAGE:
            return tr("Language");
        case QMPLAY2_TAG_TITLE:
            return tr("Title");
        case QMPLAY2_TAG_ARTIST:
            return tr("Artist");
        case QMPLAY2_TAG_ALBUM:
            return tr("Album");
        case QMPLAY2_TAG_GENRE:
            return tr("Genre");
        case QMPLAY2_TAG_DATE:
            return tr("Date");
        case QMPLAY2_TAG_COMMENT:
            return tr("Comment");
        case QMPLAY2_TAG_LYRICS:
            return tr("Lyrics");
        default:
            return tag;
    }
}

StreamInfo::StreamInfo()
{
    params = avcodec_parameters_alloc();
    params->format = -1;
    resetSAR();
}
StreamInfo::StreamInfo(AVCodecParameters *codecpar)
    : StreamInfo()
{
    avcodec_parameters_copy(params, codecpar);

    if (const AVCodec *codec = avcodec_find_decoder(params->codec_id))
        codec_name = codec->name;

    if (params->sample_aspect_ratio.num == 0)
        resetSAR();
}
StreamInfo::StreamInfo(quint32 sampleRateArg, quint8 channelsArg)
    : StreamInfo()
{
    params->codec_type = AVMEDIA_TYPE_AUDIO;
    params->sample_rate = sampleRateArg;
    params->channels = channelsArg;
}
StreamInfo::~StreamInfo()
{
    avcodec_parameters_free(&params);
}

QByteArray StreamInfo::getFormatName() const
{
    switch (params->codec_type)
    {
        case AVMEDIA_TYPE_AUDIO:
            return av_get_sample_fmt_name(sampleFormat());
        case AVMEDIA_TYPE_VIDEO:
            return av_get_pix_fmt_name(pixelFormat());
        default:
            break;
    }
    return QByteArray();
}
void StreamInfo::setFormat(int newFormat)
{
    params->format = newFormat;
}

inline void StreamInfo::resetSAR()
{
    params->sample_aspect_ratio.num = params->sample_aspect_ratio.den = 1;
}
