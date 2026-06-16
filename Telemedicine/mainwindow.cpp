// 远程医疗诊断系统主窗口实现文件
// 包含主窗口类的所有成员函数实现，包括UI初始化、数据库操作、CT影像处理等

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
#include <QSqlError>
#include <QStatusBar>
#include <QMenuBar>
#include <QToolBar>
#include <QElapsedTimer>

// 构造函数：初始化UI、加载样式表、创建控件、建立信号槽连接、初始化数据库模型
// parent为父窗口指针
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    resize(1580, 1200); // 设置初始窗口大小
    initMainWindow();   // 初始化主窗口

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

    // 设置数据库模型
    model = new QSqlTableModel(this);
    model->setTable("basic_inf");  // 基本信息视图
    model->select();
    model_d = new QSqlTableModel(this);
    model_d->setTable("details_inf"); // 详细信息视图
    model_d->select();
    ui->basicTableView->setModel(model);
    ui->basicTableView->setAlternatingRowColors(true); // 开启交替行颜色

    // 连接表格选择信号
    connect(ui->basicTableView, &QTableView::clicked, this, &MainWindow::on_basicTableView_clicked);

    onTableSelectChange(0); // 默认显示第一行患者信息

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

// 析构函数：释放UI资源
MainWindow::~MainWindow()
{
    delete ui;
}

// 记录控件几何信息：递归遍历所有子控件，记录原始位置、大小和字体大小，用于窗口缩放
// parent为父窗口指针
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

// 窗口缩放事件：实现窗口等比缩放功能，保持控件比例和布局一致性
// event为大小变化事件对象

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

// 初始化主窗口：加载初始CT影像、设置日期显示、启动定时器

void MainWindow::initMainWindow()
{
    // 加载初始CT影像（使用应用程序所在目录的绝对路径）
    QString ctImgPath = QCoreApplication::applicationDirPath() + "/Tumor.jpg";
    Mat ctImg = imread(ctImgPath.toLatin1().data());
    if (ctImg.empty()) {
        qDebug() << "无法加载CT影像：" << ctImgPath;
        return;
    }
    cvtColor(ctImg,ctImg,COLOR_BGR2RGB);
    myCtImg=ctImg;
    myCtQImage=QImage((const unsigned char*)(ctImg.data),ctImg.cols,ctImg.rows,QImage::Format_RGB888);
    ctImgShow();

    // 设置日期显示
    QDate date=QDate::currentDate();
    int year=date.year();
    ui->yearLcdNumber->display(year);
    int month=date.month();
    ui->monthLcdNumber->display(month);
    int day=date.day();
    ui->dayLcdNumber->display(day);
    
    // 启动定时器更新时间
    myTimer=new QTimer();
    myTimer->setInterval(1000);
    myTimer->start();
    connect(myTimer,SIGNAL(timeout()),this,SLOT(onTimeOut()));
}

// 显示CT影像：将处理后的CT影像显示到界面上
void MainWindow::ctImgShow()
{
    ui->CT_Img_Label->setPixmap(QPixmap::fromImage(
        myCtQImage.scaled(ui->CT_Img_Label->size(), Qt::KeepAspectRatio)
    ));
}

// 表格选择变化：更新右侧患者详细信息显示
// row为选中的行号

