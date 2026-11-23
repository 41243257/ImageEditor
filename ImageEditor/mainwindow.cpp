#include "mainwindow.h"

#include <QAction>
#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QImageReader>
#include <QImageWriter>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QScrollArea>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStyle>
#include <QToolBar>
#include <QTransform>
#include <QtMath>

namespace {

// Qt 沒有內建「旋轉」的標準圖示，所以自己畫一個順時針的圓弧箭頭 ↻
QIcon makeRotateRightIcon()
{
    QIcon icon;
    for (int size : {16, 24, 32, 48}) {
        QPixmap pm(size, size);
        pm.fill(Qt::transparent);

        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        const QColor color(40, 110, 200);

        const qreal margin = size * 0.18;
        const QRectF circle(margin, margin, size - 2 * margin, size - 2 * margin);
        // 角度：0° 在 3 點鐘方向，逆時針為正
        const qreal startDeg = 60;   // 從右上方開始
        const qreal sweepDeg = -290; // 順時針繞將近一圈，停在左上方（缺口在上面）

        QPainterPath arc;
        arc.arcMoveTo(circle, startDeg);
        arc.arcTo(circle, startDeg, sweepDeg);
        p.setPen(QPen(color, size * 0.11, Qt::SolidLine, Qt::RoundCap));
        p.drawPath(arc);

        // 在圓弧終點畫箭頭，朝順時針方向
        const qreal endRad = qDegreesToRadians(startDeg + sweepDeg);
        const QPointF end = arc.currentPosition();
        const QPointF dir(qSin(endRad), qCos(endRad));     // 前進方向
        const QPointF normal(qCos(endRad), -qSin(endRad)); // 半徑方向
        const qreal length = size * 0.24;
        const qreal halfWidth = size * 0.17;

        QPolygonF head;
        head << end + dir * length << end + normal * halfWidth << end - normal * halfWidth;
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawPolygon(head);

        icon.addPixmap(pm);
    }
    return icon;
}

// 把圖示左右翻轉：↻ 翻過來就是 ↺（向左旋轉）
QIcon mirroredIcon(const QIcon &icon)
{
    QIcon result;
    for (int size : {16, 24, 32, 48}) {
        const QPixmap pm = icon.pixmap(QSize(size, size));
        result.addPixmap(pm.transformed(QTransform().scale(-1, 1)));
    }
    return result;
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    createActions();
    createMenus();
    createToolBar();
    createCentralArea();
    createStatusBar();

    updateActions();
    updateWindowTitle();
    resize(1000, 700);
}

// ───────────────────────── 建立介面 ─────────────────────────

void MainWindow::createActions()
{
    QStyle *s = style();

    // 圖示：先用系統圖示主題，沒有的話改用 Qt 內建的標準圖示
    // ── File ──
    m_openAct = new QAction(QIcon::fromTheme(QStringLiteral("document-open"),
                                             s->standardIcon(QStyle::SP_DialogOpenButton)),
                            tr("&Open..."), this);
    m_openAct->setShortcut(QKeySequence(QStringLiteral("Ctrl+O")));
    m_openAct->setToolTip(tr("開啟圖片 (Ctrl+O)"));
    m_openAct->setStatusTip(tr("開啟電腦中的圖片檔"));
    connect(m_openAct, &QAction::triggered, this, &MainWindow::openImage);

    m_saveAct = new QAction(QIcon::fromTheme(QStringLiteral("document-save"),
                                             s->standardIcon(QStyle::SP_DialogSaveButton)),
                            tr("&Save"), this);
    m_saveAct->setShortcut(QKeySequence(QStringLiteral("Ctrl+S")));
    m_saveAct->setToolTip(tr("儲存 (Ctrl+S)"));
    m_saveAct->setStatusTip(tr("把目前的圖片存回原本的檔案"));
    connect(m_saveAct, &QAction::triggered, this, &MainWindow::saveImage);

    m_saveAsAct = new QAction(QIcon::fromTheme(QStringLiteral("document-save-as"),
                                               s->standardIcon(QStyle::SP_DialogSaveButton)),
                              tr("Save &As..."), this);
    m_saveAsAct->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+S")));
    m_saveAsAct->setToolTip(tr("另存新檔 (Ctrl+Shift+S)"));
    m_saveAsAct->setStatusTip(tr("把目前的圖片存成新的檔案"));
    connect(m_saveAsAct, &QAction::triggered, this, &MainWindow::saveImageAs);

    m_exitAct = new QAction(QIcon::fromTheme(QStringLiteral("application-exit"),
                                             s->standardIcon(QStyle::SP_DialogCloseButton)),
                            tr("E&xit"), this);
    m_exitAct->setShortcut(QKeySequence(QStringLiteral("Ctrl+Q")));
    m_exitAct->setToolTip(tr("離開程式 (Ctrl+Q)"));
    m_exitAct->setStatusTip(tr("關閉程式"));
    // close() 會觸發 closeEvent()，所以離開前一樣會詢問是否儲存
    connect(m_exitAct, &QAction::triggered, this, &QWidget::close);

    // ── Image ──
    const QIcon rotateRightIcon = makeRotateRightIcon();

    m_rotateLeftAct = new QAction(QIcon::fromTheme(QStringLiteral("object-rotate-left"),
                                                   mirroredIcon(rotateRightIcon)),
                                  tr("Rotate &Left 90°"), this);
    m_rotateLeftAct->setShortcut(QKeySequence(QStringLiteral("Ctrl+L")));
    m_rotateLeftAct->setToolTip(tr("向左旋轉 90° (Ctrl+L)"));
    m_rotateLeftAct->setStatusTip(tr("把圖片逆時針旋轉 90 度"));
    connect(m_rotateLeftAct, &QAction::triggered, this, &MainWindow::rotateLeft);

    m_rotateRightAct = new QAction(QIcon::fromTheme(QStringLiteral("object-rotate-right"),
                                                    rotateRightIcon),
                                   tr("Rotate &Right 90°"), this);
    m_rotateRightAct->setShortcut(QKeySequence(QStringLiteral("Ctrl+R")));
    m_rotateRightAct->setToolTip(tr("向右旋轉 90° (Ctrl+R)"));
    m_rotateRightAct->setStatusTip(tr("把圖片順時針旋轉 90 度"));
    connect(m_rotateRightAct, &QAction::triggered, this, &MainWindow::rotateRight);
}

void MainWindow::createMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(m_openAct);
    fileMenu->addAction(m_saveAct);
    fileMenu->addAction(m_saveAsAct);
    fileMenu->addSeparator();
    fileMenu->addAction(m_exitAct);

