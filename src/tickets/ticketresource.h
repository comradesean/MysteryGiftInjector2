#ifndef TICKETRESOURCE_H
#define TICKETRESOURCE_H

#include <QString>
#include <QByteArray>
#include "savefile.h"

/// <summary>
/// Represents a single Mystery Gift ticket with its associated data files.
/// Each ticket contains Wonder Card data, GMScript dialog data, and metadata.
/// </summary>
class TicketResource
{
public:
    TicketResource();
    TicketResource(const QString &id, const QString &name, GameType gameType,
                   const QString &wonderCardFile, const QString &scriptFile,
                   const QString &description = QString());

    // Accessors
    QString id() const { return m_id; }
    QString name() const { return m_name; }
    GameType gameType() const { return m_gameType; }
    QString gameTypeString() const;
    QString wonderCardFile() const { return m_wonderCardFile; }
    QString scriptFile() const { return m_scriptFile; }
    QString description() const { return m_description; }

    // Data accessors (loaded on demand)
    const QByteArray& wonderCardData() const { return m_wonderCardData; }
    const QByteArray& scriptData() const { return m_scriptData; }
    bool isDataLoaded() const { return m_dataLoaded; }

    // Load binary data from files
    bool loadData(const QString &ticketsFolder, QString &errorMessage);

    // Constants - Wonder Card format (336 bytes total = 0x150)
    static const qint64 WONDERCARD_SIZE = 336;           // Total file size with header
    static const qint64 WONDERCARD_HEADER_SIZE = 4;      // CRC16 (2) + padding (2)
    static const qint64 WONDERCARD_PAYLOAD_SIZE = 332;   // Struct data (330 + 2 alignment)

    // Constants - Script format (1004 bytes total = 0x3EC)
    static const qint64 SCRIPT_SIZE = 1004;              // Total file size with header
    static const qint64 SCRIPT_HEADER_SIZE = 4;          // CRC16 (2) + padding (2)
    static const qint64 SCRIPT_PAYLOAD_SIZE = 1000;      // Script bytecode (0x3E8)

private:
    QString m_id;                 // Unique identifier (e.g., "aurora_emerald")
    QString m_name;               // Display name (e.g., "Aurora Ticket")
    GameType m_gameType;          // Game type (Emerald, FireRedLeafGreen, RubySapphire)
    QString m_wonderCardFile;     // Filename of Wonder Card binary
    QString m_scriptFile;         // Filename of GMScript binary
    QString m_description;        // User-friendly description

    // Loaded binary data
    QByteArray m_wonderCardData;
    QByteArray m_scriptData;
    bool m_dataLoaded;
};

#endif // TICKETRESOURCE_H
