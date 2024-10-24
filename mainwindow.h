#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkReply>

#include <QMediaPlayer>
#include <QVideoWidget>
#include <QAudioOutput>
#include <QEventLoop>

#include "ElaWidget.h"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public ElaWidget
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();
    void Urlpost(QString fileName);
    void requestFinished(QNetworkReply* reply);

private:
    Ui::MainWindow *ui;
    QEventLoop loop;
    QMediaPlayer* m_player = nullptr;
    QAudioOutput* m_audioOutput = nullptr;
    QVideoWidget* m_videoOutput = nullptr;
};
#endif // MAINWINDOW_H
