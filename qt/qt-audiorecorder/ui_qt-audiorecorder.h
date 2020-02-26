/********************************************************************************
** Form generated from reading UI file 'qt-audiorecorder.ui'
**
** Created by: Qt User Interface Compiler version 5.6.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_QT_2D_AUDIORECORDER_H
#define UI_QT_2D_AUDIORECORDER_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_AudioRecorder
{
public:
    QWidget *centralwidget;
    QGridLayout *gridLayout_3;
    QLabel *levelLabel;
    QVBoxLayout *levelsLayout;
    QLabel *label;
    QPushButton *recordButton;
    QPushButton *pauseButton;
    QLabel *statuslabel;
    QPushButton *exitButton;
    QComboBox *audioDeviceBox;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *AudioRecorder)
    {
        if (AudioRecorder->objectName().isEmpty())
            AudioRecorder->setObjectName(QStringLiteral("AudioRecorder"));
        AudioRecorder->resize(505, 197);
        centralwidget = new QWidget(AudioRecorder);
        centralwidget->setObjectName(QStringLiteral("centralwidget"));
        gridLayout_3 = new QGridLayout(centralwidget);
        gridLayout_3->setObjectName(QStringLiteral("gridLayout_3"));
        levelLabel = new QLabel(centralwidget);
        levelLabel->setObjectName(QStringLiteral("levelLabel"));

        gridLayout_3->addWidget(levelLabel, 1, 0, 1, 1);

        levelsLayout = new QVBoxLayout();
        levelsLayout->setSpacing(8);
        levelsLayout->setObjectName(QStringLiteral("levelsLayout"));

        gridLayout_3->addLayout(levelsLayout, 1, 1, 1, 2);

        label = new QLabel(centralwidget);
        label->setObjectName(QStringLiteral("label"));

        gridLayout_3->addWidget(label, 4, 0, 1, 1);

        recordButton = new QPushButton(centralwidget);
        recordButton->setObjectName(QStringLiteral("recordButton"));

        gridLayout_3->addWidget(recordButton, 0, 0, 1, 1);

        pauseButton = new QPushButton(centralwidget);
        pauseButton->setObjectName(QStringLiteral("pauseButton"));
        pauseButton->setEnabled(false);

        gridLayout_3->addWidget(pauseButton, 0, 1, 1, 1);

        statuslabel = new QLabel(centralwidget);
        statuslabel->setObjectName(QStringLiteral("statuslabel"));

        gridLayout_3->addWidget(statuslabel, 2, 0, 1, 2);

        exitButton = new QPushButton(centralwidget);
        exitButton->setObjectName(QStringLiteral("exitButton"));

        gridLayout_3->addWidget(exitButton, 0, 2, 1, 1);

        audioDeviceBox = new QComboBox(centralwidget);
        audioDeviceBox->setObjectName(QStringLiteral("audioDeviceBox"));

        gridLayout_3->addWidget(audioDeviceBox, 5, 0, 1, 1);

        AudioRecorder->setCentralWidget(centralwidget);
        statusbar = new QStatusBar(AudioRecorder);
        statusbar->setObjectName(QStringLiteral("statusbar"));
        AudioRecorder->setStatusBar(statusbar);

        retranslateUi(AudioRecorder);
        QObject::connect(recordButton, SIGNAL(clicked()), AudioRecorder, SLOT(toggleRecord()));
        QObject::connect(pauseButton, SIGNAL(clicked()), AudioRecorder, SLOT(togglePause()));

        QMetaObject::connectSlotsByName(AudioRecorder);
    } // setupUi

    void retranslateUi(QMainWindow *AudioRecorder)
    {
        AudioRecorder->setWindowTitle(QApplication::translate("AudioRecorder", "MainWindow", Q_NULLPTR));
        levelLabel->setText(QApplication::translate("AudioRecorder", "Audio Signal:", Q_NULLPTR));
        label->setText(QApplication::translate("AudioRecorder", "Device", Q_NULLPTR));
        recordButton->setText(QApplication::translate("AudioRecorder", "Record", Q_NULLPTR));
        pauseButton->setText(QApplication::translate("AudioRecorder", "Pause", Q_NULLPTR));
        statuslabel->setText(QString());
        exitButton->setText(QApplication::translate("AudioRecorder", "Quit", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class AudioRecorder: public Ui_AudioRecorder {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_QT_2D_AUDIORECORDER_H
