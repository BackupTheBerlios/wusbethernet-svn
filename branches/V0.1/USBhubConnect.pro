TEMPLATE = app
TARGET = USBhubConnect
CODECFORTR = UTF-8
QT += core \
    network \
    xml \
    gui
HEADERS += src/TI_USBhub.h \
    src/TI_WusbStack.h \
    src/config.h \
    src/AboutBox.h \
    src/utils/LogFileAppender.h \
    src/utils/LogConsoleAppender.h \
    src/utils/LogAppender.h \
    src/utils/Logger.h \
    src/utils/LogWriter.h \
    src/azurewave/HubDevice.h \
    src/azurewave/WusbHelperLib.h \
    src/azurewave/WusbMessageBuffer.h \
    src/azurewave/WusbReceiverThread.h \
    src/azurewave/WusbStack.h \
    src/azurewave/XMLmessageDOMparser.h \
    src/azurewave/ConnectionController.h \
    src/azurewave/ControlMessageBuffer.h \
    src/preferencesbox.h \
    src/Textinfoview.h \
    src/USBinfoTables.h \
    src/USBdeviceInfoProducer.h \
    src/BasicUtils.h \
    src/USBconnectionWorker.h \
    src/USButils.h \
    src/ConfigManager.h \
    src/mainframe.h
SOURCES += src/AboutBox.cpp \
    src/utils/LogFileAppender.cpp \
    src/utils/LogConsoleAppender.cpp \
    src/utils/Logger.cpp \
    src/utils/LogWriter.cpp \
    src/azurewave/HubDevice.cpp \
    src/azurewave/WusbHelperLib.cpp \
    src/azurewave/WusbMessageBuffer.cpp \
    src/azurewave/WusbReceiverThread.cpp \
    src/azurewave/WusbStack.cpp \
    src/azurewave/XMLmessageDOMparser.cpp \
    src/azurewave/ConnectionController.cpp \
    src/azurewave/ControlMessageBuffer.cpp \
    src/preferencesbox.cpp \
    src/Textinfoview.cpp \
    src/USBinfoTables.cpp \
    src/USBdeviceInfoProducer.cpp \
    src/BasicUtils.cpp \
    src/USBconnectionWorker.cpp \
    src/USButils.cpp \
    src/ConfigManager.cpp \
    src/mainframe.cpp \
    src/main.cpp
FORMS += src/AboutBox.ui \
    src/preferencesbox.ui \
    src/Textinfoview.ui \
    src/mainframe.ui
RESOURCES += src/Resources.qrc
TRANSLATIONS = i18n/USBhubConnect_en.ts \
    i18n/USBhubConnect_de.ts
