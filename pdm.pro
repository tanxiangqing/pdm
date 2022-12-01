QT       += core gui network core5compat

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/basemodel.cpp \
    src/consumer.cpp \
    src/cursor.cpp \
    src/function.cpp \
    src/main.cpp \
    src/message.cpp \
    src/namespace.cpp \
    src/cluster.cpp \
    src/producer.cpp \
    src/pulsarmessage.cpp \
    src/qjsonwebtoken.cpp \
    src/qmulticombobox.cpp \
    src/role.cpp \
    src/services/baseservice.cpp \
    src/services/clusterservice.cpp \
    src/services/cursorservice.cpp \
    src/services/functionservice.cpp \
    src/services/httpclient.cpp \
    src/services/namespaceservice.cpp \
    src/services/permissionservice.cpp \
    src/services/prestoqueryservice.cpp \
    src/services/sinkservice.cpp \
    src/services/sourceservice.cpp \
    src/services/tenantservice.cpp \
    src/services/tokenservice.cpp \
    src/services/topicservice.cpp \
    src/sink.cpp \
    src/source.cpp \
    src/subscription.cpp \
    src/table.cpp \
    src/tenant.cpp \
    src/token.cpp \
    src/topic.cpp \
    src/widgets/baseinputwindow.cpp \
    src/widgets/basemdisubwindow.cpp \
    src/widgets/functioninstanceswindow.cpp \
    src/widgets/functionswindow.cpp \
    src/widgets/lastcommitmessagewindow.cpp \
    src/widgets/mainwindow.cpp \
    src/widgets/mdisubwindow.cpp \
    src/widgets/namespacewindow.cpp \
    src/widgets/newclusterwindow.cpp \
    src/widgets/newfunctionwindow.cpp \
    src/widgets/newtopicwindow.cpp \
    src/widgets/peekmessageswindow.cpp \
    src/widgets/permissionswindow.cpp \
    src/widgets/querytopicdatawindow.cpp \
    src/widgets/sendmessagewindow.cpp \
    src/widgets/sinkinputwindow.cpp \
    src/widgets/sinkswindow.cpp \
    src/widgets/sourceinputwindow.cpp \
    src/widgets/sourceswindow.cpp \
    src/widgets/subscriptionwindow.cpp \
    src/widgets/tenantwindow.cpp \
    src/widgets/tokenwindow.cpp \
    src/widgets/topicoverviewwindow.cpp \
    src/widgets/topicstoragewindow.cpp \
    src/widgets/topicswindow.cpp

HEADERS += \
    src/basemodel.h \
    src/constants.h \
    src/consumer.h \
    src/cursor.h \
    src/function.h \
    src/functioninstance.h \
    src/message.h \
    src/basefunction.h \
    src/namespace.h \
    src/cluster.h \
    src/producer.h \
    src/pulsarmessage.h \
    src/qjsonwebtoken.h \
    src/qmulticombobox.h \
    src/role.h \
    src/services/baseservice.h \
    src/services/clusterservice.h \
    src/services/cursorservice.h \
    src/services/functionservice.h \
    src/services/httpclient.h \
    src/services/namespaceservice.h \
    src/services/permissionservice.h \
    src/services/prestoqueryservice.h \
    src/services/sinkservice.h \
    src/services/sourceservice.h \
    src/services/tenantservice.h \
    src/services/tokenservice.h \
    src/services/topicservice.h \
    src/sink.h \
    src/source.h \
    src/subscription.h \
    src/table.h \
    src/tenant.h \
    src/token.h \
    src/topic.h \
    src/widgets/baseinputwindow.h \
    src/widgets/basemdisubwindow.h \
    src/widgets/functioninstanceswindow.h \
    src/widgets/functionswindow.h \
    src/widgets/lastcommitmessagewindow.h \
    src/widgets/mainwindow.h \
    src/widgets/mdisubwindow.h \
    src/widgets/namespacewindow.h \
    src/widgets/newclusterwindow.h \
    src/widgets/newfunctionwindow.h \
    src/widgets/newtopicwindow.h \
    src/widgets/peekmessageswindow.h \
    src/widgets/permissionswindow.h \
    src/widgets/querytopicdatawindow.h \
    src/widgets/sendmessagewindow.h \
    src/widgets/sinkinputwindow.h \
    src/widgets/sinkswindow.h \
    src/widgets/sourceinputwindow.h \
    src/widgets/sourceswindow.h \
    src/widgets/subscriptionwindow.h \
    src/widgets/tenantwindow.h \
    src/widgets/tokenwindow.h \
    src/widgets/topicoverviewwindow.h \
    src/widgets/topicstoragewindow.h \
    src/widgets/topicswindow.h

DESTDIR = $$PWD/build

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32{
    RC_ICONS = $$PWD/resource/icons/pulsar.ico
}

unix:macx { # OSX
    TARGET = "PDM"
    ICON = $$PWD/resource/icons/logo.icns
    CONFIG += sdk_no_version_check

    CONFIG(release, debug|release) {
        QMAKE_RPATHDIR += @executable_path/../Frameworks
        QMAKE_RPATHDIR += @loader_path/../Frameworks
    }


    #include($$PWD/libs/avro/avro.pri)

    #SOURCES += \
    #   src/avromessage.cpp

    #HEADERS += \
    #       src/avromessage.h \
    #       src/user.hh

    #INCLUDEPATH += /usr/local/include
}

include($$PWD/library/log4qt/log4qt.pri)
include($$PWD/library/qca/qca.pri)
include($$PWD/library/pulsar/pulsar.pri)

RESOURCES += \
    resource/resources.qrc
