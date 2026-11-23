#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QImage>
#include <QMainWindow>

class QAction;
class QLabel;
class QScrollArea;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void openImage();
    bool saveImage();
    bool saveImageAs();
    void rotateLeft();
    void rotateRight();

private:
    // 建立介面（依 Week 10 需求分成五塊）
    void createActions();     // QAction：Open / Save / Exit ...
    void createMenus();       // File、Image 選單
    void createToolBar();     // 常用 QAction 放到工具列
    void createCentralArea(); // centralWidget：影像工作區
    void createStatusBar();   // 狀態列：顯示訊息與圖片尺寸

    // 影像與狀態處理
    void setImage(const QImage &image);
    bool writeImage(const QString &filePath);
    bool maybeSave();
    void rotate(int degrees);
    void updateActions();
    void updateWindowTitle();
    void updateStatusInfo();

    // 影像資料
    QImage m_image;
    QString m_filePath;

    // 中央工作區
    QScrollArea *m_scrollArea = nullptr;
    QLabel *m_imageLabel = nullptr;

    // 狀態列右側的尺寸資訊
    QLabel *m_sizeLabel = nullptr;

    // File 選單的動作
    QAction *m_openAct = nullptr;
    QAction *m_saveAct = nullptr;
    QAction *m_saveAsAct = nullptr;
    QAction *m_exitAct = nullptr;

    // Image 選單的動作
    QAction *m_rotateLeftAct = nullptr;
    QAction *m_rotateRightAct = nullptr;
};

#endif // MAINWINDOW_H
