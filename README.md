# ImageEditor：Week 10 主視窗骨架

使用 Qt Widgets 開發的桌面影像處理程式第一版，依照 Week 10 最低功能需求建立主視窗架構。
Qt 5 和 Qt 6 都可以編譯。介面完全用程式碼建立，沒有使用 .ui 檔。

## 檔案

| 檔案 | 內容 |
|---|---|
| `main.cpp` | 程式進入點，設定視窗圖示並顯示主視窗 |
| `mainwindow.h` / `mainwindow.cpp` | 主視窗：選單、工具列、動作、工作區、狀態列 |
| `images/designwindow.qrc` | 老師提供的資源檔（prefix 為 `images`） |
| `images/*.png` | 老師提供的 icon |
| `CMakeLists.txt` | CMake 專案檔 |
| `ImageEditor.pro` | qmake 專案檔（和 CMakeLists.txt 二選一） |

## 開啟與執行（Qt Creator）

1. Qt Creator →「檔案 > 開啟檔案或專案」，選 `CMakeLists.txt`（或 `ImageEditor.pro`）
2. 第一次開啟時選擇 Kit（例如 Desktop Qt 6.x MinGW 64-bit），按「Configure Project」
3. 按左下角綠色三角形建置並執行

## Week 10 需求對照

| 最低需求 | 程式中的位置 |
|---|---|
| 建立 QMainWindow 主視窗框架 | `MainWindow` 繼承 `QMainWindow` |
| File 與 Image / Edit 選單 | `createMenus()`：File、Edit、Image 三個選單 |
| Open、Save、Exit 等 QAction | `createActions()`：New、Open、Save、Save As、Exit、Undo、Copy、Paste、旋轉 |
| 常用 QAction 同時出現在 ToolBar | `createToolBar()`，和選單共用同一個 QAction |
| centralWidget 作為影像工作區 | `createCentralArea()`：QScrollArea + QLabel |
| QAction 有 icon 和 shortcut | `createActions()`，icon 來自 `designwindow.qrc` |
| 可 Build、Run，結構清楚 | 介面依功能分成五個 `create...()` 函式 |

## 選單、icon 與快捷鍵

| 選單 | 動作 | icon | 快捷鍵 | 功能 |
|---|---|---|---|---|
| File | New | `new.png` | Ctrl+N | 建立 800 × 600 白色空白畫布 |
| File | Open Image | `open.png` | Ctrl+O | 開啟圖片（選單和工具列共用同一個 QAction） |
| File | Save | `save.png` | Ctrl+S | 存回原檔（沒有檔名時改成另存新檔） |
| File | Save As | `asave.png` | Ctrl+Shift+S | 另存新檔 |
| File | Exit | Qt 內建 | Ctrl+Q | 離開程式 |
| Edit | Undo | `undo.png` | Ctrl+Z | 復原上一步（最多 10 步） |
| Edit | Copy | `copy.png` | Ctrl+C | 把整張圖複製到剪貼簿 |
| Edit | Paste | `paste.png` | Ctrl+V | 用剪貼簿的圖片取代目前圖片（可復原） |
| Image | Rotate Left 90° | 程式自己畫 | Ctrl+L | 逆時針旋轉 90° |
| Image | Rotate Right 90° | 程式自己畫 | Ctrl+R | 順時針旋轉 90° |

- `editor.png` 用在視窗左上角和工作列的程式圖示
- `cut.png`、`find.png` 目前還沒用到：沒有「選取範圍」就無法剪下，影像程式也很少用到「尋找」。
  之後做「切割」功能時可以用 `cut.png`，做「放大檢視」時可以用 `find.png`
- `Thumbs.db` 是 Windows 自動產生的縮圖快取，不是 icon，不需要放進專案

## 額外做的使用者體驗細節

- 還沒開圖時，工作區顯示提示文字，Save、Copy、旋轉等按鈕會灰掉；剪貼簿沒有圖片時 Paste 會灰掉
- 狀態列右下角顯示圖片尺寸（寬 × 高）
- 圖片修改後標題列出現 `*`；關閉、離開、開新圖前會詢問是否儲存
- 復原到和存檔時一樣的狀態，`*` 會自動消失
- 存成 JPG 時，透明的部分會填成白色
- 手機照片會依 EXIF 方向自動轉正

## 之後加功能

新的影像處理功能（調整解析度、切割、濾鏡…）照這個順序加：

1. 在 `createActions()` 用 `makeAction()` 建立 QAction（設定 icon 和 shortcut）
2. 在 `createMenus()` 加到 Image 選單；常用的再加到 `createToolBar()`
3. 在 `updateActions()` 設定沒開圖時要停用
4. 寫對應的 slot，處理完呼叫 `applyChange(新圖片)`，就會自動支援 Undo 和未存檔提示
