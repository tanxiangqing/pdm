#include "topicservice.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QTextCodec>
#include <QDebug>

#include <log4qt/logger.h>

#include "../constants.h"

QList<Topic> TopicService::topics(const Namespace& _namespace) const
{
    QList<Topic> topics;
    topics << this->nonePartitionedTopics(_namespace) << this->partitionedTopics(_namespace);
    return topics;
}

/**
 * @brief Create a partitioned topic
 * @param _topic
 * @param _code
 */
void TopicService::createTopic(const Topic& _topic, HttpStatusCode& _code)
{
    QString path(_topic.getNamespace().tenant().cluster().adminUrl());
    if (_topic.stats().partitions() > 0)
    {
        path = path.append(this->m_Settings->value(PUT_NEW_PARTITIONED_TOPIC_PATH_KEY).toString());
    }
    else
    {
        path = path.append(this->m_Settings->value(PUT_NEW_NONPARTITIONED_TOPIC_PATH_KEY).toString());
    }

    QUrl url(path.arg(_topic.getNamespace().tenant().name(), _topic.getNamespace().name(), _topic.name(), _topic.domain()));

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Create a new Topic service url: %1", url.toString());
    }

    QVariant root;
    root.setValue(_topic.stats().partitions());
    QByteArray body = root.toByteArray();

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Create a new Topic request body: %1", QString::fromLatin1(body));
    }

    int statusCode;
    QByteArray result = this->m_Client->put(url, body, statusCode);
    switch (statusCode)
    {
    case 204:
        _code.code = HttpStatusCode::StatusCode::NoContent;
        break;
    case 409:
        _code.code = HttpStatusCode::StatusCode::Conflict;
        _code.errorDesc = QString(tr("Partitioned topic already exist."));
        break;
    case 412:
        _code.code = HttpStatusCode::StatusCode::PreconditionFailed;
        _code.errorDesc = QString(tr("Failed Reason : Name is invalid or Namespace does not have any clusters configured."));
        break;
    case 403:
        _code.code = HttpStatusCode::StatusCode::Forbidden;
        _code.errorDesc = QString(tr("Don't have admin permission."));
        break;
    case 401:
        _code.code = HttpStatusCode::StatusCode::Unauthorized;
        _code.errorDesc = QString(tr("Don't have permission to administrate resources on this tenant."));
        break;
    case 500:
        _code.code = HttpStatusCode::StatusCode::InternalServerError;
        _code.errorDesc = QString(tr("Internal server error."));
        break;
    case 503:
        _code.code = HttpStatusCode::StatusCode::ServiceUnavailable;
        _code.errorDesc = QString(tr("Failed to validate global cluster configuration."));
        break;
    }

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Create a new Topic response result: %1", QString::fromLatin1(result));
    }
}

/**
 * @brief Delete a topic
 * @param _topic
 * @param _code
 */
void TopicService::deleteTopic(const Topic& _topic, HttpStatusCode& _code)
{
    QString path(_topic.getNamespace().tenant().cluster().adminUrl());
    if (_topic.stats().partitions() > 0)
    {
        path = path.append(this->m_Settings->value(DELETE_PARTITIONED_TOPIC_PATH_KEY).toString());
    }
    else
    {
        path = path.append(this->m_Settings->value(DELETE_TOPIC_PATH_KEY).toString());
    }
    QUrl url(path.arg(_topic.getNamespace().tenant().name(), _topic.getNamespace().name(), _topic.name(), _topic.domain()));

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Delete a Topic service url: %1", url.toString());
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
        _code.errorDesc = QString(tr("Topic does not exist."));
        break;
    case 412:
        _code.code = HttpStatusCode::StatusCode::PreconditionFailed;
        _code.errorDesc = QString(tr("Topic has active producers/subscriptions."));
        break;
    case 403:
        _code.code = HttpStatusCode::StatusCode::Forbidden;
        _code.errorDesc = QString(tr("Don't have admin permission."));
        break;
    case 401:
        _code.code = HttpStatusCode::StatusCode::Unauthorized;
        _code.errorDesc = QString(tr("Don't have permission to administrate resources on this tenant."));
        break;
    case 500:
        _code.code = HttpStatusCode::StatusCode::InternalServerError;
        _code.errorDesc = QString(tr("Internal server error."));
        break;
    }

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Delete a Topic response result: %1", QString::fromLatin1(result));
    }
}

