#include "findsupport.h"
#include "task.h"
#include "storage.h"

FindResultsModel::FindResultsModel(QObject* parent)
  :QAbstractTableModel(parent)
{
}

int FindResultsModel::columnCount(const QModelIndex &parent) const
{
  if (parent.isValid())
    return 0;
  return 2;
}

int FindResultsModel::rowCount(const QModelIndex &parent) const
{
  return mItemList.count();
}

QVariant FindResultsModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid())
    return QVariant();

  FindResultItem item = mItemList[index.row()];
  switch (role)
  {
  case Qt::DisplayRole:
    return index.column() == 0 ? item.mTask->title() : "..." + item.mPattern + "...";
  }
  return QVariant();
}

Qt::ItemFlags FindResultsModel::flags(const QModelIndex &index) const
{
  Qt::ItemFlags result = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  return result;
}

QVariant FindResultsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
  {
    switch (section)
    {
    case 0: return tr("Task title");
    case 1: return tr("Occurence");
    }
  }
  return QVariant();
}

void FindResultsModel::addItem(const FindResultItem& item)
{
  beginInsertRows(QModelIndex(), rowCount(), rowCount());
  mItemList.append(item);
  endInsertRows();
}

void FindResultsModel::clear()
{
  beginRemoveRows(QModelIndex(), 0, rowCount()-1);
  mItemList.clear();
  endRemoveRows();
}

const FindResultItem& FindResultsModel::getItem(int row) const
{
  return mItemList.at(row);
}
// ------ TaskSearch -------
TaskSearch::TaskSearch()
  :mShutdown(false)
{

}

TaskSearch::~TaskSearch()
{
  //stop();
}

void TaskSearch::start(const QString &request)
{
  mShutdown = false;
  mRequest = request;
  QThread::start();
}

void TaskSearch::stop()
{
  mShutdown = true;
  QThread::wait();
  mRequest.clear();
}

bool TaskSearch::isRunning()
{
  return false;
}

static int findInString(const QStringList& pattern, const QString& doc, int& firstIndex, QString& occurenceSubstring, QString& foundWord)
{
  int result = 0;
  firstIndex = -1;
  for (const QString& s: pattern)
  {
    int wordIndex = doc.indexOf(s);
    if (wordIndex >= 0)
    {
      if (firstIndex == -1)
      {
        firstIndex = wordIndex;

        // Copy pattern
        int startIndex = firstIndex - 15;
        if (startIndex < 0)
          startIndex = 0;
        int endIndex = firstIndex + 15;
        if (endIndex >= doc.length())
          endIndex = doc.length() - 1;
        occurenceSubstring = doc.mid(startIndex, endIndex - startIndex + 1);
        occurenceSubstring.replace("\n", " ").replace("\r", "");
        foundWord = s;
      }
      result++;
    }
  }
  return result;
}

void TaskSearch::run()
{
  // Reserve array with initial length to make copy
  TaskArray tasks(Storage::instance().topOfTaskTree().count());

  // Copy top level tasks
  std::copy(Storage::instance().topOfTaskTree().begin(), Storage::instance().topOfTaskTree().end(), tasks.begin());

  QStringList sl;
  if (mRequest.startsWith("\"") && mRequest.endsWith("\""))
    sl.append(mRequest);
  else
   sl = mRequest.split(" ", Qt::SkipEmptyParts);

  if (!sl.empty())
  {
    while (tasks.count() > 0 && !mShutdown)
    {
      PTask current = tasks.front();
      tasks.erase(tasks.begin());

      // Add children to tasks list
      for (PTask child: current->children())
        tasks.push_back(child);

      // Search in title
      FindResultItem titleSearchItem;
      titleSearchItem.mIndex = -1;
      titleSearchItem.mOccurences = findInString(sl, current->title(), titleSearchItem.mIndex, titleSearchItem.mPattern, titleSearchItem.mFoundWord);

      // Get content from task
      bool loadedThisTime = false;
      if (!current->isContentLoaded())
        current->loadContent();
      QString content = current->html();

      // Search in content
      FindResultItem contentSearchItem;
      contentSearchItem.mIndex = -1;
      contentSearchItem.mOccurences = findInString(sl, content, contentSearchItem.mIndex, contentSearchItem.mPattern, contentSearchItem.mFoundWord);
      contentSearchItem.mTask = current;

      if (contentSearchItem.mOccurences + titleSearchItem.mOccurences >= sl.count())
      {
        emit newResultAvailable(contentSearchItem);
      }
      // Unload content if it is not needed now
      if (loadedThisTime)
        current->unloadContent();

    }
  }
  emit searchComplete();
}
