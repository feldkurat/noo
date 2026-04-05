#include "obsidianexport.h"
#include "storage.h"

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QTextDocument>
#include <QDateTime>

ObsidianExporter::ObsidianExporter()
{
}

bool ObsidianExporter::exportToDirectory(const QString& rootDir)
{
    QDir dir(rootDir);
    if (!dir.exists())
    {
        if (!dir.mkpath("."))
        {
            mLastError = QString("Failed to create directory: %1").arg(rootDir);
            return false;
        }
    }

    TaskArray& topTasks = Storage::instance().topOfTaskTree();
    for (const PTask& task : topTasks)
    {
        if (!exportTask(task, rootDir))
            return false;
    }

    return true;
}

bool ObsidianExporter::exportTask(const PTask& task, const QString& parentDir)
{
    QString safeName = sanitizeFilename(task->title());
    if (safeName.isEmpty())
        safeName = QString("untitled_%1").arg(task->id());

    TaskArray& children = task->children();
    bool hasChildren = !children.isEmpty();

    if (hasChildren)
    {
        // Create a directory for this task
        QString taskDir = parentDir + QDir::separator() + safeName;
        QDir dir(taskDir);
        if (!dir.mkpath("."))
        {
            mLastError = QString("Failed to create directory: %1").arg(taskDir);
            return false;
        }

        // Write the task's own content as <name>.md inside its directory
        QString mdPath = taskDir + QDir::separator() + safeName + ".md";
        if (!writeMarkdownFile(mdPath, task))
            return false;

        // Write timeline if present
        if (!writeTimelineFile(taskDir + QDir::separator() + "timeline.txt", task))
            return false;

        // Export children
        for (const PTask& child : children)
        {
            if (!exportTask(child, taskDir))
                return false;
        }
    }
    else
    {
        // Leaf task: just a markdown file
        QString mdPath = parentDir + QDir::separator() + safeName + ".md";
        if (!writeMarkdownFile(mdPath, task))
            return false;

        // Write timeline if present
        if (!writeTimelineFile(parentDir + QDir::separator() + safeName + " - timeline.txt", task))
            return false;
    }

    return true;
}

bool ObsidianExporter::writeMarkdownFile(const QString& filePath, const PTask& task)
{
    // Ensure content is loaded
    if (!task->isContentLoaded())
        task->loadContent();

    QString markdown = htmlToMarkdown(task->html());

    // Normalize line endings to \r\n
    markdown.replace("\r\n", "\n");
    markdown.replace("\n", "\r\n");

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
    {
        mLastError = QString("Failed to write file: %1").arg(filePath);
        return false;
    }

    QTextStream out(&file);
    out << "# " << task->title() << "\r\n\r\n";
    out << markdown << "\r\n";
    file.close();
    return true;
}

bool ObsidianExporter::writeTimelineFile(const QString& filePath, const PTask& task)
{
    // Ensure content is loaded (timeline loads with content)
    if (!task->isContentLoaded())
        task->loadContent();

    PTimeLine tl = task->timeline();
    if (!tl)
        return true;

    TimeArray& records = tl->data();
    if (records.empty())
        return true;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
    {
        mLastError = QString("Failed to write file: %1").arg(filePath);
        return false;
    }

    QTextStream out(&file);
    out << "# Time records for: " << task->title() << "\r\n";
    out << "# Total time: " << QString::fromStdString(helper::chrono::secondsToDisplay(tl->totalTime(), true)) << "\r\n\r\n";

    for (const TimeRecord& rec : records)
    {
        QDateTime start = QDateTime::fromSecsSinceEpoch(rec.startTime(), Qt::LocalTime);
        QDateTime end = QDateTime::fromSecsSinceEpoch(rec.endTime(), Qt::LocalTime);
        int duration = rec.endTime() - rec.startTime();
        QString durationStr = QString::fromStdString(helper::chrono::secondsToDisplay(duration, true));

        out << start.toString("yyyy-MM-dd HH:mm:ss")
            << "  ->  "
            << end.toString("yyyy-MM-dd HH:mm:ss")
            << "  (" << durationStr << ")\r\n";
    }

    file.close();
    return true;
}

QString ObsidianExporter::htmlToMarkdown(const QString& html)
{
    if (html.isEmpty())
        return QString();

    QTextDocument doc;
    doc.setHtml(html);

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    return doc.toMarkdown();
#else
    return doc.toPlainText();
#endif
}

QString ObsidianExporter::sanitizeFilename(const QString& name)
{
    QString result = name.trimmed();

    // Replace characters not allowed in filenames
    static const QChar forbidden[] = { '/', '\\', ':', '*', '?', '"', '<', '>', '|' };
    for (QChar ch : forbidden)
        result.replace(ch, '_');

    // Trim trailing dots and spaces (Windows compatibility)
    while (result.endsWith('.') || result.endsWith(' '))
        result.chop(1);

    // Limit length
    if (result.length() > 200)
        result = result.left(200);

    return result;
}

QString ObsidianExporter::lastError() const
{
    return mLastError;
}
