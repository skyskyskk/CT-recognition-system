// 远程医疗诊断系统主程序入口
// 负责初始化应用程序、建立数据库连接、启动主窗口

#include "mainwindow.h"
#include <QApplication>
#include <QProcess>
#include <QMessageBox>
#include <QDebug>
#include <QThread>
#include <QSqlDatabase>
#include <QSqlError>

// 主函数：程序入口点
// argc为命令行参数个数，argv为命令行参数数组
// 返回程序退出码
int main(int argc, char *argv[])
{
    // 注释：界面缩放
    // qputenv("QT_SCALE_FACTOR", "1.5");
    
    // 初始化Qt应用程序
    QApplication a(argc, argv);

    // 尝试连接数据库，失败则自动启动MySQL服务后重试
    if (!createMySqlConn()) {
        QProcess process;
        process.start("net", QStringList() << "start" << "MySQL80");
        process.waitForFinished(5000);
        QThread::sleep(3);
        if (!createMySqlConn()) {
            QMessageBox::critical(nullptr, "数据库错误",
                                 "MySQL连接失败，请检查MySQL80服务是否已启动！");
            return 1;
        }
    }

    // 创建并显示主窗口
    MainWindow w;
    w.show();
    
    // 进入Qt事件循环
    return a.exec();
}
