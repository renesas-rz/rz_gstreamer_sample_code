/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtMultimedia/QAudioProbe>
#include <QtMultimedia/QAudioRecorder>
#include <QtCore/QDir>
#include <QtWidgets/QFileDialog>
#include <QtMultimedia/QMediaRecorder>
#include "qt-audiorecorder.h"
#include "qaudiolevel.h"
#include "ui_qt-audiorecorder.h"

static qreal getPeakValue(const QAudioFormat &format);
static QVector<qreal> getBufferLevels(const QAudioBuffer &buffer);

// Constructor
template <class T>
static QVector<qreal> getBufferLevels(const T *buffer, int frames, int channels);
QString filenamerecorder;
AudioRecorder::AudioRecorder(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::AudioRecorder),
    outputLocationSet(false)
{
    ui->setupUi(this);
    audioRecorder = new QAudioRecorder(this);
    probe = new QAudioProbe;
    connect(probe, SIGNAL(audioBufferProbed(QAudioBuffer)),
            this, SLOT(processBuffer(QAudioBuffer)));
    probe->setSource(audioRecorder);

    //audio devices
    for (QString &device: audioRecorder->audioInputs()) {
        ui->audioDeviceBox->addItem(device, QVariant(device));
    }

    // Title of window
    this->setWindowTitle("Audio recorder version 1.0");
    connect(audioRecorder, SIGNAL(durationChanged(qint64)), this,
            SLOT(updateProgress(qint64)));
    connect(audioRecorder, SIGNAL(statusChanged(QMediaRecorder::Status)), this,
            SLOT(updateStatus(QMediaRecorder::Status)));
    connect(audioRecorder, SIGNAL(stateChanged(QMediaRecorder::State)),
            this, SLOT(onStateChanged(QMediaRecorder::State)));
    connect(audioRecorder, SIGNAL(error(QMediaRecorder::Error)), this,
            SLOT(displayErrorMessage()));
}

// Destructor
AudioRecorder::~AudioRecorder()
{
    delete audioRecorder;
    delete probe;
}

// Function helps update record progress - total record duration.
void AudioRecorder::updateProgress(qint64 duration)
{
    if (audioRecorder->error() != QMediaRecorder::NoError || duration < 2000)
        return;

    ui->statusbar->showMessage(tr("Recorded %1 sec").arg(duration / 1000));
}

// Funtion helps update record status include: Location and State ( pause/stop ).
void AudioRecorder::updateStatus(QMediaRecorder::Status status)
{
    QString statusMessage;


    switch (status) {
    case QMediaRecorder::RecordingStatus:
        filenamerecorder = tr("Recording to %1").arg(audioRecorder->actualLocation().toString());
        break;
    case QMediaRecorder::PausedStatus:
        clearAudioLevels();
        statusMessage = tr("Paused");
        break;
    case QMediaRecorder::UnloadedStatus:
    case QMediaRecorder::LoadedStatus:
        clearAudioLevels();
        statusMessage = tr("Stopped");
    default:
        break;
    }

    if (audioRecorder->error() == QMediaRecorder::NoError)
        ui->statusbar->showMessage(statusMessage);
    ui->statuslabel->setText(filenamerecorder);
}

// Function helps update application UI ( button ) base on received state
void AudioRecorder::onStateChanged(QMediaRecorder::State state)
{
    switch (state) {
    case QMediaRecorder::RecordingState:
        ui->recordButton->setText(tr("Stop"));
        ui->pauseButton->setText(tr("Pause"));
        break;
    case QMediaRecorder::PausedState:
        ui->recordButton->setText(tr("Stop"));
        ui->pauseButton->setText(tr("Resume"));
        break;
    case QMediaRecorder::StoppedState:
        ui->recordButton->setText(tr("Record"));
        ui->pauseButton->setText(tr("Pause"));
        break;
    }

    ui->pauseButton->setEnabled(audioRecorder->state() != QMediaRecorder::StoppedState);
}

// Return current value in box.
static QVariant boxValue (const QComboBox *box)
{
    int idx = box->currentIndex();
    if (idx == -1)
        return QVariant();

    return box->itemData(idx);
}

// Function helps toggle record state between RecordingState and StoppedState
void AudioRecorder::toggleRecord()
{
    if (audioRecorder->state() == QMediaRecorder::StoppedState) {
        audioRecorder->setAudioInput(boxValue(ui->audioDeviceBox).toString());
        QAudioEncoderSettings settings;
        settings.setBitRate(BIT_RATE);
        settings.setChannelCount(CHANEL_COUNT);
        settings.setCodec(RECORD_CODEC);
        QString container = RECORD_CONTAINER;
        audioRecorder->setEncodingSettings(settings, QVideoEncoderSettings(), container);
        audioRecorder->record();
    }
    else {
        audioRecorder->stop();
    }
}

