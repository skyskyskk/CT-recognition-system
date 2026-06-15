@echo off
chcp 65001 >nul
echo ==============================================
echo   远程医疗系统 - 独立 EXE 打包部署脚本
echo   Qt 5.14.2 + MSVC2017_64 + OpenCV 4 + MySQL 8
echo ==============================================
echo.

REM ====== 路径配置 ======
set QTDIR=D:\Qt\Qt5.14.2\5.14.2\msvc2017_64

REM 项目源码目录（deploy.bat 所在的目录）
set PROJECT_DIR=%~dp0

REM 构建输出目录（固定）
set BUILD_DIR=%PROJECT_DIR%..\build-Telemedicine-Desktop_Qt_5_14_2_MSVC2017_64bit-Release
set RELEASE_DIR=%BUILD_DIR%\release

set OPENCV_DIR=%PROJECT_DIR%packages\opencv4_x64-windows
set MYSQL_DIR=%PROJECT_DIR%packages\mysql_lib\x64_8.1

echo [信息] 源码目录:   %PROJECT_DIR%
echo [信息] Release 目录: %RELEASE_DIR%
echo.

REM ====== 检查 Release 目录是否存在 ======
if not exist "%RELEASE_DIR%\Telemedicine.exe" (
    echo [错误] 找不到 Telemedicine.exe！
    echo.
    echo 请先在 Qt Creator 中以 Release 模式编译项目。
    echo 预期路径: %RELEASE_DIR%\Telemedicine.exe
    echo.
    pause
    exit /b 1
)

REM ====== 第1步：windeployqt ======
echo [步骤 1/5] 运行 windeployqt...
cd /d "%RELEASE_DIR%"

where windeployqt >nul 2>&1
if errorlevel 1 set PATH=%QTDIR%\bin;%PATH%

windeployqt Telemedicine.exe --no-translation --no-compiler-runtime
echo.

REM ====== 第2步：OpenCV + MySQL DLL ======
echo [步骤 2/5] 复制 OpenCV 和 MySQL DLL...

if exist "%OPENCV_DIR%\bin\opencv_core4*.dll" (
    copy /y "%OPENCV_DIR%\bin\opencv_core4*.dll" . >nul      && echo   opencv_core4.dll      ✓
    copy /y "%OPENCV_DIR%\bin\opencv_imgproc4*.dll" . >nul   && echo   opencv_imgproc4.dll   ✓
    copy /y "%OPENCV_DIR%\bin\opencv_highgui4*.dll" . >nul   && echo   opencv_highgui4.dll   ✓
    copy /y "%OPENCV_DIR%\bin\opencv_imgcodecs4*.dll" . >nul && echo   opencv_imgcodecs4.dll ✓
    copy /y "%OPENCV_DIR%\bin\opencv_videoio4*.dll" . >nul 2>nul
) else (
    echo   [警告] 找不到 OpenCV DLL
)

if exist "%MYSQL_DIR%\lib\libmysql.dll" (
    copy /y "%MYSQL_DIR%\lib\libmysql.dll" . >nul && echo   libmysql.dll          ✓
    REM 关键：MySQL 驱动需要 libmysql.dll 在 sqldrivers 目录下
    if not exist sqldrivers mkdir sqldrivers
    copy /y "%MYSQL_DIR%\lib\libmysql.dll" sqldrivers\ >nul && echo   sqldrivers\libmysql.dll ✓
) else (
    echo   [警告] 找不到 libmysql.dll
)
echo.

REM ====== 第3步：Qt 插件 ======
echo [步骤 3/5] 检查 Qt 插件...

if not exist platforms mkdir platforms
if not exist platforms\qwindows.dll (
    copy /y "%QTDIR%\plugins\platforms\qwindows.dll" platforms\ >nul
    echo   platforms\qwindows.dll    ✓ (已补充)
) else (
    echo   platforms\qwindows.dll    ✓
)

if not exist sqldrivers mkdir sqldrivers
if not exist sqldrivers\qsqlmysql.dll (
    if exist "%QTDIR%\plugins\sqldrivers\qsqlmysql.dll" (
        copy /y "%QTDIR%\plugins\sqldrivers\qsqlmysql.dll" sqldrivers\ >nul
        echo   sqldrivers\qsqlmysql.dll ✓ (已补充)
    )
) else (
    echo   sqldrivers\qsqlmysql.dll ✓
)

if not exist imageformats mkdir imageformats
if not exist imageformats\qjpeg.dll (
    copy /y "%QTDIR%\plugins\imageformats\qjpeg.dll" imageformats\ >nul
    echo   imageformats\qjpeg.dll    ✓ (已补充)
) else (
    echo   imageformats\qjpeg.dll    ✓
)
echo.

REM ====== 第4步：资源文件 ======
echo [步骤 4/5] 复制资源文件...

if exist "%PROJECT_DIR%style.qss" (
    copy /y "%PROJECT_DIR%style.qss" "%RELEASE_DIR%" >nul
    echo   style.qss ✓
) else (
    echo   [警告] 找不到 style.qss
)

if exist "%PROJECT_DIR%Tumor.jpg" (
    copy /y "%PROJECT_DIR%Tumor.jpg" "%RELEASE_DIR%" >nul
    echo   Tumor.jpg ✓
) else (
    echo   [警告] 找不到 Tumor.jpg
)
echo.

REM ====== 第5步：完成 ======
echo [步骤 5/5] 完成
echo.
echo ==============================================
echo   部署完成！目录: %RELEASE_DIR%
echo ==============================================
echo.
echo   将此 release 目录复制到任意 Windows 电脑，
echo   无需安装 Qt 即可直接运行 Telemedicine.exe
echo.
echo ==============================================
echo 正在启动程序...
start Telemedicine.exe
pause
