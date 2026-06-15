# 远程医疗诊断系统

基于 Qt 5.14.2 + OpenCV 4 + MySQL 8.0 的医院患者管理系统。

## 主要功能

### 患者管理
- **新增患者**：录入社保号、姓名、性别、民族、出生日期、住址
- **修改患者**：编辑患者基本信息并保存
- **删除患者**：永久删除患者记录（需确认）
- **患者列表**：表格展示所有患者，点击行查看详情
- **照片上传**：点击照片区域上传患者照片（JPG/PNG/BMP）

### 病历管理
- **病历编辑**：在病历选项卡中为患者编写病历
- **病历保存**：将病历保存到数据库

### CT 影像诊断
- **影像加载**：打开 CT 影像文件（PNG/JPG/BMP）
- **自动诊断**：点击"开始诊断"执行霍夫圆检测算法，识别影像中的圆形病灶
- **影像调节**：水平/垂直滑块调节图像显示参数
- **进度显示**：诊断过程显示进度条

### 辅助功能
- **LCD 时钟**：实时显示当前日期
- **时间显示**：实时显示当前时间、年月日 LCD
- **科室树**：左侧科室导航树
- **窗口缩放**：支持拖拽缩放，控件等比自适应
- **浅蓝主题**：全局浅蓝配色样式表

## 技术栈

| 技术 | 版本 |
|------|------|
| Qt | 5.14.2 MSVC 2017 64-bit |
| OpenCV | 4.x |
| MySQL | 8.0 |
| 编译器 | MSVC 2017 |
| 平台 | Windows |

## 数据库结构

| 表名 | 用途 | 主要字段 |
|------|------|----------|
| `basic_inf` | 患者主表（表格显示） | ssn, name, sex, ethnic, birth |
| `details_inf` | 患者详细表 | 姓名, photo, case_history |
| `user_profile` | 患者档案表（增删改） | ssn, name, sex, ethnic, birth, address, casehistory |

## 项目结构

```
Telemedicine/
├── mainwindow.h        # 主窗口头文件（类声明、ClickableLabel）
├── mainwindow.cpp      # 主窗口实现（UI初始化、业务逻辑）
├── mainwindow.ui       # Qt Designer 界面文件
├── style.qss           # 全局样式表（仅背景+按钮样式）
├── Telemedicine.pro    # Qt 项目文件
├── deploy.bat          # 一键部署打包脚本
└── CLAUDE.md           # 开发指南
```

## 构建与部署

### 开发环境
1. 安装 Qt 5.14.2 (MSVC 2017 64-bit)
2. 安装 OpenCV 4.x，配置环境变量
3. 安装 MySQL 8.0，创建 `patient` 数据库
4. 用 Qt Creator 打开 `Telemedicine.pro`

### 打包发布
在 Release 构建目录下运行 `deploy.bat`，自动收集 Qt/OpenCV/MySQL DLL，生成独立可运行的发布包。

### 数据库要求
- 数据库名：`patient`
- 用户名：`root`
- 密码：`123456`
- 需要 `QMYSQL` 驱动（`qsqlmysql.dll`）

## 注意事项
- 样式表 (`style.qss`) 仅设置颜色和样式，不控制尺寸和字体
- 控件布局在 Qt Designer 的 `.ui` 文件中管理，代码不做位置覆盖
- 窗口最小尺寸：760×650，支持等比缩放
- CT 霍夫圆检测算法有约 5 秒的模拟处理延迟