    // 之後的影像處理功能（調整解析度、切割、濾鏡…）都加在這個選單
    QMenu *imageMenu = menuBar()->addMenu(tr("&Image"));
    imageMenu->addAction(m_rotateLeftAct);
    imageMenu->addAction(m_rotateRightAct);
}

void MainWindow::createToolBar()
{
    // 工具列和選單共用「同一個」QAction：
    // 一邊停用（灰掉），另一邊也會跟著停用
    QToolBar *toolBar = addToolBar(tr("Main Toolbar"));
    toolBar->setObjectName(QStringLiteral("mainToolBar"));
    toolBar->addAction(m_openAct);
    toolBar->addAction(m_saveAct);
    toolBar->addSeparator();
    toolBar->addAction(m_rotateLeftAct);
    toolBar->addAction(m_rotateRightAct);
}

void MainWindow::createCentralArea()
{
    // 圖片顯示在 QLabel 上；外面包一層 QScrollArea，圖比視窗大時可以捲動
    m_imageLabel = new QLabel;
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setText(tr("尚未開啟圖片\n\n請從 File > Open 開啟，或按 Ctrl+O"));

    QPalette pal = m_imageLabel->palette();
    pal.setColor(QPalette::WindowText, Qt::gray);
    m_imageLabel->setPalette(pal);

    m_scrollArea = new QScrollArea;
    m_scrollArea->setWidget(m_imageLabel);
    m_scrollArea->setWidgetResizable(true); // 圖比視窗小時置中
    setCentralWidget(m_scrollArea);
}

void MainWindow::createStatusBar()
{
    m_sizeLabel = new QLabel;
    statusBar()->addPermanentWidget(m_sizeLabel);
    statusBar()->showMessage(tr("就緒"));
}

// ───────────────────────── File 功能 ─────────────────────────

void MainWindow::openImage()
{
    if (!maybeSave())
        return;

    const QString startDir = m_filePath.isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)
        : QFileInfo(m_filePath).absolutePath();

    const QString filePath = QFileDialog::getOpenFileName(
        this, tr("開啟圖片"), startDir,
        tr("圖片檔 (*.png *.jpg *.jpeg *.bmp);;所有檔案 (*)"));
    if (filePath.isEmpty())
        return; // 使用者按了取消

    QImageReader reader(filePath);
    reader.setAutoTransform(true); // 手機照片會依 EXIF 方向自動轉正
    const QImage image = reader.read();
    if (image.isNull()) {
        QMessageBox::warning(this, tr("無法開啟"),
                             tr("無法讀取這個檔案：\n%1\n\n原因：%2")
                                 .arg(QDir::toNativeSeparators(filePath), reader.errorString()));
        return;
    }

    m_filePath = filePath;
    setImage(image);
    setWindowModified(false);
    updateWindowTitle();
    statusBar()->showMessage(tr("已開啟 %1").arg(QFileInfo(filePath).fileName()), 3000);
}

