#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QBuffer>
#include <QTimer>
#include <QFileDialog>
#include <QVideoSink>
#include <QVideoFrame>
#include <QStandardItemModel>
#include <QSettings>

MainWindow::MainWindow(QWidget *parent)
    : ElaWidget(parent)
    , ui(new Ui::MainWindow)
{
    //界面初始化
    ui->setupUi(this);
    this->setWindowTitle("ZcAIDanmuGenerator");
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    QStandardItemModel* model = new QStandardItemModel(ui->treeView_up);
    model->appendRow(new QStandardItem(QStringLiteral("生成")));
    model->appendRow(new QStandardItem(QStringLiteral("导出")));
    model->appendRow(new QStandardItem(QStringLiteral("设置")));
    ui->treeView_up->setModel(model);
    QModelIndex modelindex = ui->treeView_up->model()->index(0, 0);
    ui->treeView_up->setCurrentIndex(modelindex);
    //读取配置文件
    QSettings *settings = new QSettings(qApp->applicationDirPath()+"/Setting.ini",QSettings::IniFormat);
    ui->lineEdit_apikey->setText(settings->value("/apikey").toString());
    ui->lineEdit_prompt->setText(settings->value("/prompt").toString());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    QVideoSink* videoSink = new QVideoSink(this);
    /*视频截图*/
    m_player->setAudioOutput(m_audioOutput);
    m_player->setVideoOutput(videoSink);
    //加载视频文件
    QString str = QFileDialog::getOpenFileName();
    m_player->setSource(QUrl(str));
    m_player->play();
    //循环五秒
    for (int i = 0; i <= m_player->duration(); i += 7000)
    {
        m_player->setPosition(i); //使用局部事件循环等待帧捕获
        bool frameCaptured = false; //连接视频帧捕获的信号
        connect(videoSink, &QVideoSink::videoFrameChanged, this, [&](const QVideoFrame &frame) mutable {
            if (!frameCaptured && frame.isValid()) {
                frameCaptured = true; //第一次捕获后设置为 true
                QImage image = frame.toImage();
                ui->label_img->setPixmap(QPixmap::fromImage(image).scaled(ui->label_img->size(),Qt::KeepAspectRatio));
                if (!image.isNull()) {
                    //保存图像为 jpg
                    image.save(qApp->applicationDirPath()+"/temp/sc" + QString::number(i) + ".jpg", "JPG");
                    qDebug() << "Frame captured and saved.";
                    Urlpost(qApp->applicationDirPath()+"/temp/sc" + QString::number(i) + ".jpg");
                } else {
                    qDebug() << "Frame is not valid or conversion failed.";
                }
                //断开信号，停止等待
                disconnect(videoSink, &QVideoSink::videoFrameChanged, this, nullptr);

            }
        });
        // 开启局部事件循环，等待帧捕获完成
        loop.exec();
    }

}
void MainWindow::Urlpost(QString fileName)
{
    /*post请求*/
    qDebug()<<"post!";
    QNetworkRequest request;
    QNetworkAccessManager* naManager = new QNetworkAccessManager(this);
    QMetaObject::Connection connRet = QObject::connect(naManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(requestFinished(QNetworkReply*)));
    Q_ASSERT(connRet);
    //头设置
    request.setUrl(QUrl("https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));
    request.setRawHeader("Authorization","Basic "+ui->lineEdit_apikey->text().toLatin1());
    //data设置
    QJsonObject json_main;
    json_main.insert("model","qwen-vl-plus");
    QJsonArray json_message;
    QJsonObject json_messageobj;
    json_messageobj.insert("role","user");
    QJsonArray json_content;
    QJsonObject json_content1;
    json_content1.insert("type","text");
    json_content1.insert("text",ui->lineEdit_prompt->text());
    json_content.append(json_content1);
    QJsonObject json_content2;
    json_content2.insert("type","image_url");
    QJsonObject json_content2_img;
    QImage img;
    img.load(fileName);
    QByteArray ba;
    QBuffer buf(&ba);
    img.save(&buf, "png");
    buf.close();
    json_content2.insert("image_url","data:image/jpeg;base64,"+QString::fromUtf8(ba.toBase64()));
    json_content.append(json_content2);
    json_messageobj.insert("content",json_content);
    json_message.append(json_messageobj);
    json_main.insert("messages",json_message);
    QJsonDocument jsonDoc = QJsonDocument(json_main);
    QByteArray post_data = jsonDoc.toJson(QJsonDocument::Compact);
    //发送psot
    QNetworkReply* reply = naManager->post(request, post_data);
}
/*秒的转换*/
QString secondsToTimeString(double seconds) {

    // 提取整秒部分
    int totalSeconds = static_cast<int>(seconds);
    // 提取毫秒部分（注意，这里进行了放大以得到整数毫秒）
    int milliseconds = static_cast<int>((seconds - totalSeconds) * 1000);

    // 计算小时、分钟和剩余的秒
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int secs = totalSeconds % 60;

    // 使用QString进行格式化
    QString timeStr = QString("%1:%2:%3:%4")
                          .arg(hours, 2, 10, QChar('0'))
                          .arg(minutes, 2, 10, QChar('0'))
                          .arg(secs, 2, 10, QChar('0'))
                          .arg(milliseconds, 2, 10, QChar('0'));
    return timeStr;
}
/*接收post请求*/
int ii=0;
void MainWindow::requestFinished(QNetworkReply* reply) {

    QString str_reply = reply->readAll();
    qDebug()<<str_reply;
    /*reply数据处理*/
    //将QString转换为QByteArray，因为QJsonDocument::fromJson需要QByteArray作为输入
    QByteArray jsonBytes = str_reply.toUtf8();
    //解析JSON字符串
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonBytes);
    //获取根对象
    QJsonObject rootObj = jsonDoc.object();
    //获取choices数组
    QJsonArray choicesArray = rootObj["choices"].toArray();
    //获取第一个choice对象
    QJsonObject firstChoice = choicesArray.at(0).toObject();
    //获取message对象
    QJsonObject messageObj = firstChoice["message"].toObject();
    //获取content字符串
    QString content = messageObj["content"].toString();
    ui->plainTextEdit->setPlainText(content);
    loop.quit();  // 结束局部事件循环

    QStringList content_list = content.split(";");
    int x=0;
    for(int i=0;i!=content_list.size();i++)
    {
        x++;
        if (x>=6) x=0;
        ui->plainTextEdit_2->appendPlainText("Dialogue:0,"+
                                             secondsToTimeString(ii+i-1).replace(".",":")
                                             +","+
                                             secondsToTimeString(ii+10+i).replace(".",":")
                                             +",R2L,,20,20,2,,{\\move(585,"+
                                             QString::number(x*20)
                                             +",-25,"+
                                             QString::number(x*20)
                                             +")}"+
                                             content_list[i].replace(".","。").replace("\n","").replace("<","").replace(">",""));
    }
    ii+=5;
}

void MainWindow::on_treeView_up_clicked(const QModelIndex &index)
{
    ui->stackedWidget->setCurrentIndex(index.row());
}
/*apikey保存*/
void MainWindow::on_lineEdit_apikey_textChanged(const QString &arg1)
{
    QSettings *settings = new QSettings("Setting.ini",QSettings::IniFormat);
    settings->setValue("/apikey",arg1);
    delete settings;
}
/*提示词保存*/
void MainWindow::on_lineEdit_prompt_textChanged(const QString &arg1)
{
    QSettings *settings = new QSettings("Setting.ini",QSettings::IniFormat);
    settings->setValue("/prompt",arg1);
    delete settings;
}

