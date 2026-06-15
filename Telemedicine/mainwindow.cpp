#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QBuffer>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFile>
#include <QStatusBar>
#include <QMenuBar>
#include <QToolBar>
#include <QElapsedTimer>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
     resize(1580, 1200);
    initMainWindow();

    // 加载 QSS 样式表
    QFile qssFile(QCoreApplication::applicationDirPath() + "/style.qss");
    if (qssFile.open(QFile::ReadOnly | QFile::Text)) {
        qApp->setStyleSheet(qssFile.readAll());
        qssFile.close();
    }

    // 创建可点击的照片 Label（替换原有的 photoLabel）
    clickablePhotoLabel = new ClickableLabel(ui->frame);
    clickablePhotoLabel->setGeometry(ui->photoLabel->geometry());
    clickablePhotoLabel->setFrameShape(QFrame::Panel);
    clickablePhotoLabel->setFrameShadow(QFrame::Plain);
    clickablePhotoLabel->setText("点击上传照片");
    clickablePhotoLabel->setObjectName("photoUploadLabel");
    clickablePhotoLabel->setToolTip("点击此处上传患者照片\n支持 JPG、PNG、BMP 格式");
    clickablePhotoLabel->setCursor(Qt::PointingHandCursor);
    clickablePhotoLabel->setScaledContents(true);
    clickablePhotoLabel->setAlignment(Qt::AlignCenter);
    ui->photoLabel->hide(); // 隐藏原有的 label

    // 连接信号槽
    connect(ui->primaryBtn, &QPushButton::clicked, this, &MainWindow::addPatient);
    connect(ui->saveBtn, &QPushButton::clicked, this, &MainWindow::savePatient);
    connect(ui->dangerBtn, &QPushButton::clicked, this, &MainWindow::deletePatient);
    connect(ui->saveCaseBtn, &QPushButton::clicked, this, &MainWindow::saveCaseHistory);
    connect(clickablePhotoLabel, &ClickableLabel::clicked, this, &MainWindow::uploadPhoto);

    // 设置模型
    model = new QSqlTableModel(this);
    model->setTable("basic_inf");
    model->select();
    model_d = new QSqlTableModel(this);
    model_d->setTable("details_inf");
    model_d->select();
    ui->basicTableView->setModel(model);
    ui->basicTableView->setAlternatingRowColors(true);

    // 连接表格选择信号
    connect(ui->basicTableView, &QTableView::clicked, this, &MainWindow::on_basicTableView_clicked);

    onTableSelectChange(0);

    // ===== 界面优化：占位提示、状态栏、按钮提示 =====

    // 病历编辑框占位提示文字
    ui->caseTextEdit->setPlaceholderText("请先在下方表格中选择患者，然后在此处编辑病历内容...");

    // 开始诊断按钮 tooltip
    ui->startPushButton->setToolTip("上传 CT 影像并开始自动诊断分析\n支持 PNG、JPG、JPEG、BMP 格式");
    ui->startPushButton->setCursor(Qt::PointingHandCursor);

    // 进度条 tooltip
    ui->progressBar->setToolTip("显示当前图像处理的进度");

    // 状态栏欢迎提示
    statusBar()->showMessage("欢迎使用远程诊断系统 | 请从下方列表选择患者，或点击「新增患者」开始使用");

    // 记录初始尺寸用于等比缩放
    m_baseSize = QSize(760, 650);
    recordGeometries(this);
    setMinimumSize(760, 650);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// 递归记录所有子控件的原始几何信息
void MainWindow::recordGeometries(QWidget* parent)
{
    const QList<QObject*> &children = parent->children();
    for (QObject *obj : children) {
        if (QWidget *w = qobject_cast<QWidget*>(obj)) {
            // 跳过菜单栏、状态栏和工具栏（它们会自动适应）
            if (qobject_cast<QMenuBar*>(w) || qobject_cast<QStatusBar*>(w) ||
                qobject_cast<QToolBar*>(w) || w == centralWidget()) {
                recordGeometries(w); // 继续递归，但不记录自身
                continue;
            }
            m_originalRects[w] = w->geometry();
            m_originalFontSizes[w] = w->font().pixelSize();
            recordGeometries(w); // 递归子控件
        }
    }
}

