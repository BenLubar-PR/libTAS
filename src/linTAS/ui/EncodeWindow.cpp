/*
    Copyright 2015-2016 Clément Gallet <clement.gallet@ens-lyon.org>

    This file is part of libTAS.

    libTAS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    libTAS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libTAS.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef LIBTAS_ENABLE_AVDUMPING

#include <QLabel>
#include <QFileDialog>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>

#include "EncodeWindow.h"

#include <iostream>
extern "C" {
#include <libavcodec/avcodec.h>
}

EncodeWindow::EncodeWindow(Context* c, QWidget *parent, Qt::WindowFlags flags) : QDialog(parent, flags), context(c)
{
    setFixedSize(600, 260);
    setWindowTitle("Encoding configuration");

    // window = new Fl_Double_Window(600, 260, "Encoding configuration");



    /* Game Executable */
    // QLabel* encodeLabel = new QLabel("Encode file path", encodePath);


    encodePath = new QLineEdit();
    encodePath->setReadOnly(true);

    browseEncodePath = new QPushButton("Browse...");
    connect(browseEncodePath, &QAbstractButton::clicked, this, &EncodeWindow::slotBrowseEncodePath);

    QGroupBox *encodeFileGroupBox = new QGroupBox(tr("Encode file path"));
    QHBoxLayout *encodeFileLayout = new QHBoxLayout;
    encodeFileLayout->addWidget(encodePath);
    encodeFileLayout->addWidget(browseEncodePath);
    encodeFileGroupBox->setLayout(encodeFileLayout);


    // videoChoice = new QComboBox(10, 100, 450, 30, "Video codec");
    videoChoice = new QComboBox();
    connect(videoChoice, QOverload<int>::of(&QComboBox::activated), this, &EncodeWindow::slotVideoCodec);

    // videobitrate = new Fl_Input(480, 100, 100, 30, "Video bitrate");
    // videobitrate->align(FL_ALIGN_TOP_LEFT);
    videoBitrate = new QSpinBox();

    // audiochoice = new Fl_Choice(10, 160, 450, 30, "Audio codec");
    // audiochoice->align(FL_ALIGN_TOP_LEFT);
    // audiochoice->callback(acodec_cb, this);
    audioChoice = new QComboBox();
    connect(audioChoice, QOverload<int>::of(&QComboBox::activated), this, &EncodeWindow::slotAudioCodec);

    // audiobitrate = new Fl_Input(480, 160, 100, 30, "Audio bitrate");
    // audiobitrate->align(FL_ALIGN_TOP_LEFT);
    audioBitrate = new QSpinBox();

    QGroupBox *codecGroupBox = new QGroupBox(tr("Encode codec settings"));
    QGridLayout *encodeCodecLayout = new QGridLayout;
    encodeCodecLayout->addWidget(new QLabel(tr("Video codec")), 0, 0);
    encodeCodecLayout->addWidget(videoChoice, 0, 1);
    encodeCodecLayout->addWidget(new QLabel(tr("Video bitrate")), 0, 2);
    encodeCodecLayout->addWidget(videoBitrate, 0, 3);
    encodeCodecLayout->addWidget(new QLabel(tr("Audio codec")), 1, 0);
    encodeCodecLayout->addWidget(audioChoice, 1, 1);
    encodeCodecLayout->addWidget(new QLabel(tr("Audio bitrate")), 1, 2);
    encodeCodecLayout->addWidget(audioBitrate, 1, 3);
    codecGroupBox->setLayout(encodeCodecLayout);


    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &EncodeWindow::slotOk);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &EncodeWindow::slotCancel);

    /* Create the main layout */
    QVBoxLayout *mainLayout = new QVBoxLayout;

    mainLayout->addWidget(encodeFileGroupBox);
    mainLayout->addWidget(codecGroupBox);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);


    /* Get all encoding codecs available and fill the codec menus */

    /* Initialize libavcodec, and register all codecs and formats */
    //av_register_all();
    avcodec_register_all();

    /* Enumerate the codecs */
    AVCodec* codec = av_codec_next(nullptr);

    while(codec != nullptr)
    {
        if (av_codec_is_encoder(codec)) {
            /* Codec supports encoding */

            /* Build codec name */
            std::string codecstr = codec->long_name?codec->long_name:codec->name;

            /* Escape some characters that have a special meaning in FLTK */
            for (std::string::size_type i = 0; (i = codecstr.find('/', i)) != std::string::npos;) {
                codecstr.insert(i, "\\");
                i += 2;
            }

            if (codec->type == AVMEDIA_TYPE_VIDEO) {
                videoChoice->addItem(codecstr.c_str(), QVariant(codec->id));
            }
            if (codec->type == AVMEDIA_TYPE_AUDIO) {
                audioChoice->addItem(codecstr.c_str(), QVariant(codec->id));
            }
        }
        codec = av_codec_next(codec);
    }

    update_config();
}