/**
 * @brief Return the last commit message id of topic
 * @param _topic
 * @param _partition
 * @param _message
 */
void TopicService::getLastMessageId(const Topic& _topic, const int& _partition, Message& _message)
{
    QString topicName = _partition >= 0 ? QString("%1-partition-%2").arg(_topic.name()).arg(_partition) : _topic.name();
    QString path(_topic.getNamespace().tenant().cluster().adminUrl());
    path = path.append(this->m_Settings->value(GET_LAST_MESSAGE_ID_PATH_KEY).toString());
    QUrl url(path.arg(_topic.getNamespace().tenant().name(), _topic.getNamespace().name(), topicName));

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Get Last Message ID Service url: %1", url.toString());
    }

    QByteArray result = this->m_Client->get(url);
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(result, &error);
    if (error.error == QJsonParseError::ParseError::NoError)
    {
        QJsonObject root = doc.object();
        _message.setLedgerId(root["ledgerId"].toInt());
        _message.setEntryId(root["entryId"].toInt());
    }

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Get Last Message ID response result: %1", QString::fromLatin1(result));
    }
}

/**
 * @brief Get message by its messageId
 * @param _topic
 * @param _partition
 * @param _ledgerId
 * @param _entryId
 * @param _num
 * @return
 */
QList<PulsarMessage> TopicService::messages(const Topic& _topic, const int& _partition, const int& _ledgerId, const int& _entryId, const int& _num) const
{
    QList<PulsarMessage> msgs;
    if (_ledgerId >= 0)
    {
        QString path(_topic.getNamespace().tenant().cluster().adminUrl());
        path = path.append(this->m_Settings->value(GET_MESSAGE_PATH_KEY).toString());
        QString topicName = _partition >= 0 ? QString("%1-partition-%2").arg(_topic.name()).arg(_partition) : _topic.name();
        int i = _entryId < _num ? _entryId : (_num - 1);
        while (i >= 0)
        {
            QUrl url(path.arg(_topic.getNamespace().tenant().name(), _topic.getNamespace().name(), topicName).arg(_ledgerId).arg(_entryId - i));

            if (Log4Qt::Logger::logger("main")->isDebugEnabled())
            {
                Log4Qt::Logger::logger("main")->debug("Get Last Message Service url: %1", url.toString());
            }

            QByteArray result = this->m_Client->get(url);
            PulsarMessage m(result);
            m.setLedgerId(_ledgerId);
            m.setEntryId(i);
            //QTextCodec* codec = QTextCodec::codecForName("ISO 8859-1");
            //QString str = codec->toUnicode(result);
            msgs << m;
            i--;

            if (Log4Qt::Logger::logger("main")->isDebugEnabled())
            {
                QTextCodec* codec = QTextCodec::codecForName("ISO 8859-1");
                QString str = codec->toUnicode(result);
                Log4Qt::Logger::logger("main")->debug("Get Last Message response result: %1", QString::fromUtf8(result.toHex(' ')));
                Log4Qt::Logger::logger("main")->debug("Get Last Message response result: %1", str);
            }
        }
    }
    return msgs;
}

