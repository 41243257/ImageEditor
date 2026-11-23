#include "mainwindow.h"

#include <QAction>
#include <QClipboard>
#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImageReader>
#include <QImageWriter>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
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

// 建立一個 QAction，一次設定好 icon、快捷鍵和提示文字
QAction *makeAction(QObject *parent, const QIcon &icon, const QString &text,
                    const QString &shortcut, const QString &toolTip, const QString &statusTip)
{
    auto *action = new QAction(icon, text, parent);
    action->setShortcut(QKeySequence(shortcut));
    action->setToolTip(QStringLiteral("%1 (%2)").arg(toolTip, shortcut)); // 滑鼠停在工具列按鈕上時顯示
    action->setStatusTip(statusTip);                                       // 顯示在狀態列
    return action;
}

// 老師給的 icon 裡沒有「旋轉」，所以自己畫一個順時針的圓弧箭頭 ↻
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

    // 剪貼簿內容改變時，重新判斷 Paste 能不能按
    connect(QGuiApplication::clipboard(), &QClipboard::dataChanged,
            this, &MainWindow::updatePasteAction);

    updateActions();
    updateWindowTitle();
    resize(1000, 700);
}

// ───────────────────────── 建立介面 ─────────────────────────

void MainWindow::createActions()
{
    // icon 圖片放在 images/designwindow.qrc（prefix 是 images），路徑寫成「:/images/檔名」

    // ── File ──
    m_newAct = makeAction(this, QIcon(":/images/new.png"), tr("&New"), "Ctrl+N",
                          tr("新增空白畫布"), tr("建立一張 800 × 600 的白色空白圖片"));
    connect(m_newAct, &QAction::triggered, this, &MainWindow::newImage);

    // Open Image：同一個 QAction 會同時加到 File 選單和工具列（見 createMenus、createToolBar）
    m_openAct = makeAction(this, QIcon(":/images/open.png"), tr("&Open Image..."), "Ctrl+O",
                           tr("開啟圖片"), tr("開啟電腦中的圖片檔"));
    connect(m_openAct, &QAction::triggered, this, &MainWindow::openImage);

    m_saveAct = makeAction(this, QIcon(":/images/save.png"), tr("&Save"), "Ctrl+S",
                           tr("儲存"), tr("把目前的圖片存回原本的檔案"));
    connect(m_saveAct, &QAction::triggered, this, &MainWindow::saveImage);

    m_saveAsAct = makeAction(this, QIcon(":/images/asave.png"), tr("Save &As..."), "Ctrl+Shift+S",
                             tr("另存新檔"), tr("把目前的圖片存成新的檔案"));
    connect(m_saveAsAct, &QAction::triggered, this, &MainWindow::saveImageAs);

    // 老師給的 icon 裡沒有 Exit，用 Qt 內建的標準圖示
    m_exitAct = makeAction(this, style()->standardIcon(QStyle::SP_DialogCloseButton), tr("E&xit"),
                           "Ctrl+Q", tr("離開程式"), tr("關閉程式"));
    // close() 會觸發 closeEvent()，所以離開前一樣會詢問是否儲存
    connect(m_exitAct, &QAction::triggered, this, &QWidget::close);

    // ── Edit ──
    m_undoAct = makeAction(this, QIcon(":/images/undo.png"), tr("&Undo"), "Ctrl+Z",
                           tr("復原"), tr("回到上一步的圖片"));
    connect(m_undoAct, &QAction::triggered, this, &MainWindow::undo);

    m_copyAct = makeAction(this, QIcon(":/images/copy.png"), tr("&Copy"), "Ctrl+C",
                           tr("複製"), tr("把整張圖片複製到剪貼簿"));
    connect(m_copyAct, &QAction::triggered, this, &MainWindow::copyImage);

    m_pasteAct = makeAction(this, QIcon(":/images/paste.png"), tr("&Paste"), "Ctrl+V",
                            tr("貼上"), tr("用剪貼簿裡的圖片取代目前的圖片"));
    connect(m_pasteAct, &QAction::triggered, this, &MainWindow::pasteImage);

    // ── Image ──
    const QIcon rotateRightIcon = makeRotateRightIcon();

    m_rotateLeftAct = makeAction(this, mirroredIcon(rotateRightIcon), tr("Rotate &Left 90°"), "Ctrl+L",
                                 tr("向左旋轉 90°"), tr("把圖片逆時針旋轉 90 度"));
    connect(m_rotateLeftAct, &QAction::triggered, this, &MainWindow::rotateLeft);

    m_rotateRightAct = makeAction(this, rotateRightIcon, tr("Rotate &Right 90°"), "Ctrl+R",
                                  tr("向右旋轉 90°"), tr("把圖片順時針旋轉 90 度"));
    connect(m_rotateRightAct, &QAction::triggered, this, &MainWindow::rotateRight);
}

void MainWindow::createMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(m_newAct);
    fileMenu->addAction(m_openAct);
    fileMenu->addAction(m_saveAct);
    fileMenu->addAction(m_saveAsAct);
    fileMenu->addSeparator();
    fileMenu->addAction(m_exitAct);

    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(m_undoAct);
    editMenu->addSeparator();
    editMenu->addAction(m_copyAct);
    editMenu->addAction(m_pasteAct);

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
    toolBar->addAction(m_newAct);
    toolBar->addAction(m_openAct);
    toolBar->addAction(m_saveAct);
    toolBar->addSeparator();
    toolBar->addAction(m_undoAct);
    toolBar->addSeparator();
    toolBar->addAction(m_copyAct);
    toolBar->addAction(m_pasteAct);
    toolBar->addSeparator();
    toolBar->addAction(m_rotateLeftAct);
    toolBar->addAction(m_rotateRightAct);
}

