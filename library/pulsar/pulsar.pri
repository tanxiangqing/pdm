
INCLUDEPATH += $$PWD/include

PULSAR_WIN32_ROOT = $$PWD/lib/win32
PULSAR_UNIX_ROOT = $$PWD/lib/unix
PULSAR_MACX_ROOT = $$PWD/lib/macx

win32:CONFIG(release, debug|release): LIBS += -L$${PULSAR_WIN32_ROOT} -lpulsar
else:win32:CONFIG(debug, debug|release): LIBS += -L$${PULSAR_WIN32_ROOT} -lpulsar
else:unix:!macx: LIBS += -L$${PULSAR_UNIX_ROOT} -lpulsar
else:unix:macx: LIBS += -L$${PULSAR_MACX_ROOT} -lpulsar
