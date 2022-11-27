#include "functionservice.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QHttpMultiPart>
#include <QMimeDatabase>
#include <QFileInfo>

#include <log4qt/logger.h>

#include "../constants.h"

QList<Function> FunctionService::functions(const Namespace& _namespace) const
{
    QList<Function> functions;
    QString path(_namespace.tenant().cluster().functionUrl());
    path += this->m_Settings->value(GET_FUNCTIONS_PATH_KEY).toString();
    QUrl url(path.arg(_namespace.tenant().name(), _namespace.name()));

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Get Functions List service url: %1", url.toString());
    }

    QByteArray result = this->m_Client->get(url);
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(result, &error);
    if (error.error == QJsonParseError::ParseError::NoError)
    {
        QJsonArray array = doc.array();
        for (int i = 0, n = array.size(); i < n; ++i)
        {
            Function function;
            function.setNamespace(_namespace);
            function.appendInformation(information(_namespace, array[i].toString()));
            function.appendStatus(status(function));
            functions << function;
        }
    }

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Get Functions List response result: %1", QString::fromLatin1(result));
    }

    return functions;
}

QByteArray FunctionService::status(const Function& _function) const
{
    QString path(_function.getNamespace().tenant().cluster().functionUrl());
    path += this->m_Settings->value(GET_FUNCTION_STATUS_PATH_KEY).toString();
    QUrl url(path.arg(_function.getNamespace().tenant().name(), _function.getNamespace().name(), _function.name()));

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Get Functions status service url: %1", url.toString());
    }

    QByteArray result = this->m_Client->get(url);

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Get Functions status response result: %1", QString::fromLatin1(result));
    }

    return result;
}

QByteArray FunctionService::information(const Namespace& _namespace, const QString& _name) const
{
    QString path(_namespace.tenant().cluster().functionUrl());
    path += this->m_Settings->value(GET_FUNCTION_INFO_PATH_KEY).toString();
    QUrl url(path.arg(_namespace.tenant().name(), _namespace.name(), _name));

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Get Function information service url: %1", url.toString());
    }

    QByteArray result = this->m_Client->get(url);

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Get Function information response result: %1", QString::fromLatin1(result));
    }

    return  result;
}

void FunctionService::createFunction(const Function& _function, QFile* _file, HttpStatusCode& _code)
{
    QByteArray json = _function.toJson();

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Greate a new Function request body: %1", QString::fromLatin1(json));
    }

    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    //配置部分
    QHttpPart configPart;
    configPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));
    configPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                         QVariant("form-data; name=\"functionConfig\""));
    configPart.setBody(json);
    //文件部分
    QHttpPart filePart;
    QFileInfo fileInfo(_file->fileName());
    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(fileInfo);
    //filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/java-archive"));
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(type.name()));
    QString header("form-data; name=\"data\"; filename=\"%1\"");
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant(header.arg(fileInfo.fileName())));
    _file->open(QIODevice::ReadOnly);
    _file->setParent(multiPart);
    filePart.setBodyDevice(_file);
    multiPart->append(filePart);
    multiPart->append(configPart);

    QString path(_function.getNamespace().tenant().cluster().functionUrl());
    path += this->m_Settings->value(PUT_NEW_FUNCTION_PATH_KEY).toString();
    QUrl url(path.arg(_function.getNamespace().tenant().name(), _function.getNamespace().name(), _function.name()));

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Greate a new Function service url: %1", url.toString());
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
    case 403:
        _code.code = HttpStatusCode::StatusCode::Forbidden;
        _code.errorDesc = QString(tr("The requester doesn't have admin permissions."));
        break;
    case 408:
        _code.code = HttpStatusCode::StatusCode::RequestTimeout;
        _code.errorDesc = QString(tr("Request timeout."));
        break;
    }

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Greate a new Function response result: %1, code: %2", QString::fromLatin1(result), code);
    }
}

void FunctionService::updateFunction(const Function& _function, QFile* _file, HttpStatusCode& _code)
{
    QByteArray json = _function.toJson();

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Update Function request body: %1", QString::fromLatin1(json));
    }

    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    //配置部分
    QHttpPart configPart;
    configPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));
    configPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"functionConfig\""));
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

    QString path(_function.getNamespace().tenant().cluster().functionUrl());
    path += this->m_Settings->value(UPDATE_FUNCTION_PATH_KEY).toString();
    QUrl url(path.arg(_function.getNamespace().tenant().name(), _function.getNamespace().name(), _function.name()));

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
        _code.errorDesc = QString(tr("The Pulsar Function doesn't exist."));
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
        Log4Qt::Logger::logger("main")->debug("Updae a Function response result: %1, code: %2", QString::fromLatin1(result), code);
    }
}