void MainWindow::createCentralArea()
{
    // 圖片顯示在 QLabel 上；外面包一層 QScrollArea，圖比視窗大時可以捲動
    m_imageLabel = new QLabel;
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setText(tr("尚未開啟圖片\n\n請從 File > Open 開啟（Ctrl+O），或按 Ctrl+N 新增空白畫布"));

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

void MainWindow::newImage()
{
    if (!maybeSave())
        return;

    QImage blank(800, 600, QImage::Format_RGB32);
    blank.fill(Qt::white);

    m_filePath.clear();
    setImage(blank);
    resetHistory();
    updateWindowTitle();
    statusBar()->showMessage(tr("已建立新的空白畫布"), 3000);
}

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
    resetHistory();
    updateWindowTitle();
    statusBar()->showMessage(tr("已開啟 %1").arg(QFileInfo(filePath).fileName()), 3000);
}

bool MainWindow::saveImage()
{
    if (m_image.isNull())
        return false;
    if (m_filePath.isEmpty()) // 新畫布或貼上的圖還沒有檔名
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

    m_cleanIndex = int(m_undoStack.size()); // 記住「存檔時」在第幾步
    updateModified();
    statusBar()->showMessage(tr("已儲存 %1").arg(QFileInfo(filePath).fileName()), 3000);
    return true;
}

// ───────────────────────── Edit 功能 ─────────────────────────

void MainWindow::undo()
{
    if (m_undoStack.isEmpty())
        return;

    setImage(m_undoStack.takeLast());
    updateModified();
    statusBar()->showMessage(tr("已復原"), 2000);
}

void MainWindow::copyImage()
{
    if (m_image.isNull())
        return;

    QGuiApplication::clipboard()->setImage(m_image);
    statusBar()->showMessage(tr("已把圖片複製到剪貼簿"), 2000);
}

void MainWindow::pasteImage()
{
    const QImage pasted = QGuiApplication::clipboard()->image();
    if (pasted.isNull()) {
        statusBar()->showMessage(tr("剪貼簿裡沒有圖片"), 3000);
        return;
    }

    if (m_image.isNull()) {
        // 還沒開圖：貼上的圖當成一張新的、還沒存檔的圖片
        m_filePath.clear();
        setImage(pasted);
        resetHistory();
        m_cleanIndex = -1; // 從沒存過檔，關閉時要詢問
        updateModified();
        updateWindowTitle();
    } else {
        applyChange(pasted); // 取代目前的圖片，可以按 Undo 復原
    }
    statusBar()->showMessage(tr("已貼上圖片（%1 × %2）").arg(pasted.width()).arg(pasted.height()), 3000);
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
    applyChange(m_image.transformed(transform));
}

// ───────────────────────── 復原紀錄 ─────────────────────────

void MainWindow::applyChange(const QImage &newImage)
{
    // 已經復原到存檔之前、又做了新修改 → 再也回不到存檔時的樣子
    if (m_undoStack.size() < m_cleanIndex)
        m_cleanIndex = -1;

    m_undoStack.append(m_image); // 修改前先把舊圖存起來
    if (m_undoStack.size() > kMaxUndo) {
        m_undoStack.removeFirst(); // 太多步了，丟掉最舊的
        if (m_cleanIndex >= 0)
            --m_cleanIndex;
    }

    setImage(newImage);
    updateModified();
}

void MainWindow::resetHistory()
{
    m_undoStack.clear();
    m_cleanIndex = 0;
    updateModified();
}

void MainWindow::updateModified()
{
    // 目前的步數和存檔時不同，就代表有沒存的修改 → 標題列顯示 *
    setWindowModified(m_undoStack.size() != m_cleanIndex);
    m_undoAct->setEnabled(!m_undoStack.isEmpty());
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
    // 沒開圖時，存檔、複製和旋轉都不能用（選單和工具列會一起灰掉）
    const bool hasImage = !m_image.isNull();
    m_saveAct->setEnabled(hasImage);
    m_saveAsAct->setEnabled(hasImage);
    m_copyAct->setEnabled(hasImage);
    m_rotateLeftAct->setEnabled(hasImage);
    m_rotateRightAct->setEnabled(hasImage);
    m_undoAct->setEnabled(!m_undoStack.isEmpty());
    updatePasteAction();
}

void MainWindow::updatePasteAction()
{
    // 剪貼簿裡有圖片才能貼上
    const QMimeData *data = QGuiApplication::clipboard()->mimeData();
    m_pasteAct->setEnabled(data && data->hasImage());
}

void MainWindow::updateWindowTitle()
{
    // [*] 是 Qt 的佔位符號：setWindowModified(true) 時會顯示成 *
    if (m_image.isNull()) {
        setWindowTitle(tr("影像處理工具[*]"));
        return;
    }
    const QString name = m_filePath.isEmpty() ? tr("未命名") : QFileInfo(m_filePath).fileName();
    setWindowTitle(tr("%1[*] - 影像處理工具").arg(name));
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
