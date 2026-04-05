# Noo Export to Obsidian Format

## Overview

Noo can export its database to a directory structure compatible with [Obsidian](https://obsidian.md/) vaults. The export converts the hierarchical task tree into directories with Markdown files, and time tracking data into plain text timeline files.

## Access

**File > Export to Obsidian...** — select an empty or existing directory as the export target.

## Directory Structure

The task tree hierarchy maps directly to the filesystem:

- **Task with children** becomes a **directory** named after the task.
  - Its own content is saved as `<Task Title>.md` inside that directory.
  - Its time records are saved as `timeline.txt` inside that directory.
  - Child tasks appear as files or subdirectories within.

- **Leaf task** (no children) becomes a **Markdown file** `<Task Title>.md` in its parent directory.
  - Its time records are saved as `<Task Title> - timeline.txt` alongside the `.md` file.

### Example

Given this task tree in Noo:

```
Work
├── Project Alpha
│   ├── Design
│   └── Implementation
└── Project Beta
Notes
```

The export produces:

```
export-dir/
├── Work/
│   ├── Work.md
│   ├── timeline.txt
│   ├── Project Alpha/
│   │   ├── Project Alpha.md
│   │   ├── timeline.txt
│   │   ├── Design.md
│   │   ├── Design - timeline.txt
│   │   ├── Implementation.md
│   │   └── Implementation - timeline.txt
│   ├── Project Beta.md
│   └── Project Beta - timeline.txt
├── Notes.md
└── Notes - timeline.txt
```

## Markdown Files

Each `.md` file starts with a level-1 heading matching the task title, followed by the task content converted from HTML to Markdown.

```markdown
# Task Title

Task content here...
```

The HTML-to-Markdown conversion uses Qt's `QTextDocument::toMarkdown()` (Qt 5.14+). On older Qt versions, content falls back to plain text.

## Timeline Files

Timeline files are plain text with a header showing the task name and total tracked time, followed by one line per time record.

```
# Time records for: Task Title
# Total time: 12:45:30

2026-01-10 09:00:00  ->  2026-01-10 12:30:00  (03:30:00)
2026-01-11 14:00:00  ->  2026-01-11 17:15:30  (03:15:30)
2026-01-12 10:00:00  ->  2026-01-12 16:00:00  (06:00:00)
```

Each line contains:

| Field | Format | Description |
|-------|--------|-------------|
| Start | `yyyy-MM-dd HH:mm:ss` | Local time when work started |
| End | `yyyy-MM-dd HH:mm:ss` | Local time when work ended |
| Duration | `HH:MM:SS` | Length of the interval |

Timeline files are only created when a task has at least one time record. Tasks with no tracked time produce no timeline file.

## Filename Sanitization

Task titles are sanitized for filesystem compatibility:

- Characters `/ \ : * ? " < > |` are replaced with `_`
- Trailing dots and spaces are stripped
- Length is capped at 200 characters
- Tasks with empty titles are named `untitled_<id>`
