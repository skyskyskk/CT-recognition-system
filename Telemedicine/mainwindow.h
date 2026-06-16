// 远程医疗诊断系统主窗口头文件
// 包含主窗口类定义、自定义控件类和数据库连接函数

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
#include<QSqlError>

// OpenCV 命名空间
using namespace cv;
// 标准命名空间
using namespace std;

namespace Ui {
class MainWindow;
}

// 可点击的标签控件，用于照片上传功能
// 继承自 QLabel，重写鼠标点击事件以发出 clicked 信号
class ClickableLabel : public QLabel {
    Q_OBJECT
public:
    // 构造函数，parent为父窗口指针
    explicit ClickableLabel(QWidget *parent = nullptr) : QLabel(parent) {}

signals:
    // 点击信号，当用户点击标签时发出此信号
    void clicked();

protected:
    // 鼠标点击事件处理函数，event为鼠标事件对象
    void mousePressEvent(QMouseEvent *event) override {
        emit clicked();
        QLabel::mousePressEvent(event);
    }
};

// 主窗口类，系统的核心界面和功能入口
// 包含患者信息管理、CT影像处理、数据库操作等所有主要功能
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    // 构造函数，parent为父窗口指针
    explicit MainWindow(QWidget *parent = nullptr);

    // 析构函数
    ~MainWindow();

    // 初始化主窗口：设置UI样式、初始化控件、建立信号槽连接
    void initMainWindow();

    // 显示CT影像：将处理后的CT影像显示到界面上
    void ctImgShow();

    // 表格选择变化处理函数，row为选中的行号
    // 当用户在表格中选择不同患者时更新右侧详细信息
    void onTableSelectChange(int row);

    // 读取CT影像：从文件或数据库中读取CT影像数据
    void ctImgRead();

    // 处理CT影像：对CT影像进行预处理、增强、分割等操作
    void ctImgProc();

    // 保存CT影像：将处理后的CT影像保存到文件或数据库
    void ctImgSave();

    // Hough圆检测：使用Hough变换检测CT影像中的圆形病灶
    void ctImgHoughCircles();

    // 显示用户照片：从数据库中读取患者照片并显示到界面上
    void showUserPhoto();

protected:
    // 窗口大小变化事件处理函数，event为大小变化事件对象
    // 实现窗口等比缩放功能
    void resizeEvent(QResizeEvent *event) override;

private slots:
    // 定时器超时处理函数：用于定时更新CT影像或执行周期性任务
    void onTimeOut();

    // 开始按钮点击处理函数：启动CT影像处理流程
    void on_startPushButton_clicked();

    // 基本信息表格点击处理函数，index为点击的模型索引
    // 处理用户在基本信息表格中的点击事件
    void on_basicTableView_clicked(const QModelIndex &index);

    // 标签页切换处理函数，index为切换到的标签页索引
    // 处理用户切换标签页的事件
    void on_tabWidget_tabBarClicked(int index);

    // 新增的槽函数
    // 新增患者：弹出对话框输入患者信息并插入到数据库
    void addPatient();

    // 保存修改：将编辑后的患者信息保存到数据库
    void savePatient();

    // 删除患者：删除当前选中的患者记录
    void deletePatient();

    // 保存病历：将病历文本保存到数据库
    void saveCaseHistory();

    // 上传照片：处理用户上传患者照片的请求
    void uploadPhoto();

private:
    Ui::MainWindow *ui;                // UI对象指针
    Mat myCtImg;                       // 原始CT影像
    Mat myCtGrayImg;                   // 灰度化后的CT影像
    QImage myCtQImage;                 // 用于显示的QImage格式CT影像
    QSqlTableModel *model;             // 基本信息表格模型
    QSqlTableModel *model_d;           // 详细信息表格模型
    QTimer *myTimer;                   // 定时器对象

    ClickableLabel *clickablePhotoLabel; // 可点击的照片标签

    // 窗口等比缩放相关成员
    QSize m_baseSize;                  // 窗口基准大小
    QMap<QWidget*, QRect> m_originalRects; // 控件原始位置和大小
    QMap<QWidget*, int> m_originalFontSizes; // 控件原始字体大小

    // 记录控件原始几何信息，parent为父窗口指针
    // 递归记录所有子控件的原始位置、大小和字体大小
    void recordGeometries(QWidget* parent);
};

// 创建MySQL数据库连接
// 连接成功返回true，失败返回false
// 初始化数据库连接，设置连接参数并打开连接
static bool createMySqlConn()
{
    if (!QSqlDatabase::isDriverAvailable("QMYSQL")) {
        QMessageBox::critical(nullptr, "驱动错误",
                             "MySQL驱动未找到！请确保qsqlmysql.dll在sqldrivers目录下");
        return false;
    }

    // 重试时先移除旧连接，避免重复添加连接警告
    if (QSqlDatabase::contains()) {
        QSqlDatabase::database().close();
        QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
    }

    QSqlDatabase sqldb = QSqlDatabase::addDatabase("QMYSQL");
    sqldb.setHostName("127.0.0.1");
    sqldb.setPort(3306);
    sqldb.setDatabaseName("patient");
    sqldb.setUserName("root");
    sqldb.setPassword("123456");

    if (!sqldb.open()) {
        QMessageBox::critical(nullptr, "数据库连接失败",
            QString("无法连接数据库！\n错误：%1\n\n请检查MySQL服务是否启动")
            .arg(sqldb.lastError().text()));
        return false;
    }
    return true;
}

#endif // MAINWINDOW_H