// 窗口缩放事件：等比缩放所有控件
void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    if (m_baseSize.width() <= 0 || m_baseSize.height() <= 0) return;

    QSize newSize = event->size();
    double xScale = (double)newSize.width() / m_baseSize.width();
    double yScale = (double)newSize.height() / m_baseSize.height();

    // 使用较小的缩放比，保持等比
    double scale = qMin(xScale, yScale);

    // 缩放所有记录的控件
    for (auto it = m_originalRects.begin(); it != m_originalRects.end(); ++it) {
        QWidget *w = it.key();
        if (!w) continue;
        QRect origRect = it.value();

        int newX = (int)(origRect.x() * xScale);
        int newY = (int)(origRect.y() * yScale);
        int newW = (int)(origRect.width() * xScale);
        int newH = (int)(origRect.height() * yScale);

        w->setGeometry(newX, newY, newW, newH);

        // 缩放字体
        if (m_originalFontSizes.contains(w)) {
            int origFontSize = m_originalFontSizes[w];
            if (origFontSize > 0) {
                QFont f = w->font();
                int newFontSize = qMax(8, (int)(origFontSize * scale));
                f.setPixelSize(newFontSize);
                w->setFont(f);
            }
        }
    }

    // 刷新 CT 图片显示
    if (ui->CT_Img_Label->pixmap() && !ui->CT_Img_Label->pixmap()->isNull()) {
        ctImgShow();
    }
}

void MainWindow::initMainWindow()
{
    QString ctImgPath="Tumor.jpg";
    Mat ctImg=imread(ctImgPath.toLatin1().data());
    cvtColor(ctImg,ctImg,COLOR_BGR2RGB);
    myCtImg=ctImg;
    myCtQImage=QImage((const unsigned char*)(ctImg.data),ctImg.cols,ctImg.rows,QImage::Format_RGB888);
    ctImgShow();

    QDate date=QDate::currentDate();
    int year=date.year();
    ui->yearLcdNumber->display(year);
    int month=date.month();
    ui->monthLcdNumber->display(month);
    int day=date.day();
    ui->dayLcdNumber->display(day);
    myTimer=new QTimer();
    myTimer->setInterval(1000);
    myTimer->start();
    connect(myTimer,SIGNAL(timeout()),this,SLOT(onTimeOut()));
}

void MainWindow::ctImgShow()
{
    ui->CT_Img_Label->setPixmap(QPixmap::fromImage(myCtQImage.scaled(ui->CT_Img_Label->size(),Qt::KeepAspectRatio)));
}

void MainWindow::onTableSelectChange(int row)
{
    // 修复：直接使用传入的 row 参数
    if (row < 0 || row >= model->rowCount()) {
        ui->nameLabel->setText("");
        ui->ssnLineEdit->setText("");
        ui->ageSpinBox->setValue(0);
        ui->ethniComboBox->setCurrentText("汉");
        ui->maleRadioButton->setChecked(true);
        clickablePhotoLabel->clear();
        clickablePhotoLabel->setText("点击上传照片");
        return;
    }

    // 获取当前行数据
    QModelIndex index;
    
    // 姓名 (列1)
    index = model->index(row, 1);
    ui->nameLabel->setText(model->data(index).toString());
    
    // 性别 (列2)
    index = model->index(row, 2);
    QString sex = model->data(index).toString();
    (sex.compare("男") == 0) ? ui->maleRadioButton->setChecked(true) : ui->femaleRadioButton->setChecked(true);
    
    // 出生日期 (列4) - 修复年龄计算
    index = model->index(row, 4);
    QDate birthDate = model->data(index).toDate();
    if (birthDate.isValid()) {
        QDate currentDate = QDate::currentDate();
        int age = currentDate.year() - birthDate.year();
        // 精确计算：如果今年的生日还没到，年龄减1
        if (currentDate < birthDate.addYears(age)) {
            age--;
        }
        ui->ageSpinBox->setValue(age);
    } else {
        ui->ageSpinBox->setValue(0);
    }
    
    // 民族 (列3)
    index = model->index(row, 3);
    QString ethnic = model->data(index).toString();
    ui->ethniComboBox->setCurrentText(ethnic);
    
    // 社保号 (列0)
    index = model->index(row, 0);
    QString ssn = model->data(index).toString();
    ui->ssnLineEdit->setText(ssn);
    
    // 显示照片
    showUserPhoto();
}