void FunctionService::deleteFunction(const Function& _function, HttpStatusCode& _code)
{
    QString path(_function.getNamespace().tenant().cluster().functionUrl());
    path += this->m_Settings->value(DELETE_FUNCTION_PATH_KEY).toString();
    QUrl url(path.arg(_function.getNamespace().tenant().name(), _function.getNamespace().name(), _function.name()));

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Delete a Function service url: %1", url.toString());
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
        _code.errorDesc = QString(tr("The Pulsar Function doesn't exist."));
        break;
    case 400:
        _code.code = HttpStatusCode::StatusCode::BadRequest;
        _code.errorDesc = QString(tr("Invalid request."));
        break;
    case 403:
        _code.code = HttpStatusCode::StatusCode::Forbidden;
        _code.errorDesc = QString(tr("The requester doesn't have admin permissions."));
        break;
    case 408:
        _code.code = HttpStatusCode::StatusCode::RequestTimeout;
        _code.errorDesc = QString(tr("Request timeout."));
        break;
    }

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Delete a Function response result: %1, code: %2", QString::fromLatin1(result), statusCode);
    }
}

void FunctionService::startFunction(const Function& _function, HttpStatusCode& _code)
{
    QString path(_function.getNamespace().tenant().cluster().functionUrl());
    path += this->m_Settings->value(START_FUNCTION_PATH_KEY).toString();
    QUrl url(path.arg(_function.getNamespace().tenant().name(), _function.getNamespace().name(), _function.name()));

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Start a Function service url: %1", url.toString());
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
        _code.errorDesc = QString(tr("The Pulsar Function does not exist."));
        break;
    case 400:
        _code.code = HttpStatusCode::StatusCode::BadRequest;
        _code.errorDesc = QString(tr("Invalid request."));
        break;
    case 500:
        _code.code = HttpStatusCode::StatusCode::InternalServerError;
        _code.errorDesc = QString(tr("Internal server error."));
        break;
    }

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Start a Function response result: %1, code: %2", QString::fromLatin1(result), statusCode);
    }
}

void FunctionService::stopFunction(const Function& _function, HttpStatusCode& _code)
{
    QString path(_function.getNamespace().tenant().cluster().functionUrl());
    path += this->m_Settings->value(STOP_FUNCTION_PATH_KEY).toString();
    QUrl url(path.arg(_function.getNamespace().tenant().name(), _function.getNamespace().name(), _function.name()));

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Stop a Function service url: %1", url.toString());
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
        _code.errorDesc = QString(tr("The Pulsar Function does not exist."));
        break;
    case 400:
        _code.code = HttpStatusCode::StatusCode::BadRequest;
        _code.errorDesc = QString(tr("Invalid request."));
        break;
    case 500:
        _code.code = HttpStatusCode::StatusCode::InternalServerError;
        _code.errorDesc = QString(tr("Internal server error."));
        break;
    }

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Stop a Function response result: %1, code: %2", QString::fromLatin1(result), statusCode);
    }
}

QList<FunctionInstance> FunctionService::instances(const Function& _function) const
{
    QList<FunctionInstance> instances;
    QString path(_function.getNamespace().tenant().cluster().functionUrl());
    path += this->m_Settings->value(GET_FUNCTION_STATUS_PATH_KEY).toString();
    QUrl url(path.arg(_function.getNamespace().tenant().name(), _function.getNamespace().name(), _function.name()));

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Get Function status service url: %1", url.toString());
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
                instance.setNamespace(_function.getNamespace());
                instance.setName(_function.name());
                instance.setInstanceId(object["instanceId"].toInt());
                QJsonObject status = object["status"].toObject();
                instance.setRunning(status["running"].toBool());
                instance.setReceivedNum(status["numReceived"].toInt());
                instance.setWrittenNum(status["numSuccessfullyProcessed"].toInt());
                instance.setErrorNum(status["numUserExceptions"].toInt());
                instance.setWorkId(status["workerId"].toString());
                instances << instance;
            }
        }
    }

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Get Source Function response result: %1", QString::fromLatin1(result));
    }

    return instances;
}
