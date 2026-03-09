#ifndef ENVIRONMENT_LOGS_SERVICE_H
#define ENVIRONMENT_LOGS_SERVICE_H

#include <QVector>
#include <QDateTime>
#include <QString>

struct LogEntry
{
    QString time;
    QString line;
    QString type;
    QString message;
    QDateTime eventAt;
};

class EnvironmentLogsService
{
public:
    QVector<LogEntry> fetchRecentLogs();
    QVector<LogEntry> fetchLogsAfter(const QDateTime &lastEventAt);
    bool insertLog(int process,
                   const QString &sensorType,
                   const QString &sensorValue,
                   const QString &description);

private:
    QString resolveLine(const QString &processId) const;
    QString resolveType(const QString &desc) const;
};

#endif // ENVIRONMENT_LOGS_SERVICE_H