void MainWindow::onTableSelectChange(int row)
{
    // 无效行处理
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
    
    // 出生日期 (列4) - 精确计算年龄
    index = model->index(row, 4);
    QDate birthDate = model->data(index).toDate();
    if (birthDate.isValid()) {
        QDate currentDate = QDate::currentDate();
        int age = currentDate.year() - birthDate.year();
        // 如果今年的生日还没到，年龄减1
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
    
    // 住址 — 从 user_profile 表查询（basic_inf 视图可能不含此字段）
    {
        QSqlQuery addrQuery;
        QString curName = ui->nameLabel->text();
        addrQuery.prepare("SELECT address FROM user_profile WHERE name = :name");
        addrQuery.bindValue(":name", curName);
        if (addrQuery.exec() && addrQuery.next()) {
            QLineEdit *addrEdit = findChild<QLineEdit*>("addrLineEdit");
            if (addrEdit)
                addrEdit->setText(addrQuery.value(0).toString());
        }
    }

    // 显示患者照片
    showUserPhoto();
}

// 读取CT影像：弹出文件对话框选择CT影像文件并加载
void MainWindow::ctImgRead()
{
    QString ctImgName=QFileDialog::getOpenFileName(
        this, u8"载入CT相片", ".", "Image File(*.png *.jpg *.jpeg *.bmp)"
    );
    if(ctImgName.isEmpty()) return;
    
    // 读取并转换CT影像
    Mat ctRgbImg,ctGrayImg;
    Mat ctImg=imread(ctImgName.toLatin1().data());
    if (ctImg.empty()) {
        QMessageBox::warning(this, u8"提示", u8"无法读取CT影像文件");
        return;
    }
    cvtColor(ctImg,ctRgbImg,COLOR_BGR2RGB);
    cvtColor(ctRgbImg,ctGrayImg,COLOR_RGB2GRAY);
    
    // 保存影像数据
    myCtImg=ctRgbImg;
    myCtGrayImg=ctGrayImg;
    myCtQImage=QImage(
        (const unsigned char*)(ctRgbImg.data),
        ctRgbImg.cols, ctRgbImg.rows,
        QImage::Format_RGB888
    );
    
    // 显示影像
    ctImgShow();
}

// 处理CT影像：执行CT影像分析流程，包括Hough圆检测和结果显示
void MainWindow::ctImgProc()
{
    QElapsedTimer timer;
    timer.start();
    
    // 模拟处理进度
    ui->progressBar->setValue(19);
    while(timer.elapsed()<2000) QCoreApplication::processEvents();
    
    // 执行Hough圆检测
    ctImgHoughCircles();
    
    // 继续模拟处理进度
    while (timer.elapsed()<2000) QCoreApplication::processEvents();
    ui->progressBar->setValue(ui->progressBar->value()+20);
    
    // 显示处理后的影像
    ctImgShow();
    
    // 完成处理
    while (timer.elapsed()<2000) QCoreApplication::processEvents();
    ui->progressBar->setValue(ui->progressBar->maximum());
    
    // 显示诊断结果
    QMessageBox::information(
        this, u8"完毕", u8"子宫内壁见椭球形阴影，疑似子宫肌瘤"
    );
}

// 保存CT影像：将处理后的CT影像保存为JPG文件
void MainWindow::ctImgSave()
{
    QFile imageFile("Tumor_proced.jpg");
    if(!imageFile.open(QIODevice::ReadWrite)) return;
    
    // 将QImage转换为JPG格式
    QByteArray qba;
    QBuffer buf(&qba);
    buf.open(QIODevice::WriteOnly);
    myCtQImage.save(&buf, "JPG");
    
    // 写入文件
    imageFile.write(qba);
}

// Hough圆检测：检测CT影像中的圆形病灶并标记
void MainWindow::ctImgHoughCircles()
{
    // 复制灰度影像并转换为彩色
    Mat ctGrayImg=myCtGrayImg.clone();
    Mat ctColorImg;
    cvtColor(ctGrayImg,ctColorImg,COLOR_GRAY2BGR);
    
    // 高斯模糊降噪
    GaussianBlur(ctGrayImg,ctGrayImg,Size(9,9),2,2);
    
    // 执行Hough圆检测
    vector<Vec3f> h_circles;
    HoughCircles(
        ctGrayImg, h_circles, HOUGH_GRADIENT,
        2, ctGrayImg.rows/8, 200, 100
    );
    
    // 模拟处理进度
    int processValue=45;
    ui->progressBar->setValue(processValue);
    QElapsedTimer timer;
    timer.start();
    while(timer.elapsed()<2000) QCoreApplication::processEvents();
    
    // 标记检测到的圆形
    for(size_t i=0;i<h_circles.size();i++)
    {
        Point center(cvRound(h_circles[i][0]),cvRound(h_circles[i][1]));
        int h_radius=cvRound(h_circles[i][2]);
        // 绘制圆形
        circle(ctColorImg,center,h_radius,Scalar(238,0,238),3,8,0);
        // 绘制圆心
        circle(ctColorImg,center,3,Scalar(238,0,0),-1,8,0);
        
        // 更新进度
        processValue+=1;
        ui->progressBar->setValue(processValue);
    }
    
    // 保存处理结果
    myCtImg=ctColorImg;
    myCtQImage=QImage(
        (const unsigned char*)(myCtImg.data),
        myCtImg.cols, myCtImg.rows,
        QImage::Format_RGB888
    );
}

// 显示用户照片：从数据库读取患者照片并显示
void MainWindow::showUserPhoto()
{
    QString name = ui->nameLabel->text();
    if (name.isEmpty()) {
        clickablePhotoLabel->clear();
        clickablePhotoLabel->setText("点击上传照片");
        return;
    }

    // 查询数据库获取照片
    QSqlQuery query;
    query.prepare("SELECT picture FROM user_profile WHERE name = :name");
    query.bindValue(":name", name);
    
    if (query.exec() && query.next()) {
        QByteArray base64Data = query.value(0).toByteArray();
        if (!base64Data.isEmpty()) {
            // 解码Base64并显示照片
            QByteArray imageData = QByteArray::fromBase64(base64Data);
            QPixmap photo;
            photo.loadFromData(imageData, "JPG");
            clickablePhotoLabel->setPixmap(
                photo.scaled(clickablePhotoLabel->size(), Qt::KeepAspectRatio)
            );
        } else {
            clickablePhotoLabel->clear();
            clickablePhotoLabel->setText("点击上传照片");
        }
    } else {
        clickablePhotoLabel->clear();
        clickablePhotoLabel->setText("点击上传照片");
    }
}

// 定时器超时：更新当前时间显示
void MainWindow::onTimeOut()
{
    QTime time=QTime::currentTime();
    ui->timeEdit->setTime(time);
}


// 开始诊断按钮：读取CT影像 → 模拟处理 → 诊断 → 保存
void MainWindow::on_startPushButton_clicked()
{
    ui->startPushButton->setEnabled(false); // 防重复点击

    ctImgRead();
    if (myCtImg.empty()) {
        ui->startPushButton->setEnabled(true);
        return;
    }

    QElapsedTimer timer;
    timer.start();
    ui->progressBar->setMaximum(0);
    while (timer.elapsed() < 5000) {
        QCoreApplication::processEvents();
    }

    ui->progressBar->setMaximum(100);
    ctImgProc();
    ctImgSave();
    ui->progressBar->setValue(100);

    ui->startPushButton->setEnabled(true);
}

// 表格点击事件：处理用户在基本信息表格中的点击事件
// index为点击的模型索引
void MainWindow::on_basicTableView_clicked(const QModelIndex &index)
{
    onTableSelectChange(index.row());
}


// 标签页切换事件：处理用户切换标签页的事件
// index为切换到的标签页索引
void MainWindow::on_tabWidget_tabBarClicked(int index)
{
    if(index==1) // 病历标签页
    {
        QString name = ui->nameLabel->text();
        if (name.isEmpty()) return;

        // 查询病历
        QSqlQuery query;
        query.prepare("SELECT casehistory FROM user_profile WHERE name = :name");
        query.bindValue(":name", name);
        
        if (query.exec() && query.next()) {
            ui->caseTextEdit->setText(query.value(0).toString());
        }
        
        // 设置字体
        ui->caseTextEdit->setFont(QFont("楷体", 12));
    }
}

// 新增患者：弹出对话框输入患者信息并插入到数据库
void MainWindow::addPatient()
{
    // 创建对话框
    QDialog dialog(this);
    dialog.setWindowTitle(u8"新增患者");
    QFormLayout form(&dialog);

    // 创建输入控件
    QLineEdit *ssnEdit = new QLineEdit(&dialog);
    QLineEdit *nameEdit = new QLineEdit(&dialog);
    QComboBox *sexCombo = new QComboBox(&dialog);
    sexCombo->addItems(QStringList() << u8"男" << u8"女");
    QLineEdit *ethnicEdit = new QLineEdit(&dialog);
    ethnicEdit->setText(u8"汉");
    QDateEdit *birthEdit = new QDateEdit(QDate::currentDate(), &dialog);
    birthEdit->setCalendarPopup(true);
    birthEdit->setDisplayFormat("yyyy-MM-dd");
    QLineEdit *addrEdit = new QLineEdit(&dialog);
    addrEdit->setPlaceholderText(u8"请输入家庭住址");

    // 添加表单行
    form.addRow(u8"社保号:", ssnEdit);
    form.addRow(u8"姓名:", nameEdit);
    form.addRow(u8"性别:", sexCombo);
    form.addRow(u8"民族:", ethnicEdit);
    form.addRow(u8"出生日期:", birthEdit);
    form.addRow(u8"住址:", addrEdit);

    // 添加按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *okBtn = new QPushButton(u8"确定", &dialog);
    QPushButton *cancelBtn = new QPushButton(u8"取消", &dialog);
    btnLayout->addStretch();
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    form.addRow(btnLayout);
    
    // 连接信号槽
    QObject::connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    QObject::connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);

    // 显示对话框
    if (dialog.exec() != QDialog::Accepted) return;

    // 验证输入
    QString ssn = ssnEdit->text().trimmed();
    QString name = nameEdit->text().trimmed();
    if (ssn.isEmpty() || name.isEmpty()) {
        QMessageBox::warning(this, u8"提示", u8"社保号和姓名不能为空");
        return;
    }

    // 插入数据库
    QSqlQuery query;
    query.prepare(
        "INSERT INTO user_profile (ssn, name, sex, ethnic, birth, address) "
        "VALUES (:ssn, :name, :sex, :ethnic, :birth, :address)"
    );
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

    // 更新表格并显示新增的患者
    model->select();
    model_d->select();
    onTableSelectChange(model->rowCount() - 1);
}

