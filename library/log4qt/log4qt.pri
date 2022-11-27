LOG4QT_WIN32_ROOT = $$PWD/win32
LOG4QT_UNIX_ROOT = $$PWD/unix
LOG4QT_MACX_ROOT = $$PWD/macx

win32{
	INCLUDEPATH += $${LOG4QT_WIN32_ROOT}/include
}

unix:!macx{
	INCLUDEPATH += $${LOG4QT_UNIX_ROOT}/include
}

unix:macx{
	INCLUDEPATH += $${LOG4QT_MACX_ROOT}/include
}

win32:CONFIG(release, debug|release):LIBS += -L$${LOG4QT_WIN32_ROOT}/lib -llog4qt
else:win32:CONFIG(debug, debug|release): LIBS += -L$${LOG4QT_WIN32_ROOT}/lib -llog4qt_d
else:unix:!macx:LIBS += -L$${LOG4QT_UNIX_ROOT}/lib -llog4qt
else:unix:macx:LIBS += -L$${LOG4QT_MACX_ROOT}/lib -llog4qt
