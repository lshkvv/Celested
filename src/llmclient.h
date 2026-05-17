#ifndef LLMCLIENT_H
#define LLMCLIENT_H

#include <QObject>
#include <QString>
#include <QVariantMap>

class QNetworkAccessManager;
class QNetworkReply;

class LlmClient : public QObject
{
    Q_OBJECT

public:
    explicit LlmClient(QObject *parent = nullptr);

    bool isConfigured() const;
    void askAboutObject(const QString &userMessage, const QVariantMap &objectContext);

signals:
    void replyReady(const QString &reply);
    void errorOccurred(const QString &error);

private slots:
    void handleReplyFinished();

private:
    QString buildSystemPrompt(const QVariantMap &objectContext) const;

    QNetworkAccessManager *m_network = nullptr;
    QNetworkReply *m_activeReply = nullptr;
};

#endif // LLMCLIENT_H