// Function helps toggle record state between RecordingState and PausedState
void AudioRecorder::togglePause()
{
    if (audioRecorder->state() != QMediaRecorder::PausedState) {
        audioRecorder->pause();
    } else {
        audioRecorder->record();
    }
}

//
void AudioRecorder::setOutputLocation()
{
    QString fileName = QFileDialog::getSaveFileName();
    audioRecorder->setOutputLocation(QUrl::fromLocalFile(fileName));
    outputLocationSet = true;
}

// Function helps show error log when application's errors occur.
void AudioRecorder::displayErrorMessage()
{
    ui->statusbar->showMessage(audioRecorder->errorString());
}

// Function helps clear the audio level status bar - Filled with black color
void AudioRecorder::clearAudioLevels()
{
    for (int i = 0; i < audioLevels.size(); ++i)
        audioLevels.at(i)->setLevel(0);
}

// This function returns the maximum possible sample value for a given audio format
qreal getPeakValue(const QAudioFormat& format)
{
    // Note: Only the most common sample formats are supported
    if (!format.isValid())
        return qreal(0);
    if (format.codec() != FORTMAT_CODEC)
        return qreal(0);
    switch (format.sampleType()) {
    case QAudioFormat::Unknown:
        break;
    case QAudioFormat::Float:
        if (format.sampleSize() != SAMPLE_SIZE_32_BIT) // other sample formats are not supported
            return qreal(0);
        return qreal(1.00003);
    case QAudioFormat::SignedInt:
        if (format.sampleSize() == SAMPLE_SIZE_16_BIT)
            return qreal(SHRT_MAX);
        break;
    case QAudioFormat::UnSignedInt:
        if (format.sampleSize() == SAMPLE_SIZE_16_BIT)
            return qreal(USHRT_MAX);
        break;
    }

    return qreal(0);
}

// returns the audio level for each channel
QVector<qreal> getBufferLevels(const QAudioBuffer& buffer)
{
    QVector<qreal> values;
    if (!buffer.format().isValid() || buffer.format().byteOrder() != QAudioFormat::LittleEndian)
        return values;
    if (buffer.format().codec() != FORTMAT_CODEC)
        return values;
    int channelCount = buffer.format().channelCount();
    values.fill(0, channelCount);
    qreal peak_value = getPeakValue(buffer.format());
    if (qFuzzyCompare(peak_value, qreal(0)))
        return values;
    switch (buffer.format().sampleType()) {
    case QAudioFormat::Unknown:
    case QAudioFormat::UnSignedInt:
        if (buffer.format().sampleSize() == SAMPLE_SIZE_16_BIT)
            values = getBufferLevels(buffer.constData<quint16>(), buffer.frameCount(), channelCount);
        for (int i = 0; i < values.size(); ++i)
            values[i] = qAbs(values.at(i) - peak_value / 2) / (peak_value / 2);
        break;
    case QAudioFormat::Float:
        if (buffer.format().sampleSize() == SAMPLE_SIZE_32_BIT) {
            values = getBufferLevels(buffer.constData<float>(), buffer.frameCount(), channelCount);
            for (int i = 0; i < values.size(); ++i)
                values[i] /= peak_value;
        }
        break;
    case QAudioFormat::SignedInt:
        if (buffer.format().sampleSize() == SAMPLE_SIZE_16_BIT)
            values = getBufferLevels(buffer.constData<qint16>(), buffer.frameCount(), channelCount);
        for (int i = 0; i < values.size(); ++i)
            values[i] /= peak_value;
        break;
    }
    return values;
}

template <class T>
QVector<qreal> getBufferLevels(const T *buffer, int frames, int channels)
{
    QVector<qreal> max_values;
    max_values.fill(0, channels);
    for (int i = 0; i < frames; ++i) {
        for (int j = 0; j < channels; ++j) {
            qreal value = qAbs(qreal(buffer[i * channels + j]));
            if (value > max_values.at(j))
                max_values.replace(j, value);
        }
    }
    return max_values;
}

// Function helps update audio level whenever signal audioBufferProbed emits
void AudioRecorder::processBuffer(const QAudioBuffer& buffer)
{
    if (audioLevels.count() != buffer.format().channelCount()) {
        qDeleteAll(audioLevels);
        audioLevels.clear();
        for (int i = 0; i < buffer.format().channelCount(); ++i) {
            QAudioLevel *level = new QAudioLevel(ui->centralwidget);
            audioLevels.append(level);
            ui->levelsLayout->addWidget(level);
        }
    }
    QVector<qreal> levels = getBufferLevels(buffer);
    for (int i = 0; i < levels.count(); ++i)
        audioLevels.at(i)->setLevel(levels.at(i));
}

// Event handler for button Quit, helps close the application
void AudioRecorder::on_exitButton_clicked()
{
    close();
}
