#include "widgets/mainwindow.h"

#include <QApplication>
#include <QDir>

#include <QtCrypto/QtCrypto>
#include <log4qt/propertyconfigurator.h>
#include <log4qt/logger.h>

#include "constants.h"

int main(int argc, char* argv[])
{
    Q_INIT_RESOURCE(resources);

    QApplication a(argc, argv);

    QDir basePath(QCoreApplication::applicationDirPath());
    //first of all, initialize Log4qt
    Log4Qt::PropertyConfigurator::configure(basePath.absoluteFilePath("etc/log4j.properties"));
    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("The application's working directory: %1", QDir::currentPath());
    }
    QCoreApplication::addLibraryPath(basePath.absoluteFilePath("plugins"));
    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Application plugins library paths: %1", QCoreApplication::libraryPaths().join(";"));
    }
    QCA::Initializer init;
    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        QStringList paths = QCA::pluginPaths();
        Q_FOREACH (const QString& path, paths)
            Log4Qt::Logger::logger("main")->debug("QCA plugins path: %1", QDir::toNativeSeparators(path));

        QCA::ProviderList list = QCA::providers();
        for (int n = 0; n < list.count(); ++n)
        {
            QString str = QCA::pluginDiagnosticText();
            Log4Qt::Logger::logger("main")->debug("Available Plugin Diagnostic: %1", str);
            QCA::clearPluginDiagnosticText();

            Log4Qt::Logger::logger("main")->debug("Available Providers: %1", list[n]->name());
            QString credit = list[n]->credit();
            Log4Qt::Logger::logger("main")->debug("Available Providers Credit: %1", credit);
        }
        QStringList features = QCA::supportedFeatures();
        Q_FOREACH (const QString& feature, features)
            Log4Qt::Logger::logger("main")->debug("QCA feature: %1", feature);
    }

    QCoreApplication::setApplicationName(APPLICATION_NAME);
    QCoreApplication::setOrganizationName(ORGANIZATION_NAME);
    QCoreApplication::setApplicationVersion(APPLICATION_VERSION);

    MainWindow mainwindow;
    emit mainwindow.loadClusters();
    mainwindow.show();

    return a.exec();
}
