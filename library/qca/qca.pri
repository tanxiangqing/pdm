QCA_WIN32_ROOT = $$PWD/win32
QCA_UNIX_ROOT = $$PWD/unix
QCA_MACX_ROOT = $$PWD/macx

win32{
    INCLUDEPATH += $${QCA_WIN32_ROOT}/include
}

unix:!macx{
    INCLUDEPATH += $${QCA_UNIX_ROOT}/include
}

unix:macx{
    INCLUDEPATH += $${QCA_MACX_ROOT}/include
}

win32:CONFIG(release, debug|release):LIBS += -L$${QCA_WIN32_ROOT}/lib -lqca
else:win32:CONFIG(debug, debug|release):LIBS += -L$${QCA_WIN32_ROOT}/lib -lqcad
else:unix:!macx:LIBS += -L$${QCA_UNIX_ROOT}/lib -lqca
else:unix:macx:LIBS += -F$${LOG4QT_MACX_ROOT}/lib -framework qca