TopicStorage& TopicService::topicStorage(const Topic& _topic, const int& _partition, TopicStorage& _storage)
{
    QString topicName = _partition >= 0 ? QString("%1-partition-%2").arg(_topic.name()).arg(_partition) : _topic.name();
    QString path(_topic.getNamespace().tenant().cluster().adminUrl());
    path = path.append(this->m_Settings->value(GET_STORED_TOPIC_METADATA_KEY).toString());
    QUrl url(path.arg(_topic.getNamespace().tenant().name(), _topic.getNamespace().name(), topicName, _topic.domain()));

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Get stored topic metadata Service url: %1", url.toString());
    }
    int code;
    QByteArray result = this->m_Client->get(url, code);
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(result, &error);
    if (error.error == QJsonParseError::ParseError::NoError)
    {
        QJsonObject root = doc.object();
        _storage.setStorageSize(root["totalSize"].toInt());
        _storage.setEntryNum(root["numberOfEntries"].toInt());
        QJsonArray ledgers = root["ledgers"].toArray();
        _storage.setSegmentNum(ledgers.size());
        TopicSegments ts;
        for (int i = 0, n = ledgers.size(); i < n; ++i)
        {
            QJsonObject ledger = ledgers[i].toObject();
            TopicSegment segment;
            segment.setLedgerId(ledger["ledgerId"].toInt());
            segment.setEntries(i < (n - 1) ? ledger["entries"].toInt(0) : root["currentLedgerEntries"].toInt(0));
            segment.setSize(i < (n - 1) ? ledger["size"].toInt(0) : root["currentLedgerSize"].toInt(0));
            segment.setOffload(ledger["offloaded"].toBool());
            segment.setStatus(i < (n - 1) ? TopicSegment::Status::CLOSE : TopicSegment::Status::OPEN);
            ts.append(segment);
        }
        _storage.setSegments(ts);
    }

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Get stored topic metadata response result: %1", QString::fromLatin1(result));
    }

    return _storage;
}

TopicStats TopicService::overview(const Topic& _topic, const int& _partition) const
{
    TopicStats stats;
    QStringList topicNames;
    if (_partition >= 0)
    {
        QString topicName("%1-partition-%2");
        topicNames << topicName.arg(_topic.name()).arg(_partition);
    }
    else
    {
        topicNames << _topic.name();
    }

    QString path(_topic.getNamespace().tenant().cluster().adminUrl());
    path = path.append(this->m_Settings->value(GET_TOPIC_STATS_KEY).toString());
    foreach (const QString& name, topicNames)
    {
        QUrl url(path.arg(_topic.getNamespace().tenant().name(), _topic.getNamespace().name(), name, _topic.domain()));

        if (Log4Qt::Logger::logger("main")->isDebugEnabled())
        {
            Log4Qt::Logger::logger("main")->debug("Get Topic stats Service url: %1", url.toString());
        }

        QByteArray result = this->m_Client->get(url);
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(result, &error);
        if (error.error == QJsonParseError::ParseError::NoError)
        {
            QJsonObject root = doc.object();
            QJsonArray publishers = root["publishers"].toArray();
            QJsonObject subscriptions = root["subscriptions"].toObject();

            for (int i = 0, n = publishers.size(); i < n; ++i)
            {
                QJsonObject obj = publishers[i].toObject();
                QJsonDocument _doc(obj);
                Producer producer;
                producer = Producer::fromJson(_doc.toJson(), producer);
                stats.addProducer(producer);
            }

            QJsonObject::const_iterator it;
            for (it = subscriptions.constBegin(); it != subscriptions.constEnd(); ++it)
            {
                Subscription subscription;
                subscription.setName(it.key());
                QJsonDocument _doc(it->toObject());
                subscription = Subscription::fromJson(_doc.toJson(), subscription);
                stats.addSubscription(subscription);


            }
        }

        if (Log4Qt::Logger::logger("main")->isDebugEnabled())
        {
            Log4Qt::Logger::logger("main")->debug("Get Topic stats response result: %1", QString::fromLatin1(result));
        }
    }
    return stats;
}

/**
 * @brief Peek nth message on a topic subscription
 * @param _topic
 * @param _partition
 * @param _subName
 * @param _num
 * @return
 */
