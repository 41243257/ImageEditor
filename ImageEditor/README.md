# ImageEditor：Week 10 主視窗骨架

使用 Qt Widgets 開發的桌面影像處理程式第一版，依照 Week 10 最低功能需求建立主視窗架構。
Qt 5 和 Qt 6 都可以編譯。介面完全用程式碼建立，沒有使用 .ui 檔。

## 檔案

| 檔案 | 內容 |
|---|---|
| `main.cpp` | 程式進入點，建立並顯示主視窗 |
| `mainwindow.h` / `mainwindow.cpp` | 主視窗：選單、工具列、動作、工作區、狀態列 |
| `CMakeLists.txt` | CMake 專案檔 |
| `ImageEditor.pro` | qmake 專案檔（和 CMakeLists.txt 二選一） |

## 開啟與執行（Qt Creator）

1. Qt Creator →「檔案 > 開啟檔案或專案」，選 `CMakeLists.txt`（或 `ImageEditor.pro`）
2. 第一次開啟時選擇 Kit（例如 Desktop Qt 6.x MinGW 64-bit），按「Configure Project」
3. 按左下角綠色三角形（或 Qt Creator 的 Ctrl+R）建置並執行

## Week 10 需求對照

| 最低需求 | 程式中的位置 |
|---|---|
| 建立 QMainWindow 主視窗框架 | `MainWindow` 繼承 `QMainWindow` |
| File 與 Image 選單 | `createMenus()` |
| Open、Save、Exit 等 QAction | `createActions()`（另外加了 Save As） |
| 常用 QAction 同時出現在 ToolBar | `createToolBar()`，和選單共用同一個 QAction |
| centralWidget 作為影像工作區 | `createCentralArea()`：QScrollArea + QLabel |
| QAction 有 icon 和 shortcut | `createActions()` 中每個動作都有設定 |
| 可 Build、Run，結構清楚 | 介面依功能分成五個 `create...()` 函式 |

## 快捷鍵

| 動作 | 快捷鍵 |
|---|---|
| Open | Ctrl+O |
| Save | Ctrl+S |
| Save As | Ctrl+Shift+S |
| Exit | Ctrl+Q |
| Rotate Left 90° | Ctrl+L |
| Rotate Right 90° | Ctrl+R |

## 額外做的使用者體驗細節

- 還沒開圖時，工作區顯示提示文字，Save 和旋轉按鈕會灰掉
- 狀態列右下角顯示圖片尺寸（寬 × 高）
- 圖片修改後標題列出現 `*`；關閉、離開或開新圖前會詢問是否儲存
- 存成 JPG 時，透明的部分會填成白色
- 手機照片會依 EXIF 方向自動轉正

## 之後加功能

新的影像處理功能（調整解析度、切割、濾鏡…）照這個順序加：

1. 在 `createActions()` 建立 QAction（設定 icon 和 shortcut）
2. 在 `createMenus()` 加到 Image 選單；常用的再加到 `createToolBar()`
3. 在 `updateActions()` 設定沒開圖時要停用
4. 寫對應的 slot，處理完呼叫 `setImage()` 和 `setWindowModified(true)`
