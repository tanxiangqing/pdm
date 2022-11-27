#include "sinkservice.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QHttpPart>
#include <QFileInfo>
#include <QMimeDatabase>

#include <log4qt/logger.h>

#include "../constants.h"

QList<Sink> SinkService::sinks(const Namespace& _namespace) const
{
    QList<Sink> sinks;

    QStringList names = sinkNames(_namespace);
    Q_FOREACH (const QString& name, names)
    {
        Sink sink;
        sink.setNamespace(_namespace);
        sink.appendInfo(information(_namespace, name));
        sink.appendStatus(status(_namespace, name));
        sinks << sink;
    }

    return sinks;
}

void SinkService::create(const Sink& _sink, QFile* _file, HttpStatusCode& _code)
{
    QByteArray json = _sink.toJson();

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Greate a new Sink request body: %1", QString::fromLatin1(json));
    }

    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    //配置部分
    QHttpPart configPart;
    configPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));
    configPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"sinkConfig\""));
    configPart.setBody(json);
    //文件部分
    QHttpPart filePart;
    QFileInfo fileInfo(_file->fileName());
    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(fileInfo);
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(type.name()));
    QString header("form-data; name=\"data\"; filename=\"%1\"");
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(header.arg(fileInfo.fileName())));
    _file->open(QIODevice::ReadOnly);
    _file->setParent(multiPart);
    filePart.setBodyDevice(_file);
    multiPart->append(filePart);
    multiPart->append(configPart);

    QString path(_sink.getNamespace().tenant().cluster().functionUrl());
    path += this->m_Settings->value(PUT_NEW_SINK_PATH_KEY).toString();
    QUrl url(path.arg(_sink.getNamespace().tenant().name(), _sink.getNamespace().name(), _sink.name()));

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Greate a new Sink service url: %1", url.toString());
    }

    int code;
    QByteArray result = this->m_Client->post(url, multiPart, code);
    switch (code)
    {
    case 204:
        _code.code = HttpStatusCode::StatusCode::NoContent;
        break;
    case 400:
        _code.code = HttpStatusCode::StatusCode::BadRequest;
        _code.errorDesc = QString::fromLatin1(result);
        break;
    case 401:
        _code.code = HttpStatusCode::StatusCode::Unauthorized;
        _code.errorDesc = QString(tr("Client is not authorize to perform operation."));
        break;
    case 503:
        _code.code = HttpStatusCode::StatusCode::ServiceUnavailable;
        _code.errorDesc = QString(tr("Function worker service is now initializing. Please try again later."));
        break;
    case 500:
        _code.code = HttpStatusCode::StatusCode::InternalServerError;
        _code.errorDesc = QString(tr("Internal Server Error."));
        break;
    }

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Greate a new Sink response result: %1, code: %2", QString::fromLatin1(result), code);
    }
}

void SinkService::update(const Sink& _sink, QFile* _file, HttpStatusCode& _code)
{
    QByteArray json = _sink.toJson();

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Update Sink request body: %1", QString::fromLatin1(json));
    }

    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    //配置部分
    QHttpPart configPart;
    configPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));
    configPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"sinkConfig\""));
    configPart.setBody(json);
    multiPart->append(configPart);
    if (_file != Q_NULLPTR)
    {
        QHttpPart filePart;
        QFileInfo fileInfo(_file->fileName());
        QMimeDatabase db;
        QMimeType type = db.mimeTypeForFile(fileInfo);
        //filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/java-archive"));
        filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(type.name()));
        QString header("form-data; name=\"data\"; filename=\"%1\"");
        filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(header.arg(fileInfo.fileName())));
        _file->open(QIODevice::ReadOnly);
        _file->setParent(multiPart);
        filePart.setBodyDevice(_file);
        multiPart->append(filePart);
    }

    QString path(_sink.getNamespace().tenant().cluster().functionUrl());
    path += this->m_Settings->value(PUT_SINK_PATH_KEY).toString();
    QUrl url(path.arg(_sink.getNamespace().tenant().name(), _sink.getNamespace().name(), _sink.name()));

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Updae a Sink service url: %1", url.toString());
    }

    int code;
    QByteArray result = this->m_Client->put(url, multiPart, code);
    switch (code)
    {
    case 204:
        _code.code = HttpStatusCode::StatusCode::NoContent;
        break;
    case 400:
        _code.code = HttpStatusCode::StatusCode::BadRequest;
        _code.errorDesc = QString::fromLatin1(result);
        break;
    case 401:
        _code.code = HttpStatusCode::StatusCode::Unauthorized;
        _code.errorDesc = QString(tr("Client is not authorize to perform operation."));
        break;
    case 403:
        _code.code = HttpStatusCode::StatusCode::Forbidden;
        _code.errorDesc = QString(tr("The requester doesn't have admin permissions."));
        break;
    case 404:
        _code.code = HttpStatusCode::StatusCode::NotFound;
        _code.errorDesc = QString(tr("The Pulsar Sink doesn't exist."));
        break;
    case 503:
        _code.code = HttpStatusCode::StatusCode::ServiceUnavailable;
        _code.errorDesc = QString(tr("Function worker service is now initializing. Please try again later."));
        break;
    case 500:
        _code.code = HttpStatusCode::StatusCode::InternalServerError;
        _code.errorDesc = QString(tr("Internal Server Error."));
        break;
    }

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Updae a Sink response result: %1, code: %2", QString::fromLatin1(result), code);
    }
}