// 保存患者：将编辑后的患者信息保存到数据库
void MainWindow::savePatient()
{
    // 检查是否选择了患者
    int row = ui->basicTableView->currentIndex().row();
    if (row < 0) {
        QMessageBox::warning(this, u8"提示", u8"请先选择要修改的患者");
        return;
    }

    // 验证输入
    QString ssn = ui->ssnLineEdit->text().trimmed();
    QString name = ui->nameLabel->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, u8"提示", u8"患者姓名不能为空");
        return;
    }

    // 根据年龄反推出生年份（保留原始月/日）
    int newAge = ui->ageSpinBox->value();
    QModelIndex birthIdx = model->index(row, 4);
    QDate origBirth = birthIdx.data().toDate();
    QDate newBirth(QDate::currentDate().year() - newAge,
                   origBirth.month(), origBirth.day());

    // 更新数据库（包含住址）
    QSqlQuery query;
    query.prepare(
        "UPDATE user_profile SET ssn=:ssn, sex=:sex, ethnic=:ethnic, birth=:birth, address=:address WHERE name=:name"
    );
    query.bindValue(":ssn", ssn);
    query.bindValue(":sex", ui->maleRadioButton->isChecked() ? u8"男" : u8"女");
    query.bindValue(":ethnic", ui->ethniComboBox->currentText());
    query.bindValue(":birth", newBirth.toString("yyyy-MM-dd"));
    QLineEdit *addrEdit = findChild<QLineEdit*>("addrLineEdit");
    query.bindValue(":address", addrEdit ? addrEdit->text().trimmed() : "");
    query.bindValue(":name", name);

    if (!query.exec()) {
        QMessageBox::critical(this, u8"错误", u8"保存失败: " + query.lastError().text());
        return;
    }
    if (query.numRowsAffected() == 0) {
        QMessageBox::warning(this, u8"提示", u8"未找到对应患者记录，保存失败");
        return;
    }

    // 更新表格并刷新显示
    model->select();
    model_d->select();
    onTableSelectChange(row);
    QMessageBox::information(this, u8"提示", u8"保存成功");
}

