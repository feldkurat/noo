#include "timetreemodel.h"
#include "helper.h"

#include <QFontMetrics>
#include <assert.h>

using namespace helper;

#ifdef USE_SIMPLE_TIMETREE

TimeTreeModel::TimeTreeModel(PTimeLine timeline)
    :mTimeLine(timeline)
{
}

TimeTreeModel::~TimeTreeModel()
{
}

void TimeTreeModel::setTimeLine(PTimeLine timeline)
{
    mTimeLine = timeline;
}

QModelIndex TimeTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    return createIndex(row, column);
}

QModelIndex TimeTreeModel::parent(const QModelIndex &child) const
{
    return QModelIndex();
}

int TimeTreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return mTimeLine->data().count();
}

int TimeTreeModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}

QVariant TimeTreeModel::data(const QModelIndex &index, int role) const
{
    TimeRecord& tr = mTimeLine->data()[index.row()];
    switch (role)
    {
    case Qt::DisplayRole:
        return QString("%1 - %2").arg(tr.startTime().toLocalTime().toString( Qt::SystemLocaleShortDate), tr.endTime().toLocalTime().toString(Qt::SystemLocaleShortDate));
    }
    return QVariant();
}

bool TimeTreeModel::setData(const QModelIndex &index, const QVariant &value, int role /* = Qt::EditRole */)
{
    return false;
}

Qt::ItemFlags TimeTreeModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

void TimeTreeModel::beginAddRow()
{
    beginInsertRows(QModelIndex(), 0, 0);
}

void TimeTreeModel::endAddRow()
{
    endInsertRows();
}

void TimeTreeModel::timeUpdated()
{
    dataChanged(createIndex(0, 0), createIndex(0, 0));
}

QVariant TimeTreeModel::headerData(int section, Qt::Orientation orientation, int role /* = Qt::DisplayRole */) const
{
    return "";//QVariant();
}

#else

/*
static int PackDate(Level l, int year, int month, int day, quint64 intervalId)
{
  return l + (day << 2) + (month << 7) + (year << 11);
}

static void UnpackDate(int packed, Level& l, int& year, int& month, int& day, quint64& intervalId)
{
  l = (Level)(packed & 3);
  day = (packed >> 2) & 31;
  month = (packed >> 7) & 15;
  year = (packed >> 11) & 0x7FFF;
}
*/


TimeTreeModel::TimeTreeModel(PTimeLine timeline, Settings& settings)
    :mTimeLine(timeline), mSettings(settings), mItemIdGenerator(0)
{
    if (settings.data()[KEY_SHOW_SECONDS].toBool())
        mTimeFormat = "hh:mm:ss";
    else
        mTimeFormat = "hh:mm";
}

TimeTreeModel::~TimeTreeModel()
{
}

void TimeTreeModel::setTimeLine(PTimeLine timeline)
{
    mTimeLine = timeline;
}

QModelIndex TimeTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    std::set<int> components;
    std::set<int>::iterator iter;
    Level l;
    int id = 0, year = 0, month = 0, day = 0;
    quint64 intervalId = 0;
    std::vector<TimeRecord> intervals;
    if (parent.isValid())
    {
        // Find parent date
        UnpackDate(parent.internalId(), l, year, month, day, intervalId);
        l = (Level)((int)l + 1);
    }
    else
        l = Level_Year;

    switch (l)
    {
    case Level_Year:
        components = mTimeLine->getYears();
        iter = components.begin();
        std::advance(iter, row);
        id = PackDate(Level_Year, *iter, 1, 1, 0);
        break;

    case Level_Month:
        // Find monthes set
        components = mTimeLine->getMonthes(year);
        iter = components.begin();
        std::advance(iter, row);
        month = *iter;

        // Save year month to id
        id = PackDate(Level_Month, year, month, 1, 0);
        break;

    case Level_Day:
        // Get set of available days
        components = mTimeLine->getDays(year, month);

        // Find day corresponding by requested row
        iter = components.begin();
        std::advance(iter, row);
        day = *iter;

        // Pack date to internal id
        id = PackDate(Level_Day, year, month, day, 0);
        break;

    case Level_Time:
        // Get time intervals related to that day
        mTimeLine->getTime(year, month, day, &intervals);

        // internal id will refer corresponding DB record
        id = PackDate(Level_Time, year, month, day, intervals[row].id());
        break;
    }

    return createIndex(row, column, id);
}

