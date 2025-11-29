#include "romloader.h"
#include "romdatabase.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QDebug>

RomLoader::RomLoader()
{
}

RomLoader::~RomLoader()
{
}

QStringList RomLoader::getStandardFilenames() const
{
    return {
        // Official dump names (USA)
        "Pokemon - FireRed Version (USA).gba",
        "Pokemon - FireRed Version (USA, Europe).gba",
        "Pokemon - FireRed Version (USA, Europe) (Rev 1).gba",
        "Pokemon - LeafGreen Version (USA).gba",
        "Pokemon - LeafGreen Version (USA, Europe).gba",
        "Pokemon - LeafGreen Version (USA, Europe) (Rev 1).gba",
        "Pokemon - Emerald Version (USA).gba",
        "Pokemon - Emerald Version (USA, Europe).gba",

        // Short/common names
        "firered.gba",
        "leafgreen.gba",
        "emerald.gba",
        "pokemon_firered.gba",
        "pokemon_leafgreen.gba",
        "pokemon_emerald.gba",
        "pokemonfirered.gba",
        "pokemonleafgreen.gba",
        "pokemonemerald.gba",

        // Abbreviated names
        "fr.gba",
        "lg.gba",
        "em.gba",
        "poke_fr.gba",
        "poke_lg.gba",
        "poke_em.gba"
    };
}

QStringList RomLoader::findRomsBySize(const QString &dir, qint64 expectedSize, bool recursive) const
{
    QStringList result;

    if (recursive) {
        searchDirectoryRecursive(dir, expectedSize, result, 3);  // Max 3 levels deep
    } else {
        QDir directory(dir);
        QStringList filters;
        filters << "*.gba" << "*.GBA";

        QFileInfoList files = directory.entryInfoList(filters, QDir::Files);
        for (const QFileInfo &fileInfo : files) {
            if (fileInfo.size() == expectedSize) {
                result.append(fileInfo.absoluteFilePath());
            }
        }
    }

    return result;
}

void RomLoader::searchDirectoryRecursive(const QString &dir, qint64 expectedSize, QStringList &results, int maxDepth) const
{
    if (maxDepth <= 0) return;

    QDir directory(dir);

    // Check .gba files in this directory
    QStringList filters;
    filters << "*.gba" << "*.GBA";

    QFileInfoList files = directory.entryInfoList(filters, QDir::Files);
    for (const QFileInfo &fileInfo : files) {
        if (fileInfo.size() == expectedSize) {
            results.append(fileInfo.absoluteFilePath());
        }
    }

    // Recurse into subdirectories
    QFileInfoList subdirs = directory.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &subdir : subdirs) {
        // Skip common non-ROM directories
        QString name = subdir.fileName().toLower();
        if (name == "build" || name == ".git" || name == "node_modules" ||
            name == "__pycache__" || name == ".cache") {
            continue;
        }
        searchDirectoryRecursive(subdir.absoluteFilePath(), expectedSize, results, maxDepth - 1);
    }
}

QString RomLoader::computeMD5(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file for MD5:" << filePath;
        return QString();
    }

    QCryptographicHash hash(QCryptographicHash::Md5);

    // Read in chunks to handle large files efficiently
    const qint64 chunkSize = 1024 * 1024; // 1 MB chunks
    while (!file.atEnd()) {
        QByteArray chunk = file.read(chunkSize);
        hash.addData(chunk);
    }

    file.close();

    return hash.result().toHex().toLower();
}

RomLoader::RomSearchResult RomLoader::tryRomFile(const QString &path, RomDatabase *db)
{
    RomSearchResult result;
    result.found = false;
    result.path = path;

    // Check file exists and has correct size
    QFileInfo fileInfo(path);
    if (!fileInfo.exists()) {
        result.errorMessage = "File does not exist";
        return result;
    }

    if (fileInfo.size() != GBA_ROM_SIZE) {
        result.errorMessage = QString("Invalid file size: %1 (expected %2)")
            .arg(fileInfo.size())
            .arg(GBA_ROM_SIZE);
        return result;
    }

    // Compute MD5
    result.md5 = computeMD5(path);
    if (result.md5.isEmpty()) {
        result.errorMessage = "Failed to compute MD5 hash";
        return result;
    }

    // Try to identify ROM
    const RomDatabase::RomVersion *version = db->identifyRom(result.md5);
    if (version) {
        result.found = true;
        result.versionName = version->name;
        qDebug() << "ROM identified:" << result.versionName << "MD5:" << result.md5;
    } else {
        result.errorMessage = QString("Unknown ROM (MD5: %1)").arg(result.md5);
        qDebug() << "ROM not recognized:" << result.md5;
    }

    return result;
}

RomLoader::RomSearchResult RomLoader::findRom(const QString &appDir, RomDatabase *db)
{
    RomSearchResult result;
    result.found = false;

    if (!db || !db->isLoaded()) {
        result.errorMessage = "ROM database not loaded";
        return result;
    }

    qDebug() << "Searching for ROM in:" << appDir;

    // Phase 1: Check standard filenames
    QStringList standardNames = getStandardFilenames();
    for (const QString &filename : standardNames) {
        QString fullPath = QDir(appDir).absoluteFilePath(filename);
        QFileInfo fileInfo(fullPath);

        if (fileInfo.exists()) {
            qDebug() << "Found standard filename:" << filename;
            result = tryRomFile(fullPath, db);
            if (result.found) {
                return result;
            }
            // File exists but wasn't recognized, continue searching
        }
    }

    // Phase 2: Search for any .gba file with correct size
    qDebug() << "No standard filenames found, searching by size...";
    QStringList romsBySize = findRomsBySize(appDir, GBA_ROM_SIZE);

    for (const QString &romPath : romsBySize) {
        qDebug() << "Checking ROM by size:" << romPath;
        result = tryRomFile(romPath, db);
        if (result.found) {
            return result;
        }
    }

    // Phase 3: No ROM found
    result.found = false;
    result.errorMessage = "No valid Pokemon Gen3 ROM found in application directory";
    qDebug() << result.errorMessage;

    return result;
}