void MainWindow::ctImgRead()
{
    QString ctImgName=QFileDialog::getOpenFileName(this,u8"载入CT相片",".","Image File(*.png *.jpg *.jpeg *.bmp)");
    if(ctImgName.isEmpty()) return;
    Mat ctRgbImg,ctGrayImg;
    Mat ctImg=imread(ctImgName.toLatin1().data());
    cvtColor(ctImg,ctRgbImg,COLOR_BGR2RGB);
    cvtColor(ctRgbImg,ctGrayImg,COLOR_RGB2GRAY);
    myCtImg=ctRgbImg;
    myCtGrayImg=ctGrayImg;
    myCtQImage=QImage((const unsigned char*)(ctRgbImg.data),ctRgbImg.cols,ctRgbImg.rows,QImage::Format_RGB888);
    ctImgShow();

}

void MainWindow::ctImgProc()
{
    QElapsedTimer timer;
    timer.start();
    ui->progressBar->setValue(19);
    while(timer.elapsed()<2000) QCoreApplication::processEvents();
    ctImgHoughCircles();
    while (timer.elapsed()<2000) QCoreApplication::processEvents();
    ui->progressBar->setValue(ui->progressBar->value()+20);
    ctImgShow();
    while (timer.elapsed()<2000) QCoreApplication::processEvents();
    ui->progressBar->setValue(ui->progressBar->maximum());
    QMessageBox::information(this,u8"完毕",u8"子宫内壁见椭球形阴影，疑似子宫肌瘤");
}

void MainWindow::ctImgSave()
{
    QFile imgae("Tumor_proced.jpg");
    if(!imgae.open(QIODevice::ReadWrite)) return;
    QByteArray qba;
    QBuffer buf(&qba);
    buf.open(QIODevice::WriteOnly);
    myCtQImage.save(&buf,"JPG");
    imgae.write(qba);
}

void MainWindow::ctImgHoughCircles()
{
    Mat ctGrayImg=myCtGrayImg.clone();
    Mat ctColorImg;
    cvtColor(ctGrayImg,ctColorImg,COLOR_GRAY2BGR);
    GaussianBlur(ctGrayImg,ctGrayImg,Size(9,9),2,2);
    vector<Vec3f> h_circles;
    HoughCircles(ctGrayImg,h_circles,HOUGH_GRADIENT,2,ctGrayImg.rows/8,200,100);
    int processValue=45;
    ui->progressBar->setValue(processValue);
    QElapsedTimer timer;
    timer.start();
    while(timer.elapsed()<2000) QCoreApplication::processEvents();
    for(size_t i=0;i<h_circles.size();i++)
    {
        Point center(cvRound(h_circles[i][0]),cvRound(h_circles[i][1]));
        int h_radius=cvRound(h_circles[i][2]);
        circle(ctColorImg,center,h_radius,Scalar(238,0,238),3,8,0);
        circle(ctColorImg,center,3,Scalar(238,0,0),-1,8,0);
        processValue+=1;
        ui->progressBar->setValue(processValue);
    }
    myCtImg=ctColorImg;
    myCtQImage=QImage((const unsigned char*)(myCtImg.data),myCtImg.cols,myCtImg.rows,QImage::Format_RGB888);
}

void MainWindow::showUserPhoto()
{
    QString name = ui->nameLabel->text();
    if (name.isEmpty()) {
        clickablePhotoLabel->clear();
        clickablePhotoLabel->setText("点击上传照片");
        return;
    }

    QSqlQuery query;
    query.prepare("SELECT picture FROM user_profile WHERE name = :name");
    query.bindValue(":name", name);
    if (query.exec() && query.next()) {
        QByteArray base64Data = query.value(0).toByteArray();
        if (!base64Data.isEmpty()) {
            QByteArray imageData = QByteArray::fromBase64(base64Data);
            QPixmap photo;
            photo.loadFromData(imageData, "JPG");
            clickablePhotoLabel->setPixmap(photo.scaled(clickablePhotoLabel->size(), Qt::KeepAspectRatio));
        } else {
            clickablePhotoLabel->clear();
            clickablePhotoLabel->setText("点击上传照片");
        }
    } else {
        clickablePhotoLabel->clear();
        clickablePhotoLabel->setText("点击上传照片");
    }
}

