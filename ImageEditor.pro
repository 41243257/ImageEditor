# qmake 版本的專案檔（和 CMakeLists.txt 二選一即可）
QT += core gui widgets
CONFIG += c++17

TARGET = ImageEditor
TEMPLATE = app

# MSVC 預設不把原始碼當成 UTF-8，中文字會變亂碼
msvc: QMAKE_CXXFLAGS += /utf-8

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

# icon 圖片放在資源檔裡，編譯後會包進執行檔
RESOURCES += \
    images/designwindow.qrc
