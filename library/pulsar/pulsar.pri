PULSAR_WIN32_ROOT = $$PWD/win32
PULSAR_UNIX_ROOT = $$PWD/unix
PULSAR_MACX_ROOT = $$PWD/macx

win32{
    INCLUDEPATH += $${PULSAR_WIN32_ROOT}/include
}

unix:!macx{
    INCLUDEPATH += $${PULSAR_UNIX_ROOT}/include
}

unix:macx{
    INCLUDEPATH += $${PULSAR_MACX_ROOT}/include
}

win32:CONFIG(release, debug|release): LIBS += -L$${PULSAR_WIN32_ROOT}/lib -lpulsar
else:win32:CONFIG(debug, debug|release): LIBS += -L$${PULSAR_WIN32_ROOT}/lib -lpulsar
else:unix:!macx: LIBS += -L$${PULSAR_UNIX_ROOT}/lib -lpulsar
else:unix:macx: LIBS += -L$${PULSAR_MACX_ROOT}/lib -lpulsar
