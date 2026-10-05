// Standalone support tool. Never linked into the product; no TLS verification bypass.
#include "AppDatabase.h"
#include "ValidationBuildInfo.h"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSaveFile>
#include <QSqlQuery>
#include <QSslCertificate>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QTemporaryDir>
#include <QTimer>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QCommandLineParser cli;
    cli.setApplicationDescription("Deployment support: SQLite/schema and verified HTTPS handshake.");
    cli.addHelpOption();
    cli.addOptions({{{"o", "output"}, "New JSON output file (outside app directory).", "file"},
                    {"url", "HTTPS endpoint; use a controlled loopback server for offline certification.", "url"},
                    {"ca-file", "Additional CA for a loopback-only test; never changes system trust.", "file"}});
    cli.process(app);
    const QUrl url(cli.value("url"));
    const QString output = cli.value("output");
    if (url.scheme() != "https" || url.host().isEmpty() || !url.userInfo().isEmpty()
        || output.isEmpty() || QFile::exists(output)) return 2;
    const auto outputParent = QFileInfo(output).absoluteDir().canonicalPath();
    if (outputParent.isEmpty()) return 2;
    const auto relativeParent = QDir(QCoreApplication::applicationDirPath()).relativeFilePath(outputParent);
    if (relativeParent != ".." && !relativeParent.startsWith("../") && !QDir::isAbsolutePath(relativeParent))
        return 2; // A verification run must not contaminate its deployed app subtree.
    const bool loopback = url.host() == "localhost" || url.host() == "127.0.0.1" || url.host() == "::1";
    if (cli.isSet("ca-file") && !loopback) return 2;
    QJsonObject result{{"applicationVersion", VALIDATION_APP_VERSION},
        {"sourceGitCommit", VALIDATION_SOURCE_COMMIT}, {"sourceWorkingTreeDirty", bool(VALIDATION_SOURCE_DIRTY)},
        {"sourceInputsSHA256", VALIDATION_SOURCE_SHA256}, {"qtVersion", qVersion()},
        {"timestampUtc", QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
        {"endpointKind", loopback ? "controlled-loopback" : "public-HTTPS"},
        {"peerVerification", "VerifyPeer; no ignored TLS errors"}};
    QTemporaryDir temporary;
    bool sqliteOk = false;
    {
        AppDatabase database;
        QString error;
        if (temporary.isValid() && database.open(temporary.filePath("probe.sqlite"), error)) {
            QSqlQuery query(database.connection());
            sqliteOk = query.exec("SELECT value FROM app_meta WHERE key='schema_version'")
                && query.next() && query.value(0).toInt() == AppDatabase::SchemaVersion;
            result["schema"] = query.value(0).toInt();
        }
    }
    result["qsqliteAvailable"] = QSqlDatabase::isDriverAvailable("QSQLITE");
    result["qsqliteInitializationPass"] = sqliteOk;
    const bool ssl = QSslSocket::supportsSsl();
    const auto backend = QSslSocket::activeBackend();
    const auto protocols = QSslSocket::supportedProtocols(backend);
    QJsonArray protocolNames;
    for (const auto p : protocols) {
        // Record the actual enum too, so unknown future protocols cannot be mislabeled.
        protocolNames.append(QJsonObject{{"enum", int(p)},
            {"name", p == QSsl::TlsV1_2 ? "TLSv1.2" : p == QSsl::TlsV1_3 ? "TLSv1.3" : "other"}});
    }
    result["supportsSsl"] = ssl;
    result["availableBackends"] = QJsonArray::fromStringList(QSslSocket::availableBackends());
    result["activeBackend"] = backend;
    result["supportedProtocols"] = protocolNames;
    QNetworkAccessManager network;
    const bool https = network.supportedSchemes().contains("https");
    result["httpsSchemeSupported"] = https;
    QNetworkRequest request(url);
    request.setTransferTimeout(15000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    request.setAttribute(QNetworkRequest::CookieLoadControlAttribute, QNetworkRequest::Manual);
    request.setAttribute(QNetworkRequest::CookieSaveControlAttribute, QNetworkRequest::Manual);
    auto config = QSslConfiguration::defaultConfiguration();
    config.setPeerVerifyMode(QSslSocket::VerifyPeer);
    config.setProtocol(QSsl::TlsV1_2OrLater);
    if (cli.isSet("ca-file")) {
        const auto ca = QSslCertificate::fromPath(cli.value("ca-file"));
        if (ca.isEmpty()) return 2;
        config.setCaCertificates(ca);
    }
    request.setSslConfiguration(config);
    auto* reply = network.get(request);
    reply->setReadBufferSize(64 * 1024);
    bool encrypted = false;
    bool tlsErrors = false;
    QSsl::SslProtocol negotiated = QSsl::UnknownProtocol;
    qint64 bytes = 0;
    QTimer deadline;
    deadline.setSingleShot(true);
    QObject::connect(&deadline, &QTimer::timeout, reply, &QNetworkReply::abort);
    QObject::connect(reply, &QNetworkReply::encrypted, &app, [&] {
        encrypted = true;
        // Capture while the socket is alive; HTTP/1.0 may close it before finished().
        negotiated = reply->sslConfiguration().sessionProtocol();
    });
    QObject::connect(reply, &QNetworkReply::sslErrors, &app, [&](const QList<QSslError>&) { tlsErrors = true; });
    QObject::connect(reply, &QNetworkReply::readyRead, &app, [&] {
        bytes += reply->readAll().size();
        if (bytes > 20 * 1024 * 1024) reply->abort();
    });
    QObject::connect(reply, &QNetworkReply::finished, &app, [&] {
        deadline.stop();
        const bool tlsPass = ssl && https && !backend.isEmpty() && encrypted && !tlsErrors
            && (negotiated == QSsl::TlsV1_2 || negotiated == QSsl::TlsV1_3);
        result["encryptedHandshake"] = encrypted;
        result["tlsErrors"] = tlsErrors;
        result["negotiatedProtocol"] = int(negotiated);
        result["networkError"] = int(reply->error());
        result["httpStatus"] = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        result["tlsCapabilityPass"] = tlsPass;
        result["requestSucceeded"] = reply->error() == QNetworkReply::NoError;
        const bool pass = sqliteOk && tlsPass && reply->error() == QNetworkReply::NoError;
        result["automatedPass"] = pass;
        QSaveFile file(output);
        file.setDirectWriteFallback(false);
        const auto bytes = QJsonDocument(result).toJson();
        const bool saved = file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.commit();
        reply->deleteLater();
        app.exit(saved && pass ? 0 : 1);
    });
    deadline.start(20000);
    return app.exec();
}