void SinkService::remove(const Sink& _sink, HttpStatusCode& _code)
{
    QString path(_sink.getNamespace().tenant().cluster().functionUrl());
    path += this->m_Settings->value(DELETE_SINK_PATH_KEY).toString();
    QUrl url(path.arg(_sink.getNamespace().tenant().name(), _sink.getNamespace().name(), _sink.name()));

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Delete a Sink service url: %1", url.toString());
    }

    int statusCode;
    QByteArray result = this->m_Client->deleteResource(url, statusCode);
    switch (statusCode)
    {
    case 204:
        _code.code = HttpStatusCode::StatusCode::NoContent;
        break;
    case 404:
        _code.code = HttpStatusCode::StatusCode::NotFound;
        _code.errorDesc = QString(tr("The Pulsar Sink doesn't exist."));
        break;
    case 400:
        _code.code = HttpStatusCode::StatusCode::BadRequest;
        _code.errorDesc = QString(tr("Invalid request."));
        break;
    case 401:
        _code.code = HttpStatusCode::StatusCode::Unauthorized;
        _code.errorDesc = QString(tr("Client is not authorize to perform operation."));
        break;
    case 408:
        _code.code = HttpStatusCode::StatusCode::RequestTimeout;
        _code.errorDesc = QString(tr("Request timeout."));
        break;
    case 503:
        _code.code = HttpStatusCode::StatusCode::ServiceUnavailable;
        _code.errorDesc = QString(tr("Function worker service is now initializing. Please try again later."));
        break;
    case 500:
        _code.code = HttpStatusCode::StatusCode::InternalServerError;
        _code.errorDesc = QString(tr("Internal Server Error."));
        break;
    }

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Delete a Sink response result: %1, code: %2", QString::fromLatin1(result), statusCode);
    }
}

void SinkService::start(const Sink& _sink, HttpStatusCode& _code)
{
    QString path(_sink.getNamespace().tenant().cluster().functionUrl());
    path += this->m_Settings->value(START_SINK_PATH_KEY).toString();
    QUrl url(path.arg(_sink.getNamespace().tenant().name(), _sink.getNamespace().name(), _sink.name()));

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Start a Sink service url: %1", url.toString());
    }

    int statusCode;
    QByteArray result = this->m_Client->post(url, QByteArray(), statusCode);
    switch (statusCode)
    {
    case 204:
        _code.code = HttpStatusCode::StatusCode::NoContent;
        break;
    case 404:
        _code.code = HttpStatusCode::StatusCode::NotFound;
        _code.errorDesc = QString(tr("The Pulsar Sink does not exist."));
        break;
    case 400:
        _code.code = HttpStatusCode::StatusCode::BadRequest;
        _code.errorDesc = QString(tr("Invalid request."));
        break;
    case 401:
        _code.code = HttpStatusCode::StatusCode::Unauthorized;
        _code.errorDesc = QString(tr("Client is not authorize to perform operation."));
        break;
    case 500:
        _code.code = HttpStatusCode::StatusCode::InternalServerError;
        _code.errorDesc = QString(tr("Internal server error."));
        break;
    case 503:
        _code.code = HttpStatusCode::StatusCode::ServiceUnavailable;
        _code.errorDesc = QString(tr("Function worker service is now initializing. Please try again later."));
        break;
    }

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Start a Sink response result: %1, code: %2", QString::fromLatin1(result), statusCode);
    }
}

