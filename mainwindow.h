#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QImage>
#include <QList>
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
    // File
    void newImage();
    void openImage();
    bool saveImage();
    bool saveImageAs();
    // Edit
    void undo();
    void copyImage();
    void pasteImage();
    // Image
    void rotateLeft();
    void rotateRight();

private:
    // 建立介面（依 Week 10 需求分成五塊）
    void createActions();     // QAction：New / Open / Save / Exit ...
    void createMenus();       // File、Edit、Image 選單
    void createToolBar();     // 常用 QAction 放到工具列
    void createCentralArea(); // centralWidget：影像工作區
    void createStatusBar();   // 狀態列：顯示訊息與圖片尺寸

    // 影像與狀態處理
    void setImage(const QImage &image);
    void applyChange(const QImage &newImage); // 修改圖片（可以復原）
    void resetHistory();                      // 開新圖時清空復原紀錄
    bool writeImage(const QString &filePath);
    bool maybeSave();
    void rotate(int degrees);
    void updateActions();
    void updateModified();
    void updatePasteAction();
    void updateWindowTitle();
    void updateStatusInfo();

    // 影像資料
    QImage m_image;
    QString m_filePath;

    // 復原紀錄：每次修改前把舊圖存起來
    static constexpr int kMaxUndo = 10; // 最多保留 10 步，避免大圖吃光記憶體
    QList<QImage> m_undoStack;
    int m_cleanIndex = 0; // 上次存檔時 m_undoStack 的長度；-1 表示回不到存檔狀態

    // 中央工作區
    QScrollArea *m_scrollArea = nullptr;
    QLabel *m_imageLabel = nullptr;

    // 狀態列右側的尺寸資訊
    QLabel *m_sizeLabel = nullptr;

    // File 選單的動作
    QAction *m_newAct = nullptr;
    QAction *m_openAct = nullptr;
    QAction *m_saveAct = nullptr;
    QAction *m_saveAsAct = nullptr;
    QAction *m_exitAct = nullptr;

    // Edit 選單的動作
    QAction *m_undoAct = nullptr;
    QAction *m_copyAct = nullptr;
    QAction *m_pasteAct = nullptr;

    // Image 選單的動作
    QAction *m_rotateLeftAct = nullptr;
    QAction *m_rotateRightAct = nullptr;
};

#endif // MAINWINDOW_H
