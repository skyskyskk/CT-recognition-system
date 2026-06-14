#include "mainwindow.h"
#include <QApplication>
#include <QProcess>
#include <QMessageBox>
#include <QDebug>
#include <QThread>
#include <QSqlDatabase>
#include <QSqlError>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 先检查驱动
    qDebug() << "可用SQL驱动：" << QSqlDatabase::drivers();

    if(!createMySqlConn()){
        qDebug() << "首次连接失败，尝试启动MySQL服务...";

        // 启动MySQL服务
        QProcess process;
        process.start("net", QStringList() << "start" << "MySQL80");

        // 等待服务启动
        if(!process.waitForStarted(3000)) {
            QMessageBox::critical(nullptr, "错误", "无法启动MySQL服务！");
            return 1;
        }

        process.waitForFinished(5000);

        // 等待MySQL完全启动
        QThread::sleep(3);

        // 再次尝试连接
        if(!createMySqlConn()) {
            QMessageBox::critical(nullptr, "数据库错误",
                                 "MySQL连接失败，请检查MySQL80服务是否已启动！\n"
                                 "或者检查数据库配置是否正确（用户名：root，密码：123456）");
            return 1;
        }
    }

    MainWindow w;
    w.show();
    return a.exec();
}