void SinkService::stop(const Sink& _sink, HttpStatusCode& _code)
{
    QString path(_sink.getNamespace().tenant().cluster().functionUrl());
    path += this->m_Settings->value(STOP_SINK_PATH_KEY).toString();
    QUrl url(path.arg(_sink.getNamespace().tenant().name(), _sink.getNamespace().name(), _sink.name()));

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Stop a Sink service url: %1", url.toString());
    }

    int statusCode;
    QByteArray result = this->m_Client->post(url, QByteArray(), statusCode);
    switch (statusCode)
    {
    case 204:
        _code.code = HttpStatusCode::StatusCode::NoContent;
        break;
    case 404:
        _code.code = HttpStatusCode::StatusCode::NotFound;
        _code.errorDesc = QString(tr("The Pulsar Sink does not exist."));
        break;
    case 400:
        _code.code = HttpStatusCode::StatusCode::BadRequest;
        _code.errorDesc = QString(tr("Invalid request."));
        break;
    case 401:
        _code.code = HttpStatusCode::StatusCode::Unauthorized;
        _code.errorDesc = QString(tr("Client is not authorize to perform operation."));
        break;
    case 500:
        _code.code = HttpStatusCode::StatusCode::InternalServerError;
        _code.errorDesc = QString(tr("Internal server error."));
        break;
    case 503:
        _code.code = HttpStatusCode::StatusCode::ServiceUnavailable;
        _code.errorDesc = QString(tr("Function worker service is now initializing. Please try again later."));
        break;
    }

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Stop a Sink response result: %1, code: %2", QString::fromLatin1(result), statusCode);
    }
}

QList<FunctionInstance> SinkService::instances(const Sink& _sink) const
{
    QList<FunctionInstance> instances;
    QString path(_sink.getNamespace().tenant().cluster().functionUrl());
    path += this->m_Settings->value(GET_SINK_STATUS_PATH_KEY).toString();
    QUrl url(path.arg(_sink.getNamespace().tenant().name(), _sink.getNamespace().name(), _sink.name()));

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Get Sink status service url: %1", url.toString());
    }

    int code;
    QByteArray result = this->m_Client->get(url, code);
    if (code == HttpStatusCode::StatusCode::OK)
    {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(result, &error);
        if (error.error == QJsonParseError::ParseError::NoError)
        {
            QJsonObject root = doc.object();
            QJsonArray array = root["instances"].toArray();
            for (int i = 0, n = array.size(); i < n; ++i)
            {
                QJsonObject object = array[i].toObject();
                FunctionInstance instance;
                instance.setNamespace(_sink.getNamespace());
                instance.setName(_sink.name());
                instance.setInstanceId(object["instanceId"].toInt());
                QJsonObject status = object["status"].toObject();
                instance.setRunning(status["running"].toBool());
                instance.setReceivedNum(status["numReceivedFromSource"].toInt());
                instance.setWrittenNum(status["numWritten"].toInt());
                instance.setErrorNum(status["numSourceExceptions"].toInt());
                instances << instance;
            }
        }
    }

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Get Source Sink response result: %1", QString::fromLatin1(result));
    }

    return instances;
}

QStringList SinkService::sinkNames(const Namespace& _namespace) const
{
    QStringList names;
    QString path(_namespace.tenant().cluster().functionUrl());
    path += this->m_Settings->value(GET_SINKS_PATH_KEY).toString();
    QUrl url(path.arg(_namespace.tenant().name(), _namespace.name()));

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Get Sink names service url: %1", url.toString());
    }

    int code;
    QByteArray result = this->m_Client->get(url, code);
    if (code == HttpStatusCode::StatusCode::OK)
    {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(result, &error);
        if (error.error == QJsonParseError::ParseError::NoError)
        {
            QJsonArray roots = doc.array();
            for (int i = 0, n = roots.size(); i < n; ++i)
            {
                names << roots[i].toString();
            }
        }
    }

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Get Sink names response result: %1, status code: %2", QString::fromLatin1(result), code);
    }

    return names;
}

QByteArray SinkService::information(const Namespace& _namespace, const QString& _name) const
{
    QString path(_namespace.tenant().cluster().functionUrl());
    path += this->m_Settings->value(GET_SINK_INFO_PATH_KEY).toString();
    QUrl url(path.arg(_namespace.tenant().name(), _namespace.name(), _name));

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Get Sink information service url: %1", url.toString());
    }

    int code;
    QByteArray result = this->m_Client->get(url, code);

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Get Sink information response result: %1, status code: %2", QString::fromLatin1(result), code);
    }

    return result;
}

QByteArray SinkService::status(const Namespace& _namespace, const QString& _name) const
{
    QString path(_namespace.tenant().cluster().functionUrl());
    path += this->m_Settings->value(GET_SINK_STATUS_PATH_KEY).toString();
    QUrl url(path.arg(_namespace.tenant().name(), _namespace.name(), _name));

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Get Sink status service url: %1", url.toString());
    }

    int code;
    QByteArray result = this->m_Client->get(url, code);

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Get Sink status response result: %1, status code: %2", QString::fromLatin1(result), code);
    }

    return result;
}