bool MainWindow::saveImage()
{
    if (m_image.isNull())
        return false;
    if (m_filePath.isEmpty())
        return saveImageAs();
    return writeImage(m_filePath);
}

bool MainWindow::saveImageAs()
{
    if (m_image.isNull())
        return false;

    QString selectedFilter;
    QString filePath = QFileDialog::getSaveFileName(
        this, tr("另存新檔"), m_filePath,
        tr("PNG 圖片 (*.png);;JPEG 圖片 (*.jpg *.jpeg);;BMP 圖片 (*.bmp)"),
        &selectedFilter);
    if (filePath.isEmpty())
        return false;

    // 使用者沒打副檔名時，依照選的格式補上
    if (QFileInfo(filePath).suffix().isEmpty()) {
        if (selectedFilter.contains(QStringLiteral("jpg")))
            filePath += QStringLiteral(".jpg");
        else if (selectedFilter.contains(QStringLiteral("bmp")))
            filePath += QStringLiteral(".bmp");
        else
            filePath += QStringLiteral(".png");
    }

    if (!writeImage(filePath))
        return false;

    m_filePath = filePath;
    updateWindowTitle();
    return true;
}

bool MainWindow::writeImage(const QString &filePath)
{
    const QString suffix = QFileInfo(filePath).suffix().toLower();
    const bool isJpeg = (suffix == QLatin1String("jpg") || suffix == QLatin1String("jpeg"));

    QImage output = m_image;

    // JPG 不支援透明：先把透明的地方填成白色，避免存出來變黑
    if (isJpeg && output.hasAlphaChannel()) {
        QImage flat(output.size(), QImage::Format_RGB32);
        flat.fill(Qt::white);
        QPainter painter(&flat);
        painter.drawImage(0, 0, output);
        painter.end();
        output = flat;
    }

    QImageWriter writer(filePath);
    if (isJpeg)
        writer.setQuality(95);

    if (!writer.write(output)) {
        QMessageBox::warning(this, tr("無法儲存"),
                             tr("無法儲存到：\n%1\n\n原因：%2")
                                 .arg(QDir::toNativeSeparators(filePath), writer.errorString()));
        return false;
    }

    setWindowModified(false);
    statusBar()->showMessage(tr("已儲存 %1").arg(QFileInfo(filePath).fileName()), 3000);
    return true;
}

// ───────────────────────── Image 功能 ─────────────────────────

void MainWindow::rotateLeft()
{
    rotate(-90);
}

void MainWindow::rotateRight()
{
    rotate(90);
}

void MainWindow::rotate(int degrees)
{
    if (m_image.isNull())
        return;

    QTransform transform;
    transform.rotate(degrees);
    setImage(m_image.transformed(transform));
    setWindowModified(true); // 標題列出現 *，提醒還沒存檔
}

// ───────────────────────── 狀態更新 ─────────────────────────

void MainWindow::setImage(const QImage &image)
{
    m_image = image;
    m_imageLabel->setPixmap(QPixmap::fromImage(m_image));
    updateActions();
    updateStatusInfo();
}

void MainWindow::updateActions()
{
    // 沒開圖時，存檔和旋轉都不能用（選單和工具列會一起灰掉）
    const bool hasImage = !m_image.isNull();
    m_saveAct->setEnabled(hasImage);
    m_saveAsAct->setEnabled(hasImage);
    m_rotateLeftAct->setEnabled(hasImage);
    m_rotateRightAct->setEnabled(hasImage);
}

void MainWindow::updateWindowTitle()
{
    // [*] 是 Qt 的佔位符號：setWindowModified(true) 時會顯示成 *
    if (m_filePath.isEmpty())
        setWindowTitle(tr("影像處理工具[*]"));
    else
        setWindowTitle(tr("%1[*] - 影像處理工具").arg(QFileInfo(m_filePath).fileName()));
}

void MainWindow::updateStatusInfo()
{
    if (m_image.isNull())
        m_sizeLabel->clear();
    else
        m_sizeLabel->setText(tr("%1 × %2 px").arg(m_image.width()).arg(m_image.height()));
}

bool MainWindow::maybeSave()
{
    if (!isWindowModified())
        return true;

    const QMessageBox::StandardButton ret = QMessageBox::warning(
        this, tr("尚未儲存"), tr("圖片已經修改過，要先儲存嗎？"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    switch (ret) {
    case QMessageBox::Save:
        return saveImage();
    case QMessageBox::Discard:
        return true;
    default:
        return false; // Cancel：留在原畫面
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave())
        event->accept();
    else
        event->ignore();
}
