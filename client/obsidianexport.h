#ifndef OBSIDIANEXPORT_H
#define OBSIDIANEXPORT_H

#include <QString>
#include "task.h"

class ObsidianExporter
{
public:
    ObsidianExporter();

    // Export the entire task tree to the specified directory in Obsidian format.
    // Returns true on success, false on failure.
    bool exportToDirectory(const QString& rootDir);

    QString lastError() const;

private:
    bool exportTask(const PTask& task, const QString& parentDir);
    bool writeMarkdownFile(const QString& filePath, const PTask& task);
    bool writeTimelineFile(const QString& filePath, const PTask& task);
    QString htmlToMarkdown(const QString& html);
    QString sanitizeFilename(const QString& name);

    QString mLastError;
};

#endif // OBSIDIANEXPORT_H
