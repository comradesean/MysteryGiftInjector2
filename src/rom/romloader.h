#ifndef ROMLOADER_H
#define ROMLOADER_H

#include <QString>
#include <QStringList>

class RomDatabase;

class RomLoader
{
public:
    struct RomSearchResult {
        bool found;
        QString path;
        QString versionName;
        QString md5;
        QString errorMessage;
    };

    RomLoader();
    ~RomLoader();

    // Search for a valid ROM in the given directory and subdirectories
    RomSearchResult findRom(const QString &searchDir, RomDatabase *db);

    // Compute MD5 hash of a file
    static QString computeMD5(const QString &filePath);

    // Expected GBA ROM size (16 MB)
    static constexpr qint64 GBA_ROM_SIZE = 16777216;

private:
    // Get list of standard ROM filenames to check
    QStringList getStandardFilenames() const;

    // Find all .gba files with expected size (recursive)
    QStringList findRomsBySize(const QString &dir, qint64 expectedSize, bool recursive = true) const;

    // Recursive directory search helper
    void searchDirectoryRecursive(const QString &dir, qint64 expectedSize, QStringList &results, int maxDepth = 3) const;

    // Try to identify and validate a ROM file
    RomSearchResult tryRomFile(const QString &path, RomDatabase *db);
};

#endif // ROMLOADER_H
