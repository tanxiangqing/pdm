#include "sourceservice.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QHttpPart>
#include <QFileInfo>
#include <QMimeDatabase>

#include <log4qt/logger.h>

#include "../constants.h"

QList<Source> SourceService::sources(const Namespace& _namespace)
{
    QList<Source> sources;
    QString path(_namespace.tenant().cluster().functionUrl());
    path += this->m_Settings->value(GET_SOURCES_PATH_KEY).toString();
    QUrl url(path.arg(_namespace.tenant().name(), _namespace.name()));

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Get Sources List service url: %1", url.toString());
    }

    int code;
    QByteArray result = this->m_Client->get(url, code);
    if (code == 200)
    {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(result, &error);
        if (error.error == QJsonParseError::ParseError::NoError)
        {
            QJsonArray elements = doc.array();
            for (int i = 0, n = elements.size(); i < n; ++i)
            {
                Source source;
                source.setNamespace(_namespace);
                source.setName(elements[i].toString());
                source = information(source);
                sources << source;
            }
        }
    }

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Get Sources List response result: %1", QString::fromLatin1(result));
    }

    return sources;
}

Source& SourceService::information(Source& _source)
{
    QString path(_source.getNamespace().tenant().cluster().functionUrl());
    path += this->m_Settings->value(GET_SOURCE_INFO_PATH_KEY).toString();
    QUrl url(path.arg(_source.getNamespace().tenant().name(), _source.getNamespace().name(), _source.name()));

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Get Source information service url: %1", url.toString());
    }

    int code;
    QByteArray result = this->m_Client->get(url, code);
    if (code == 200)
    {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(result, &error);
        if (error.error == QJsonParseError::ParseError::NoError)
        {
            QJsonObject root = doc.object();
            _source.setClassname(root["className"].toString());
            _source.setTopicName(root["topicName"].toString());
            QJsonObject config = root["configs"].toObject();
            QJsonObject::const_iterator it;
            for (it = config.constBegin(); it != config.constEnd(); ++it)
            {
                _source.addConfig(it.key(), it.value().toString());
            }
            _source = status(_source);
        }
    }

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Get Source information response result: %1", QString::fromLatin1(result));
    }

    return _source;
}

Source& SourceService::status(Source& _source)
{
    QString path(_source.getNamespace().tenant().cluster().functionUrl());
    path += this->m_Settings->value(GET_SOURCE_STATUS_PATH_KEY).toString();
    QUrl url(path.arg(_source.getNamespace().tenant().name(), _source.getNamespace().name(), _source.name()));

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Get Source status service url: %1", url.toString());
    }

    int code;
    QByteArray result = this->m_Client->get(url, code);
    if (code == 200)
    {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(result, &error);
        if (error.error == QJsonParseError::ParseError::NoError)
        {
            QJsonObject root = doc.object();
            _source.setInstanceNum(root["numInstances"].toInt());
            _source.setRunningNum(root["numRunning"].toInt());
        }
    }

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Get Source status response result: %1", QString::fromLatin1(result));
    }

    return _source;
}

void SourceService::createSource(const Source& _source, QFile* _file, HttpStatusCode& _code)
{
    QByteArray json = _source.toJson();

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Greate a new Source request body: %1", QString::fromLatin1(json));
    }

    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    //配置部分
    QHttpPart configPart;
    configPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));
    configPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"sourceConfig\""));
    configPart.setBody(json);
    //文件部分
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
    multiPart->append(configPart);

    QString path(_source.getNamespace().tenant().cluster().functionUrl());
    path += this->m_Settings->value(PUT_NEW_SOURCE_PATH_KEY).toString();
    QUrl url(path.arg(_source.getNamespace().tenant().name(), _source.getNamespace().name(), _source.name()));

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Greate a new Source service url: %1", url.toString());
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
        Log4Qt::Logger::logger("main")->debug("Greate a new Source response result: %1, code: %2", QString::fromLatin1(result), code);
    }
}

void SourceService::updateSource(const Source& _source, QFile* _file, HttpStatusCode& _code)
{
    QByteArray json = _source.toJson();

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Update Source request body: %1", QString::fromLatin1(json));
    }

    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    //配置部分
    QHttpPart configPart;
    configPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));
    configPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"sourceConfig\""));
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
        //_file->setParent(multiPart);
        filePart.setBodyDevice(_file);
        multiPart->append(filePart);
    }

    QString path(_source.getNamespace().tenant().cluster().functionUrl());
    path += this->m_Settings->value(PUT_SOURCE_PATH_KEY).toString();
    QUrl url(path.arg(_source.getNamespace().tenant().name(), _source.getNamespace().name(), _source.name()));

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Updae a Source service url: %1", url.toString());
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
        _code.errorDesc = QString(tr("The Pulsar Source doesn't exist."));
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
        Log4Qt::Logger::logger("main")->debug("Updae a Source response result: %1, code: %2", QString::fromLatin1(result), code);
    }
}

void SourceService::deleteSource(const Source& _source, HttpStatusCode& _code)
{
    QString path(_source.getNamespace().tenant().cluster().functionUrl());
    path += this->m_Settings->value(DELETE_SOURCE_PATH_KEY).toString();
    QUrl url(path.arg(_source.getNamespace().tenant().name(), _source.getNamespace().name(), _source.name()));

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Delete a Source service url: %1", url.toString());
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
        _code.errorDesc = QString(tr("The Pulsar Source doesn't exist."));
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
        Log4Qt::Logger::logger("main")->debug("Delete a Source response result: %1, code: %2", QString::fromLatin1(result), statusCode);
    }
}

void SourceService::startSource(const Source& _source, HttpStatusCode& _code)
{
    QString path(_source.getNamespace().tenant().cluster().functionUrl());
    path += this->m_Settings->value(START_SOURCE_PATH_KEY).toString();
    QUrl url(path.arg(_source.getNamespace().tenant().name(), _source.getNamespace().name(), _source.name()));

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Start a Source service url: %1", url.toString());
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
        _code.errorDesc = QString(tr("The Pulsar Source does not exist."));
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
        Log4Qt::Logger::logger("main")->debug("Start a Source response result: %1, code: %2", QString::fromLatin1(result), statusCode);
    }
}

void SourceService::stopSource(const Source& _source, HttpStatusCode& _code)
{
    QString path(_source.getNamespace().tenant().cluster().functionUrl());
    path += this->m_Settings->value(STOP_SOURCE_PATH_KEY).toString();
    QUrl url(path.arg(_source.getNamespace().tenant().name(), _source.getNamespace().name(), _source.name()));

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Stop a Source service url: %1", url.toString());
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
        _code.errorDesc = QString(tr("The Pulsar Source does not exist."));
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
        Log4Qt::Logger::logger("main")->debug("Stop a Source response result: %1, code: %2", QString::fromLatin1(result), statusCode);
    }
}

QList<SourceInstance> SourceService::instances(const Source& _source)
{
    QList<SourceInstance> instances;
    QString path(_source.getNamespace().tenant().cluster().functionUrl());
    path += this->m_Settings->value(GET_SOURCE_STATUS_PATH_KEY).toString();
    QUrl url(path.arg(_source.getNamespace().tenant().name(), _source.getNamespace().name(), _source.name()));

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Get Source status service url: %1", url.toString());
    }

    int code;
    QByteArray result = this->m_Client->get(url, code);
    if (code == 200)
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
                SourceInstance instance(_source);
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
        Log4Qt::Logger::logger("main")->debug("Get Source status response result: %1", QString::fromLatin1(result));
    }

    return instances;
}
