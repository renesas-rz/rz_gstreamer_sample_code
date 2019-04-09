/********************************************************************************
** Form generated from reading UI file 'qt-audiorecorder.ui'
**
** Created by: Qt User Interface Compiler version 5.6.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_AUDIORECORDER_H
#define UI_AUDIORECORDER_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
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
    QPushButton *exitButton;
    QLabel *levelLabel;
    QVBoxLayout *levelsLayout;
    QPushButton *recordButton;
    QPushButton *pauseButton;
    QLabel *statuslabel;
    QStatusBar *statusbar;

    // Setup Ui elements include buttons, text boxes, status bars
    void setupUi(QMainWindow *AudioRecorder)
    {
        // Setup UI properties
        if (AudioRecorder->objectName().isEmpty())
            AudioRecorder->setObjectName(QStringLiteral("AudioRecorder"));
        AudioRecorder->resize(505, 176);
        centralwidget = new QWidget(AudioRecorder);
        centralwidget->setObjectName(QStringLiteral("centralwidget"));
        gridLayout_3 = new QGridLayout(centralwidget);
        gridLayout_3->setObjectName(QStringLiteral("gridLayout_3"));
        exitButton = new QPushButton(centralwidget);
        exitButton->setObjectName(QStringLiteral("exitButton"));

        gridLayout_3->addWidget(exitButton, 0, 2, 1, 1);

        levelLabel = new QLabel(centralwidget);
        levelLabel->setObjectName(QStringLiteral("levelLabel"));

        gridLayout_3->addWidget(levelLabel, 1, 0, 1, 1);

        levelsLayout = new QVBoxLayout();
        levelsLayout->setObjectName(QStringLiteral("levelsLayout"));

        gridLayout_3->addLayout(levelsLayout, 1, 1, 1, 2);

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

        AudioRecorder->setCentralWidget(centralwidget);
        statusbar = new QStatusBar(AudioRecorder);
        statusbar->setObjectName(QStringLiteral("statusbar"));
        AudioRecorder->setStatusBar(statusbar);

        // Connect UI slots for catching signal
        retranslateUi(AudioRecorder);
        QObject::connect(recordButton, SIGNAL(clicked()), AudioRecorder, SLOT(toggleRecord()));
        QObject::connect(pauseButton, SIGNAL(clicked()), AudioRecorder, SLOT(togglePause()));

        QMetaObject::connectSlotsByName(AudioRecorder);
    } // setupUi

    void retranslateUi(QMainWindow *AudioRecorder)
    {
        AudioRecorder->setWindowTitle(QApplication::translate("AudioRecorder", "MainWindow", 0));
        exitButton->setText(QApplication::translate("AudioRecorder", "Quit", 0));
        levelLabel->setText(QApplication::translate("AudioRecorder", "Audio Signal:", 0));
        recordButton->setText(QApplication::translate("AudioRecorder", "Record", 0));
        pauseButton->setText(QApplication::translate("AudioRecorder", "Pause", 0));
        statuslabel->setText(QString());
    } // retranslateUi - Assign text data for elements

};

namespace Ui {
    class AudioRecorder: public Ui_AudioRecorder {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_AUDIORECORDER_H