// 删除患者：删除当前选中的患者记录
void MainWindow::deletePatient()
{
    // 检查是否选择了患者
    int row = ui->basicTableView->currentIndex().row();
    if (row < 0) {
        QMessageBox::warning(this, u8"提示", u8"请先选择要删除的患者");
        return;
    }

    // 确认删除
    QString name = ui->nameLabel->text();
    if (QMessageBox::question(
        this, u8"确认删除",
        QString(u8"确定要删除患者 %1 吗？").arg(name)
    ) != QMessageBox::Yes)
        return;

    // 执行删除
    QSqlQuery query;
    query.prepare("DELETE FROM user_profile WHERE name=:name");
    query.bindValue(":name", name);

    if (!query.exec()) {
        QMessageBox::critical(this, u8"错误", u8"删除失败: " + query.lastError().text());
        return;
    }
    if (query.numRowsAffected() == 0) {
        QMessageBox::warning(this, u8"提示", u8"未找到对应患者记录");
        return;
    }

    // 更新表格并刷新显示
    model->select();
    model_d->select();
    
    // 选择下一个患者
    int newRow = qMin(row, model->rowCount() - 1);
    if (newRow >= 0) {
        ui->basicTableView->selectRow(newRow);
        onTableSelectChange(newRow);
    } else {
        onTableSelectChange(-1);
    }
}