void EncodeWindow::update_config()
{
    if (context->config.dumpfile.empty()) {
        encodePath->setText(context->gamepath.c_str());
    }
    else {
        encodePath->setText(context->config.dumpfile.c_str());
    }

    /* Browse the list of video codecs and select the item that matches
     * the value in the config using the item's user data.
     */
    for (int c = 0; c < videoChoice->count(); c++) {
        if (static_cast<AVCodecID>(videoChoice->itemData(c).toInt()) == context->config.sc.video_codec) {
            videoChoice->setCurrentIndex(c);
            break;
        }
    }

    /* Enable/disable video bitrate for lossy/lossless codecs */
    const AVCodecDescriptor* vcodec = avcodec_descriptor_get(context->config.sc.video_codec);
    if ((vcodec->props & AV_CODEC_PROP_LOSSLESS) && !(vcodec->props & AV_CODEC_PROP_LOSSY)) {
        videoBitrate->setEnabled(false);
    }
    else {
        videoBitrate->setEnabled(true);
    }

    /* Set video bitrate */
    videoBitrate->setValue(context->config.sc.video_bitrate);

    /* Same for audio codec and bitrate */
    for (int c = 0; c < audioChoice->count(); c++) {
        if (static_cast<AVCodecID>(audioChoice->itemData(c).toInt()) == context->config.sc.audio_codec) {
            audioChoice->setCurrentIndex(c);
            break;
        }
    }

    /* Enable/disable audio bitrate for lossy/lossless codecs */
    const AVCodecDescriptor* acodec = avcodec_descriptor_get(context->config.sc.audio_codec);
    if ((acodec->props & AV_CODEC_PROP_LOSSLESS) && !(acodec->props & AV_CODEC_PROP_LOSSY)) {
        audioBitrate->setEnabled(false);
    }
    else {
        audioBitrate->setEnabled(true);
    }

    /* Set audio bitrate */
    audioBitrate->setValue(context->config.sc.audio_bitrate);
}

void EncodeWindow::slotOk()
{
    /* Fill encode filename */
    context->config.dumpfile = encodePath->text().toStdString();
    context->config.dumpfile_modified = true;

    /* Set video codec and bitrate */
    context->config.sc.video_codec = static_cast<AVCodecID>(videoChoice->currentData().toInt());
    context->config.sc.video_bitrate = videoBitrate->value();

    /* Set audio codec and bitrate */
    context->config.sc.audio_codec = static_cast<AVCodecID>(audioChoice->currentData().toInt());
    context->config.sc.audio_bitrate = audioBitrate->value();

    context->config.sc_modified = true;

    /* Close window */
    accept();
}

void EncodeWindow::slotCancel()
{
    /* Close window */
    reject();
}

void EncodeWindow::slotBrowseEncodePath(bool checked)
{
    if (! checked)
        return;

    QString filename = QFileDialog::getSaveFileName(this, tr("Choose an encode filename"), encodePath->text());
    encodePath->setText(filename);
}

void EncodeWindow::slotVideoCodec(int index)
{
    /* Enable/disable video bitrate for lossy/lossless codecs */
    const AVCodecDescriptor* vcodec = avcodec_descriptor_get(static_cast<AVCodecID>(videoChoice->itemData(index).toInt()));
    if ((vcodec->props & AV_CODEC_PROP_LOSSLESS) && !(vcodec->props & AV_CODEC_PROP_LOSSY)) {
        videoBitrate->setEnabled(false);
    }
    else {
        videoBitrate->setEnabled(true);
    }
}

void EncodeWindow::slotAudioCodec(int index)
{
    /* Enable/disable audio bitrate for lossy/lossless codecs */
    const AVCodecDescriptor* acodec = avcodec_descriptor_get(static_cast<AVCodecID>(audioChoice->itemData(index).toInt()));
    if ((acodec->props & AV_CODEC_PROP_LOSSLESS) && !(acodec->props & AV_CODEC_PROP_LOSSY)) {
        audioBitrate->setEnabled(false);
    }
    else {
        audioBitrate->setEnabled(true);
    }
}

#endif