QModelIndex TimeTreeModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();

    std::set<int> components;
    std::set<int>::iterator iter;
    int year, month, day, row;
    quint64 intervalId;
    Level l;
    UnpackDate(child.internalId(), l, year, month, day, intervalId);

    switch (l)
    {
    case Level_Year:
        return QModelIndex();

    case Level_Month:
        components = mTimeLine->getYears();
        iter = components.find(year);
        row = std::distance(components.begin(), iter);
        return createIndex(row, 0, PackDate(Level_Year, year, 1, 1, 0));

    case Level_Day:
        components = mTimeLine->getMonthes(year);
        iter = components.find(month);
        row = std::distance(components.begin(), iter);
        return createIndex(row, 0, PackDate(Level_Month, year, month, 1, 0));

    case Level_Time:
        components = mTimeLine->getDays(year, month);
        iter = components.find(day);
        row = std::distance(components.begin(), iter);
        return createIndex(row, 0, PackDate(Level_Day, year, month, day, 0));
    }
    assert(0);
}


int TimeTreeModel::rowCount(const QModelIndex &parent) const
{
    int result = 0, year = 0, month = 0, day = 0;
    quint64 intervalId;

    Level l;
    if (!parent.isValid())
        l = Level_Year;
    else
    {
        UnpackDate(parent.internalId(), l, year, month, day, intervalId);
        l = Level((int)l + 1);
    }

    std::set<int> rows;
    switch (l)
    {
    case Level_Month:
        // Find how much monthes are in that year related records
        rows = mTimeLine->getMonthes(year);
        result = rows.size();
        break;

    case Level_Day:
        // Find how much days are in that year&month related records
        rows = mTimeLine->getDays(year, month);
        result = rows.size();
        break;

    case Level_Time:
        // Find how much time intervals are in that year&month&day related records
        result = mTimeLine->getTime(year, month, day, nullptr);
        break;

    case Level_Year:
        rows = mTimeLine->getYears();
        result = rows.size();
        break;

    default:
        return 0;
    }

    return result;
}

int TimeTreeModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}

static QString monthToString(int month)
{
    return QLocale::system().monthName(month);
}

QVariant TimeTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (role != Qt::DisplayRole)
        return QVariant();

    int year, month, day;
    quint64 intervalId;
    std::vector<TimeRecord> intervals;
    TimeRecord tr;
    Level l;
    UnpackDate(index.internalId(), l, year, month, day, intervalId);

    switch(l)
    {
    case Level_Year:
        return QString::number(year);

    case Level_Month:
        return monthToString(month);

    case Level_Day:
        return QString::number(day);

    case Level_Time:
    {
        mTimeLine->getTime(year, month, day, &intervals);
        tr = intervals[index.row()];

        // Intervals are in local time already
        // ToDo: they are in GMT!
        helper::time start = helper::time::fromTimestamp(tr.startTime(), helper::date::To_LocalTime),
                     end = helper::time::fromTimestamp(tr.endTime(), helper::date::To_LocalTime);

        return QString("%1 - %2").arg(QString::fromStdString(start.toString(false)),
                                      QString::fromStdString(end.toString(false)));
    }

    default:
        return QVariant();
    }
}

bool TimeTreeModel::setData(const QModelIndex &index, const QVariant &value, int role /* = Qt::EditRole */)
{
    return QAbstractItemModel::setData(index, value, role);
}

Qt::ItemFlags TimeTreeModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

void TimeTreeModel::beginAddRow()
{
    beginInsertRows(QModelIndex(), 0, 0);
}

void TimeTreeModel::endAddRow()
{
    endInsertRows();
}

void TimeTreeModel::timeUpdated()
{
    dataChanged(createIndex(0, 0), createIndex(0, this->columnCount()-1));
}


QVariant TimeTreeModel::headerData(int section, Qt::Orientation orientation, int role /* = Qt::DisplayRole */) const
{
    return QVariant();
}

TimeRecord TimeTreeModel::findInterval(const QModelIndex &index)
{
    int year, month, day;
    quint64 intervalId;
    std::vector<TimeRecord> intervals;
    TimeRecord tr;
    Level l;
    UnpackDate(index.internalId(), l, year, month, day, intervalId);

    switch(l)
    {
    case Level_Year:
    case Level_Month:
    case Level_Day:
        return TimeRecord();

    case Level_Time:
        mTimeLine->getTime(year, month, day, &intervals);
        return intervals[index.row()];

    default:
        return TimeRecord();
    }
}