// 保存病历：将病历文本保存到数据库
void MainWindow::saveCaseHistory()
{
    // 检查是否选择了患者
    QString name = ui->nameLabel->text();
    if (name.isEmpty()) {
        QMessageBox::warning(this, u8"提示", u8"请先选择患者");
        return;
    }

    // 获取病历文本
    QString caseHistory = ui->caseTextEdit->toPlainText();

    // 保存病历到 user_profile
    QSqlQuery query;
    query.prepare("UPDATE user_profile SET casehistory=:casehistory WHERE name=:name");
    query.bindValue(":casehistory", caseHistory);
    query.bindValue(":name", name);

    if (!query.exec()) {
        QMessageBox::critical(this, u8"错误", u8"病历保存失败: " + query.lastError().text());
        return;
    }
    if (query.numRowsAffected() == 0) {
        QMessageBox::warning(this, u8"提示", u8"未找到对应患者记录，病历保存失败");
        return;
    }

    model_d->select();
    QMessageBox::information(this, u8"提示", u8"病历保存成功");
}

// 上传照片：处理用户上传患者照片的请求
void MainWindow::uploadPhoto()
{
    // 检查是否选择了患者
    QString name = ui->nameLabel->text();
    if (name.isEmpty()) {
        QMessageBox::warning(this, u8"提示", u8"请先选择患者");
        return;
    }

    // 选择图片文件
    QString imgPath = QFileDialog::getOpenFileName(
        this, u8"选择照片", ".", u8"图片文件 (*.png *.jpg *.jpeg *.bmp)"
    );
    if (imgPath.isEmpty()) return;

    // 加载图片
    QPixmap pixmap(imgPath);
    if (pixmap.isNull()) {
        QMessageBox::warning(this, u8"提示", u8"无法加载图片");
        return;
    }

    // 转换为Base64
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    pixmap.save(&buffer, "JPG");
    QByteArray base64Data = byteArray.toBase64();

    // 保存到数据库
    QSqlQuery query;
    query.prepare("UPDATE user_profile SET picture=:photo WHERE name=:name");
    query.bindValue(":photo", base64Data);
    query.bindValue(":name", name);
    
    if (!query.exec()) {
        QMessageBox::critical(this, u8"错误", u8"照片上传失败: " + query.lastError().text());
        return;
    }
    if (query.numRowsAffected() == 0) {
        QMessageBox::warning(this, u8"提示", u8"未找到对应患者记录，照片保存失败");
        return;
    }

    model_d->select();
    clickablePhotoLabel->setPixmap(
        pixmap.scaled(clickablePhotoLabel->size(), Qt::KeepAspectRatio)
    );
}