void MainWindow::onTimeOut()
{
    QTime time=QTime::currentTime();
    ui->timeEdit->setTime(time);
}


void MainWindow::on_startPushButton_clicked()
{
    ctImgRead();
    QElapsedTimer timer;
    timer.start();
    ui->progressBar->setMaximum(0);
    ui->progressBar->setMaximum(0);
    while (timer.elapsed()<5000) {
        QCoreApplication::processEvents();
    }
    ui->progressBar->setMaximum(100);
    ui->progressBar->setMinimum(0);
    ctImgProc();
    ui->progressBar->setValue(0);
    ctImgSave();
}

void MainWindow::on_basicTableView_clicked(const QModelIndex &index)
{
    onTableSelectChange(index.row());
}


void MainWindow::on_tabWidget_tabBarClicked(int index)
{
    if(index==1)
    {
        QString name = ui->nameLabel->text();
        if (name.isEmpty()) return;

        QSqlQuery query;
        query.prepare("SELECT case_history FROM details_inf WHERE 姓名 = :name");
        query.bindValue(":name", name);
        if (query.exec() && query.next()) {
            ui->caseTextEdit->setText(query.value(0).toString());
        }
        ui->caseTextEdit->setFont(QFont("楷体",12));
    }
}

void MainWindow::addPatient()
{
    QDialog dialog(this);
    dialog.setWindowTitle(u8"新增患者");
    QFormLayout form(&dialog);

    QLineEdit *ssnEdit = new QLineEdit(&dialog);
    QLineEdit *nameEdit = new QLineEdit(&dialog);
    QComboBox *sexCombo = new QComboBox(&dialog);
    sexCombo->addItems(QStringList() << u8"男" << u8"女");
    QLineEdit *ethnicEdit = new QLineEdit(&dialog);
    ethnicEdit->setText(u8"汉");
    QDateEdit *birthEdit = new QDateEdit(QDate::currentDate(), &dialog);
    birthEdit->setCalendarPopup(true);
    birthEdit->setDisplayFormat("yyyy-MM-dd");

    form.addRow(u8"社保号:", ssnEdit);
    form.addRow(u8"姓名:", nameEdit);
    form.addRow(u8"性别:", sexCombo);
    form.addRow(u8"民族:", ethnicEdit);
    form.addRow(u8"出生日期:", birthEdit);
    QLineEdit *addrEdit = new QLineEdit(&dialog);
    addrEdit->setPlaceholderText(u8"请输入家庭住址");
    form.addRow(u8"住址:", addrEdit);
//弹出确认和取消按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *okBtn = new QPushButton(u8"确定", &dialog);
    QPushButton *cancelBtn = new QPushButton(u8"取消", &dialog);
    btnLayout->addStretch();
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    form.addRow(btnLayout);
    QObject::connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    QObject::connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) return;

    QString ssn = ssnEdit->text().trimmed();
    QString name = nameEdit->text().trimmed();
    if (ssn.isEmpty() || name.isEmpty()) {
        QMessageBox::warning(this, u8"提示", u8"社保号和姓名不能为空");
        return;
    }

    QSqlQuery query;
    query.prepare("INSERT INTO user_profile (ssn, name, sex, ethnic, birth, address) VALUES (:ssn, :name, :sex, :ethnic, :birth, :address)");
    query.bindValue(":ssn", ssn);
    query.bindValue(":name", name);
    query.bindValue(":sex", sexCombo->currentText());
    query.bindValue(":ethnic", ethnicEdit->text().trimmed());
    query.bindValue(":birth", birthEdit->date().toString("yyyy-MM-dd"));
    query.bindValue(":address", addrEdit->text().trimmed());

    if (!query.exec()) {
        QMessageBox::critical(this, u8"错误", u8"新增失败: " + query.lastError().text());
        return;
    }

    model->select();
    model_d->select();
    onTableSelectChange(model->rowCount() - 1);
}