QList<PulsarMessage> TopicService::messages(const Topic& _topic, const int& _partition, const QString& _subName, const int& _num) const
{
    QList<PulsarMessage> messages;
    QString path(_topic.getNamespace().tenant().cluster().adminUrl());
    path = path.append(this->m_Settings->value(PEEK_SUBSCRIPTION_MSG_PATH_KEY).toString());
    QString topicName = _partition >= 0 ? QString("%1-partition-%2").arg(_topic.name()).arg(_partition) : _topic.name();

    int i = _num;
    while (i > 0)
    {
        QUrl url(path.arg(_topic.getNamespace().tenant().name(), _topic.getNamespace().name(), topicName, _subName).arg(i).arg(_topic.domain()));

        if (Log4Qt::Logger::logger("main")->isDebugEnabled())
        {
            Log4Qt::Logger::logger("main")->debug("Peek nth message on a topic subscription Service url: %1", url.toString());
        }

        QByteArray result = this->m_Client->get(url);
        PulsarMessage message(result);
        message.setEntryId(i);
        messages << message;

        if (Log4Qt::Logger::logger("main")->isDebugEnabled())
        {
            Log4Qt::Logger::logger("main")->debug("Peek nth message on a topic subscription response result: %1", QString::fromUtf8(result.toHex(' ')));
            Log4Qt::Logger::logger("main")->debug("Message key: %1, body: %2", message.key(), message.toString());
        }

        i--;
    }
    return messages;
}

/**
 * @brief Create a subscription on the topic
 * @param _topic
 * @param _subName
 * @param _code
 */
void TopicService::createSubscription(const Topic& _topic, const QString& _subName, HttpStatusCode& _code)
{
    QString path(_topic.getNamespace().tenant().cluster().adminUrl());
    path += this->m_Settings->value(PUT_SUBSCRIPTION_PATH_KEY).toString();
    QUrl url(path.arg(_topic.getNamespace().tenant().name(), _topic.getNamespace().name(), _topic.name(), _subName, _topic.domain()));

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Create a subscription on the topic service url: %1", url.toString());
    }

    int statusCode;
    QByteArray result = this->m_Client->put(url, QByteArray(), statusCode);
    switch (statusCode)
    {
    case 204:
        _code.code = HttpStatusCode::StatusCode::NoContent;
        break;
    case 409:
        _code.code = HttpStatusCode::StatusCode::Conflict;
        _code.errorDesc = QString::fromLatin1(result);
        break;
    case 400:
        _code.code = HttpStatusCode::StatusCode::BadRequest;
        _code.errorDesc = QString(tr("Create subscription on non persistent topic is not supported."));
        break;
    case 404:
        _code.code = HttpStatusCode::StatusCode::NotFound;
        _code.errorDesc = QString(tr("Topic/Subscription does not exist."));
        break;
    case 405:
        _code.code = HttpStatusCode::StatusCode::MethodNotAllowed;
        _code.errorDesc = QString(tr("Not supported for partitioned topics."));
        break;
    case 403:
        _code.code = HttpStatusCode::StatusCode::Forbidden;
        _code.errorDesc = QString(tr("Don't have admin permission."));
        break;
    case 401:
        _code.code = HttpStatusCode::StatusCode::Unauthorized;
        _code.errorDesc = QString(tr("Don't have permission to administrate resources on this tenant orsubscriber is not authorized to access this operation."));
        break;
    case 500:
        _code.code = HttpStatusCode::StatusCode::InternalServerError;
        _code.errorDesc = QString(tr("Internal server error."));
        break;
    case 503:
        _code.code = HttpStatusCode::StatusCode::ServiceUnavailable;
        _code.errorDesc = QString(tr("Failed to validate global cluster configuration."));
        break;
    }

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Create a subscription on the topic response result: %1", QString::fromLatin1(result));
    }
}