void TimeTreeModel::cutInterval(const QModelIndex& index)
{
    TimeRecord t = findInterval(index);
    if (!t.id())
        return;

    int year, month, day;
    quint64 intervalId;
    std::vector<TimeRecord> intervals;
    TimeRecord tr;
    Level l;
    UnpackDate(index.internalId(), l, year, month, day, intervalId);

    if (l != Level_Time)
        return;

    beginRemoveRows(index.parent(), index.row(), index.row());
    mTimeLine->cutInterval(t);
    mTimeLine->save();
    endRemoveRows();
}

void TimeTreeModel::insertInterval(const TimeRecord &interval)
{
    mTimeLine->insertInterval(interval);
    /*
  // Get local time and see what rows are affected using date only
  QDate day = interval.startTime().toLocalTime().date();
  while (day >= interval.startTime().toLocalTime().date() && day <= interval.endTime().toLocalTime().date())
  {
    QModelIndex dayIndex = dayToIndex(day);

    // Notify about new row
    beginInsertRows(dayIndex, 0, 0);
    endInsertRows();

    // Notify about changed time records for this day
    std::vector<TimeRecord> intervals;
    mTimeLine->getTime(day.year(), day.month(), day.day(), &intervals);

    // Internal id will refer corresponding DB record
    QModelIndex tl = createIndex(0, 0, PackDate(Level_Time, day.year(), day.month(), day.day(), intervals.front().id()));
    QModelIndex br = createIndex(intervals.size()-1, 0, PackDate(Level_Time, day.year(), day.month(), day.day(), intervals.back().id()));
    emit dataChanged(tl, br);

    day = day.addDays(1);
  }
  */
}

bool operator < (const TimeTreeModel::Item& lhs, const TimeTreeModel::Item& rhs)
{
    if (lhs.mLevel < rhs.mLevel)
        return true;
    if (lhs.mLevel > rhs.mLevel)
        return false;

    if (lhs.mYear < rhs.mYear)
        return true;
    if (lhs.mYear > rhs.mYear)
        return false;

    if (lhs.mMonth < rhs.mMonth)
        return true;
    if (lhs.mMonth > rhs.mMonth)
        return false;

    if (lhs.mDay < rhs.mDay)
        return true;
    if (lhs.mDay > rhs.mDay)
        return false;

    if (lhs.mTimeIntervalId < rhs.mTimeIntervalId)
        return true;

    return false;
}

int TimeTreeModel::PackDate(Level l, int year, int month, int day, quint64 intervalId) const
{
    Item item;
    item.mLevel = l;
    item.mYear = year;
    item.mMonth = month;
    item.mDay = day;
    item.mTimeIntervalId = intervalId;

    std::map<Item, int>::iterator iter = mItem2Id.find(item);
    if (iter != mItem2Id.end())
        return iter->second;

    mItemIdGenerator++;
    mItem2Id[item] = mItemIdGenerator;
    mId2Item[mItemIdGenerator] = item;

    return mItemIdGenerator;
}

void TimeTreeModel::UnpackDate(int packed, Level& l, int& year, int& month, int& day, quint64& intervalId) const
{
    std::map<int, Item>::const_iterator iter = mId2Item.find(packed);
    if (iter != mId2Item.end())
    {
        l = iter->second.mLevel;
        year = iter->second.mYear;
        month = iter->second.mMonth;
        day = iter->second.mDay;
        intervalId = iter->second.mTimeIntervalId;
    }
}

QModelIndex TimeTreeModel::dayToIndex(const QDate& date)
{
    std::set<int> components;
    std::set<int>::iterator iter;
    components = mTimeLine->getYears();

    iter = components.find(date.year());
    if (iter == components.end())
        return QModelIndex();
    int yearRow = std::distance(components.begin(), iter);
    QModelIndex yearIndex = this->createIndex(yearRow, 0, PackDate(Level_Year, date.year(), 1, 1, 0));

    components = mTimeLine->getMonthes(date.year());
    iter = components.find(date.month());
    if (iter == components.end())
        return QModelIndex();
    int monthRow = std::distance(components.begin(), iter);
    QModelIndex monthIndex = this->createIndex(monthRow, 0, PackDate(Level_Month, date.year(), date.month(), 1, 0));

    components = mTimeLine->getDays(date.year(), date.month());
    iter = components.find(date.day());
    if (iter == components.end())
        return QModelIndex();
    int dayRow = std::distance(components.begin(), iter);
    QModelIndex dayIndex = this->createIndex(dayRow, 0, PackDate(Level_Day, date.year(), date.month(), date.day(), 0));

    return dayIndex;
}

#endif
