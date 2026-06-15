#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>
#include <QMessageBox>
#include <vector>
#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <QSqlTableModel>
#include <QSqlQuery>
#include <QTimer>
#include <QDebug>
#include <QPushButton>
#include <QLabel>
#include <QMouseEvent>
#include <QEvent>
#include <QMap>

using namespace cv;
using namespace std;

namespace Ui {
class MainWindow;
}

// 可点击的 Label，用于照片上传
class ClickableLabel : public QLabel {
    Q_OBJECT
public:
    explicit ClickableLabel(QWidget *parent = nullptr) : QLabel(parent) {}
signals:
    void clicked();
protected:
    void mousePressEvent(QMouseEvent *event) override {
        emit clicked();
        QLabel::mousePressEvent(event);
    }
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void initMainWindow();
    void ctImgShow();
    void onTableSelectChange(int row);
    void ctImgRead();
    void ctImgProc();
    void ctImgSave();
    void ctImgHoughCircles();
    void showUserPhoto();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onTimeOut();
    void on_startPushButton_clicked();
    void on_basicTableView_clicked(const QModelIndex &index);
    void on_tabWidget_tabBarClicked(int index);

    // 新增的槽函数
    void addPatient();
    void savePatient();
    void deletePatient();
    void saveCaseHistory();
    void uploadPhoto();

private:
    Ui::MainWindow *ui;
    Mat myCtImg;
    Mat myCtGrayImg;
    QImage myCtQImage;
    QSqlTableModel *model;
    QSqlTableModel *model_d;
    QTimer *myTimer;

    ClickableLabel *clickablePhotoLabel;

    // 窗口等比缩放
    QSize m_baseSize;
    QMap<QWidget*, QRect> m_originalRects;
    QMap<QWidget*, int> m_originalFontSizes;
    void recordGeometries(QWidget* parent);
};

#include <QDebug>
#include <QSqlError>
static bool createMySqlConn()
{
    // 检查驱动
    if(!QSqlDatabase::isDriverAvailable("QMYSQL")) {
        qDebug() << "QMYSQL驱动不可用！";
        QMessageBox::critical(nullptr, "驱动错误",
                             "MySQL驱动未找到！\n"
                             "请确保qsqlmysql.dll在 plugins/sqldrivers/ 目录下");
        return false;
    }

    QSqlDatabase sqldb = QSqlDatabase::addDatabase("QMYSQL");
    sqldb.setHostName("127.0.0.1");
    sqldb.setPort(3306);
    sqldb.setDatabaseName("patient");
    sqldb.setUserName("root");
    sqldb.setPassword("123456");

    // 不使用任何连接选项
    // sqldb.setConnectOptions(...); // 注释掉这一行

    if(!sqldb.open())
    {
        qDebug() << "数据库错误：" << sqldb.lastError().text();

        // 打印更详细的错误信息
        QMessageBox::critical(0,
            u8"后台数据库连接失败",
            QString(u8"无法创建连接！\n错误详情：%1\n\n请检查：\n1. MySQL服务是否启动\n2. 数据库名是否为patient\n3. 密码是否正确")
            .arg(sqldb.lastError().text()),
            QMessageBox::Cancel);
        return false;
    }

    qDebug() << "数据库连接成功！";
    return true;
}
#endif // MAINWINDOW_H