void TopicService::deleteSubscription(const Topic& _topic, const QString& _subName, HttpStatusCode& _code)
{
    QString path(_topic.getNamespace().tenant().cluster().adminUrl());
    path += this->m_Settings->value(DELETE_SUBSCRIPTION_PATH_KEY).toString();
    QUrl url(path.arg(_topic.getNamespace().tenant().name(), _topic.getNamespace().name(), _topic.name(), _subName, _topic.domain()));

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Delete a subscription service url: %1", url.toString());
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
        _code.errorDesc = QString(tr("Topic does not exist."));
        break;
    case 412:
        _code.code = HttpStatusCode::StatusCode::PreconditionFailed;
        _code.errorDesc = QString(tr("Subscription has active consumers."));
        break;
    case 403:
        _code.code = HttpStatusCode::StatusCode::Forbidden;
        _code.errorDesc = QString(tr("Don't have admin permission."));
        break;
    case 401:
        _code.code = HttpStatusCode::StatusCode::Unauthorized;
        _code.errorDesc = QString(tr("Don't have permission to administrate resources on this tenant."));
        break;
    case 500:
        _code.code = HttpStatusCode::StatusCode::InternalServerError;
        _code.errorDesc = QString(tr("Internal server error."));
        break;
    case 503:
        _code.code = HttpStatusCode::StatusCode::ServiceUnavailable;
        _code.errorDesc = QString(tr("Failed to validate global cluster configuration."));
        break;
    }

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Delete a subscription response result: %1", QString::fromLatin1(result));
    }
}

/**
 * @brief Get the list of partitioned topics under a namespace.
 * @param _namespace
 * @return
 */
QList<Topic> TopicService::partitionedTopics(const Namespace& _namespace) const
{
    QString path(_namespace.tenant().cluster().adminUrl());
    path = path.append(this->m_Settings->value(GET_PARTITIONED_TOPICS_PATH_KEY).toString());
    QStringList domains;
    domains << "persistent" << "non-persistent";
    QList<Topic> topics;
    QStringList::const_iterator it;
    for (it = domains.constBegin(); it != domains.constEnd(); ++it)
    {
        QUrl url(path.arg(_namespace.tenant().name(), _namespace.name(), *it));

        if (Log4Qt::Logger::logger("main")->isDebugEnabled())
        {
            Log4Qt::Logger::logger("main")->debug("List partitioned Topics service url: %1", url.toString());
        }

        QByteArray result = this->m_Client->get(url);
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(result, &error);
        if (error.error == QJsonParseError::ParseError::NoError)
        {
            QJsonArray roots = doc.array();
            for (int i = 0, n = roots.size(); i < n; ++i)
            {
                QString name = this->topicName(roots[i].toString());
                if (!name.isEmpty())
                {
                    Topic topic(name, _namespace);
                    topic.setDomain(this->domain(roots[i].toString()));
                    topic.setPartitions(this->partitions(topic));
                    topic.setPartitioned(Topic::TopicPartitioned::Partitioned);
                    topic.setStats(this->stats(topic));
                    topics << topic;
                }
            }
        }

        if (Log4Qt::Logger::logger("main")->isDebugEnabled())
        {
            Log4Qt::Logger::logger("main")->debug("List partitioned Topics response result: %1", QString::fromLatin1(result));
        }
    }
    return topics;
}

/**
 * @brief Get the list of none partitioned topics under a namespace.
 * @param _tenant
 * @param _namespace
 * @return
 */
