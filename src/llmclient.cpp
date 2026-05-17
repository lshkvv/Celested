#include "llmclient.h"

#include "applogger.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcessEnvironment>

namespace {

QString envValue(const char *name, const QString &fallback = QString())
{
    const QByteArray value = qgetenv(name);
    return value.isEmpty() ? fallback : QString::fromUtf8(value);
}

} // namespace

LlmClient::LlmClient(QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
{
}

bool LlmClient::isConfigured() const
{
    return !envValue("CELESTED_LLM_API_KEY").isEmpty();
}

QString LlmClient::buildSystemPrompt(const QVariantMap &objectContext) const
{
    QString prompt = QStringLiteral(
        "You are Celested, a friendly astronomy guide for amateur astrophotographers. "
        "Answer clearly in the user's language. Use the object data below; if something is unknown, say so.\n\n"
        "Object data:\n");

    for (auto it = objectContext.constBegin(); it != objectContext.constEnd(); ++it) {
        if (it.value().toString().trimmed().isEmpty())
            continue;
        prompt += QStringLiteral("- %1: %2\n").arg(it.key(), it.value().toString());
    }

    return prompt;
}

void LlmClient::askAboutObject(const QString &userMessage, const QVariantMap &objectContext)
{
    if (m_activeReply) {
        m_activeReply->abort();
        m_activeReply = nullptr;
    }

    const QString apiKey = envValue("CELESTED_LLM_API_KEY");
    if (apiKey.isEmpty()) {
        emit errorOccurred(
            QStringLiteral("LLM is not configured. Set the CELESTED_LLM_API_KEY environment variable."));
        return;
    }

    const QString apiUrl = envValue("CELESTED_LLM_API_URL", QStringLiteral("https://api.openai.com/v1/chat/completions"));
    const QString model = envValue("CELESTED_LLM_MODEL", QStringLiteral("gpt-4o-mini"));

    QJsonObject body;
    body.insert(QStringLiteral("model"), model);
    body.insert(QStringLiteral("temperature"), 0.6);

    QJsonArray messages;
    messages.append(QJsonObject{
        {QStringLiteral("role"), QStringLiteral("system")},
        {QStringLiteral("content"), buildSystemPrompt(objectContext)},
    });
    messages.append(QJsonObject{
        {QStringLiteral("role"), QStringLiteral("user")},
        {QStringLiteral("content"), userMessage},
    });
    body.insert(QStringLiteral("messages"), messages);

    QNetworkRequest request{QUrl(apiUrl)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Authorization", QByteArray("Bearer ") + apiKey.toUtf8());

    m_activeReply = m_network->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(m_activeReply, &QNetworkReply::finished, this, &LlmClient::handleReplyFinished);

    LOG_INFO(QStringLiteral("LLM"), QStringLiteral("Sent chat request (%1)").arg(model));
}

void LlmClient::handleReplyFinished()
{
    if (!m_activeReply)
        return;

    QNetworkReply *reply = m_activeReply;
    m_activeReply = nullptr;

    if (reply->error() != QNetworkReply::NoError) {
        const QString message = QStringLiteral("LLM request failed: %1").arg(reply->errorString());
        LOG_ERROR(QStringLiteral("LLM"), message);
        emit errorOccurred(message);
        reply->deleteLater();
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    reply->deleteLater();

    const QJsonArray choices = doc.object().value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty()) {
        emit errorOccurred(QStringLiteral("LLM returned an empty response."));
        return;
    }

    const QString content = choices.first().toObject()
                                .value(QStringLiteral("message"))
                                .toObject()
                                .value(QStringLiteral("content"))
                                .toString()
                                .trimmed();

    if (content.isEmpty()) {
        emit errorOccurred(QStringLiteral("LLM returned no text."));
        return;
    }

    emit replyReady(content);
}