void MainWindow::savePatient()
{
    int row = ui->basicTableView->currentIndex().row();
    if (row < 0) {
        QMessageBox::warning(this, u8"提示", u8"请先选择要修改的患者");
        return;
    }

    QString ssn = ui->ssnLineEdit->text().trimmed();
    QString name = ui->nameLabel->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, u8"提示", u8"患者姓名不能为空");
        return;
    }

    QSqlQuery query;
    query.prepare("UPDATE user_profile SET ssn=:ssn, sex=:sex, ethnic=:ethnic WHERE name=:name");
    query.bindValue(":ssn", ssn);
    query.bindValue(":sex", ui->maleRadioButton->isChecked() ? u8"男" : u8"女");
    query.bindValue(":ethnic", ui->ethniComboBox->currentText());
    query.bindValue(":name", name);

    if (!query.exec()) {
        QMessageBox::critical(this, u8"错误", u8"保存失败: " + query.lastError().text());
        return;
    }

    model->select();
    model_d->select();
    onTableSelectChange(row);
    QMessageBox::information(this, u8"提示", u8"保存成功");
}

void MainWindow::deletePatient()
{
    int row = ui->basicTableView->currentIndex().row();
    if (row < 0) {
        QMessageBox::warning(this, u8"提示", u8"请先选择要删除的患者");
        return;
    }

    QString name = ui->nameLabel->text();
    if (QMessageBox::question(this, u8"确认删除",
            QString(u8"确定要删除患者 %1 吗？").arg(name)) != QMessageBox::Yes)
        return;

    QSqlQuery query;
    query.prepare("DELETE FROM user_profile WHERE name=:name");
    query.bindValue(":name", name);

    if (!query.exec()) {
        QMessageBox::critical(this, u8"错误", u8"删除失败: " + query.lastError().text());
        return;
    }

    model->select();
    model_d->select();
    int newRow = qMin(row, model->rowCount() - 1);
    if (newRow >= 0) {
        ui->basicTableView->selectRow(newRow);
        onTableSelectChange(newRow);
    } else {
        onTableSelectChange(-1);
    }
}

void MainWindow::saveCaseHistory()
{
    QString name = ui->nameLabel->text();
    if (name.isEmpty()) {
        QMessageBox::warning(this, u8"提示", u8"请先选择患者");
        return;
    }

    QString caseHistory = ui->caseTextEdit->toPlainText();

    QSqlQuery query;
    // 先尝试 UPDATE，如果行不存在则 INSERT
    query.prepare("UPDATE details_inf SET case_history=:casehistory WHERE 姓名=:name");
    query.bindValue(":casehistory", caseHistory);
    query.bindValue(":name", name);
    if (!query.exec()) {
        QMessageBox::critical(this, u8"错误", u8"病历保存失败: " + query.lastError().text());
        return;
    }
    if (query.numRowsAffected() == 0) {
        query.prepare("INSERT INTO details_inf (姓名, case_history) VALUES (:name, :casehistory)");
        query.bindValue(":name", name);
        query.bindValue(":casehistory", caseHistory);
        if (!query.exec()) {
            QMessageBox::critical(this, u8"错误", u8"病历保存失败: " + query.lastError().text());
            return;
        }
    }

    model_d->select();
    QMessageBox::information(this, u8"提示", u8"病历保存成功");
}

void MainWindow::uploadPhoto()
{
    QString name = ui->nameLabel->text();
    if (name.isEmpty()) {
        QMessageBox::warning(this, u8"提示", u8"请先选择患者");
        return;
    }

    QString imgPath = QFileDialog::getOpenFileName(this, u8"选择照片", ".", u8"图片文件 (*.png *.jpg *.jpeg *.bmp)");
    if (imgPath.isEmpty()) return;

    QPixmap pixmap(imgPath);
    if (pixmap.isNull()) {
        QMessageBox::warning(this, u8"提示", u8"无法加载图片");
        return;
    }

    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    pixmap.save(&buffer, "JPG");
    QByteArray base64Data = byteArray.toBase64();

    QSqlQuery query;
    query.prepare("UPDATE user_profile SET picture=:photo WHERE name=:name");
    query.bindValue(":photo", base64Data);
    query.bindValue(":name", name);
    if (!query.exec()) {
        QMessageBox::critical(this, u8"错误", u8"照片上传失败: " + query.lastError().text());
        return;
    }

    model_d->select();
    clickablePhotoLabel->setPixmap(pixmap.scaled(clickablePhotoLabel->size(), Qt::KeepAspectRatio));
}