QList<Topic> TopicService::nonePartitionedTopics(const Namespace& _namespace) const
{
    QString path(_namespace.tenant().cluster().adminUrl());
    path = path.append(this->m_Settings->value(GET_TOPICS_PATH_KEY).toString());
    QStringList domains;
    domains << "persistent" << "non-persistent";
    QList<Topic> topics;
    QStringList::const_iterator it;
    for (it = domains.constBegin(); it != domains.constEnd(); ++it)
    {
        QUrl url(path.arg(_namespace.tenant().name(), _namespace.name(), *it));

        if (Log4Qt::Logger::logger("main")->isDebugEnabled())
        {
            Log4Qt::Logger::logger("main")->debug("List none partitioned Topics service url: %1", url.toString());
        }

        QByteArray result = this->m_Client->get(url);
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(result, &error);
        if (error.error == QJsonParseError::ParseError::NoError)
        {
            QJsonArray roots = doc.array();
            for (int i = 0, n = roots.size(); i < n; ++i)
            {
                QString name = this->topicName(roots[i].toString());
                if (!name.isEmpty())
                {
                    Topic topic(name, _namespace);
                    topic.setDomain(this->domain(roots[i].toString()));
                    topic.setPartitions(0);
                    topic.setPartitioned(Topic::TopicPartitioned::NonPartitioned);
                    topic.setStats(this->stats(topic));
                    topics << topic;
                }
            }
        }

        if (Log4Qt::Logger::logger("main")->isDebugEnabled())
        {
            Log4Qt::Logger::logger("main")->debug("List non partitioned Topics response result: %1", QString::fromLatin1(result));
        }
    }
    return topics;
}

TopicStats TopicService::stats(const Topic& _topic) const
{
    QStringList topicNames;
    if (_topic.partitions() > 0)
    {
        QString topicName("%1-partition-%2");
        topicNames << topicName.arg(_topic.name()).arg(0);
    }
    else
    {
        topicNames << _topic.name();
    }

    QString path(_topic.getNamespace().tenant().cluster().adminUrl());
    path = path.append(this->m_Settings->value(GET_TOPIC_STATS_KEY).toString());
    int publisherNum = 0, subscriptionNum = 0;
    foreach (const QString& name, topicNames)
    {
        QUrl url(path.arg(_topic.getNamespace().tenant().name(), _topic.getNamespace().name(), name, _topic.domain()));

        if (Log4Qt::Logger::logger("main")->isDebugEnabled())
        {
            Log4Qt::Logger::logger("main")->debug("Get Topic stats Service url: %1", url.toString());
        }

        QByteArray result = this->m_Client->get(url);
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(result, &error);
        if (error.error == QJsonParseError::ParseError::NoError)
        {
            QJsonObject root = doc.object();
            QJsonArray publishers = root["publishers"].toArray();
            publisherNum += publishers.size();
            QVariantMap subscriptions = root["subscriptions"].toVariant().toMap();
            subscriptionNum += subscriptions.size();
        }

        if (Log4Qt::Logger::logger("main")->isDebugEnabled())
        {
            Log4Qt::Logger::logger("main")->debug("Get Topic stats response result: %1", QString::fromLatin1(result));
        }
    }

    TopicStats stats;
    stats.setPartitions(_topic.partitions());
    stats.setProducerNum(publisherNum);
    stats.setSubscriptionNum(subscriptionNum);

    return stats;
}

QString TopicService::topicName(const QString& _fullname) const
{
    QUrl url(_fullname);
    if (url.fileName().indexOf("-partition-") < 0)
    {
        return url.fileName();
    }
    return QString();
}

QString TopicService::domain(const QString& _fullname) const
{
    QUrl url(_fullname);
    return url.scheme();
}

/**
 * @brief Get partitioned topic metadata
 * @param _topic
 * @return
 */
int TopicService::partitions(const Topic& _topic) const
{
    QString path(_topic.getNamespace().tenant().cluster().adminUrl());
    path = path.append(this->m_Settings->value(GET_PARTITIONS_TOPIC_PATH_KEY).toString());
    QUrl url(path.arg(_topic.getNamespace().tenant().name(), _topic.getNamespace().name(), _topic.name(), _topic.domain()));

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Get Topic partitions Service url: %1", url.toString());
    }

    QByteArray result = this->m_Client->get(url);
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(result, &error);
    if (error.error == QJsonParseError::ParseError::NoError)
    {
        QJsonObject root = doc.object();
        int partitions = root["partitions"].toInt();
        return partitions;
    }

    if (Log4Qt::Logger::logger("main")->isDebugEnabled())
    {
        Log4Qt::Logger::logger("main")->debug("Get Topic partitions response result: %1", QString::fromLatin1(result));
    }

    return 0;
}
