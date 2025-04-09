#include "task.h"
#include "storage.h"
#include "helper.h"

#include <QVariant>
#include <assert.h>

#if defined(TARGET_OSX) || defined(TARGET_LINUX)
# include <uuid/uuid.h>
#endif

using namespace helper;

// -------- WorldId ------
WorldId::WorldId()
{
}

WorldId::WorldId(const WorldId& src)
{
    mId = src.mId;
}

WorldId::WorldId(const std::string &s)
    :mId(s)
{
}

WorldId::~WorldId()
{
}

WorldId& WorldId::operator = (const WorldId& src)
{
    mId = src.mId;
    return *this;
}

bool WorldId::operator == (const WorldId& src)
{
    return mId == src.mId;
}

bool WorldId::operator < (const WorldId& src)
{
    return mId < src.mId;
}

std::string WorldId::asString() const
{
    return mId;
}

WorldId WorldId::create()
{
    WorldId result;
#if defined(TARGET_OSX) || defined(TARGET_LINUX)
    uuid_t t;
    uuid_generate(t);
    char buffer[128];
    uuid_unparse(t, buffer);
    result.mId = buffer;
#endif

#if defined(TARGET_WIN)
#endif

    return result;
}

// -------- TimeRecord ----------
TimeRecord::TimeRecord()
    :mId(0), mTaskId(0), mSaved(false)
{}

TimeRecord::TimeRecord(const time_t &startTime, const time_t &endTime, Id taskId)
    :mId(0), mTaskId(taskId), mStartTime(startTime), mEndTime(endTime), mSaved(false)
{
}

TimeRecord::~TimeRecord()
{}

time_t TimeRecord::endTime() const
{
    return mEndTime;
}

TimeRecord& TimeRecord::setEndTime(const time_t &endTime)
{
    mEndTime = endTime;
    return *this;
}

time_t TimeRecord::startTime() const
{
    return mStartTime;
}

TimeRecord& TimeRecord::setStartTime(const time_t &startTime)
{
    mStartTime = startTime;
    return *this;
}

int TimeRecord::length()
{
    return static_cast<int>(mEndTime - mStartTime);
}

Id TimeRecord::id() const
{
    return mId;
}

TimeRecord& TimeRecord::setId(Id id)
{
    mId = id;
    return *this;
}

Id TimeRecord::taskId() const
{
    return mTaskId;
}

TimeRecord& TimeRecord::setTaskId(Id id)
{
    mTaskId = id;
    return *this;
}

WorldId TimeRecord::worldId() const
{
    return mWorldId;
}

TimeRecord& TimeRecord::setWorldId(const WorldId& id)
{
    mWorldId = id;
    return *this;
}

void TimeRecord::save()
{
    auto id = Storage::instance().saveTimeRecord(*this);
    if (id)
        mId = id;
}

void TimeRecord::deleteRecord()
{
    Storage::instance().deleteTimeRecord(*this);
}

// -------------------- TimeLine --------------------
TimeLine::TimeLine()
    :mTaskId(0), mActive(false), mActiveTimeRecord(nullptr), mTotalTime(0)
{}

TimeLine::~TimeLine()
{}

int TimeLine::findTotalTime()
{
    int result = 0;
    for(auto& t: mData)
    {
        int delta = static_cast<int>(t.endTime() - t.startTime());
        result += delta + 1;
    }
    return result;
}

int TimeLine::totalTime()
{
    return mTotalTime;
}

bool TimeLine::active()
{
    return mActive;
}

void TimeLine::start()
{
    if (mActive)
        return;

    // Mark timeline as active - it means it records time interval now
    mActive = true;

    // Find current time in UTC format
    time_t current = ::time(nullptr);

    // Check if current time point does not belong to any existing time interval
    if (hasTimePoint(current))
        mActiveTimeRecord = nullptr;
    else
        mActiveTimeRecord = makeNewRecord(current, current);
}

TimeRecord* TimeLine::makeNewRecord(time_t beginTime, time_t endTime)
{
    TimeRecord tr;
    tr.setStartTime(beginTime)
      .setEndTime(endTime)
      .setTaskId(mTaskId)
      .save();

    mData.push_back(tr);
    Id intervalId = tr.id();
    sortData();

    std::reverse_iterator<TimeArray::iterator> intervalIter = std::find_if(std::reverse_iterator<TimeArray::iterator>(mData.end()),
                                                                           std::reverse_iterator<TimeArray::iterator>(mData.begin()),
                                                                           [=] (const TimeRecord& tr)
    {
        return tr.id() == intervalId;
    });

    if (intervalIter != std::reverse_iterator<TimeArray::iterator>(mData.begin()))
        return &(*intervalIter);
    else
        return nullptr;
}

TimeRecord* TimeLine::hasTimePoint(time_t t)
{
    if (mData.empty())
        return nullptr;

    TimeRecord* tr = nullptr;
    TimeArray::iterator result;
    result = std::upper_bound(mData.begin(), mData.end(), t,
                              [](time_t t2, const TimeRecord& tr)
                              {
                                return tr.startTime() > t2;
                              });

    if (result == mData.end())
        tr = &mData.back();
    else
        if (result != mData.begin())
            tr = &(*(--result));
        else
            return nullptr;

    return (tr->startTime() <= t && tr->endTime() >= t) ? tr : nullptr;
}

void TimeLine::sortData()
{
    std::sort(mData.begin(), mData.end(), [](const TimeRecord& lhs, const TimeRecord& rhs) { return lhs.startTime() < rhs.startTime();});
}

bool TimeLine::hasIntersection(const TimeRecord &interval)
{
    if (mData.empty())
        return false;

    TimeArray::iterator result;
    result = std::upper_bound(mData.begin(), mData.end(), interval.startTime(),
                              [](time_t t2, const TimeRecord& tr)
    { return tr.startTime() > t2;});

    if (result == mData.end())
    {
        // There is time record which start point is lesser than interval.startTime()
        return (mData.back().endTime() >= interval.startTime());
    }
    else
        if (result != mData.begin())
        {
            TimeRecord& prev = *(result-1);
            TimeRecord& next = *result;
            if (prev.endTime() >= interval.startTime())
                return true;
            if (next.startTime() <= interval.endTime())
                return true;
        }
        else
        {
            if (mData.front().startTime() <= interval.endTime())
                return true;
        }
    return false;
}

void TimeLine::insertInterval(const TimeRecord &interval)
{
    mData.push_back(interval);
    mData.back().save();

    sortData();
}

bool TimeLine::removeInterval(const TimeRecord &interval)
{
    // Find interval by binary search
    if (mData.empty())
        return false;

    TimeArray::iterator result;
    result = std::upper_bound(mData.begin(), mData.end(), interval.startTime(),
                              [](time_t t2, const TimeRecord& tr)
    { return tr.startTime() > t2;});

    if (result == mData.end())
    {
        // There is time record which start point is lesser than interval.startTime()
        if (mData.back().id() == interval.id())
        {
            // Remove from DB
            mData.back().deleteRecord();

            // Remove from memory
            mData.erase(mData.begin() + mData.size() - 1);

            return true;
        }
    }
    else
        if (result != mData.begin())
        {
            result--;
            if (result->id() == interval.id())
            {
                // Remove from DB
                result->deleteRecord();

                // Remove from memory
                mData.erase(result);

                return true;
            }
        }
    return false;
}

void TimeLine::cutInterval(const TimeRecord& interval)
{
    //  Find interval that startTime() time is greater than interval.endTime()
    TimeArray::iterator iter;
    iter = std::upper_bound(mData.begin(), mData.end(), interval.endTime(),
                            [](time_t t2, const TimeRecord& tr)
    { return tr.startTime() > t2;});

    // If all intervals begins later() than requested interval - exit silently
    if (iter == mData.begin())
        return;

    // Go to previous interval - it is first interval to check
    if (iter == mData.end())
        iter = mData.begin() + mData.size() - 1;
    else
        iter--;

    bool done = false;
    // Loop while current interval end time is greater than cut interval start time
    while (iter->endTime() >= interval.startTime())
    {
        if (iter->endTime() > interval.endTime())
        {
            if (iter->startTime() >= interval.startTime())
            {
                iter->setStartTime(iter->endTime() + 1); // Current interval starts in [interval.startTime(), interval.endTime()], but finishes later
                iter->save();
            }
            else
            {
                // Current interval starts before interval.startTime() and finishes later
                // So cut interval will split it to 2 new intervals
                // Also this operation will end loop
                TimeRecord toInsert(*iter);  // Backup current interval
                iter->setEndTime(interval.startTime() - 1)
                     .save();

                toInsert.setStartTime(interval.endTime() + 1)
                        .save();
                mData.insert(++iter, toInsert);

                done = true;
                break;
            }
        }
        else
        {
            if (iter->startTime() >= interval.startTime())
            {
                iter->deleteRecord();
                iter = mData.erase(iter); // Current interval fits into cut interval
            }
            else
            {
                // Current interval starts before cut interval but finishes in cut interval
                iter->setEndTime(interval.startTime() - 1)
                     .save();
                done = true;
                break;
            }
        }
        if (iter == mData.begin())
            break;
        iter--;
    }

    // Look for exact the same interval as specified one
    if (!done)
    {
        iter = std::find_if(mData.begin(), mData.end(),
                            [=] (const TimeRecord& tr)
                            { return tr.id() == interval.id();});
        if (iter != mData.end())
        {
            Storage::instance().deleteTimeRecord(*iter);
            mData.erase(iter);
        }
    }
}


TimeRecord* TimeLine::findIntervalById(Id id)
{
    TimeArray::iterator iter = std::find_if(mData.begin(), mData.end(),
                                            [=] (const TimeRecord& tr)
                                            { return tr.id() == id;});
    if (iter == mData.end())
        return nullptr;
    return &(*iter);
}

void TimeLine::stop(bool updateTimeline)
{
    if (!mActive)
        return;

    if (updateTimeline)
        flush(true, ::time(nullptr));

    mActive = false;
    mActiveTimeRecord = nullptr;
}

void TimeLine::flush(bool saveToDb, time_t currentUtc)
{
    if (!mActive)
        return;

    if (mActiveTimeRecord)
    {
        long delta = currentUtc - mActiveTimeRecord->endTime();
        mActiveTimeRecord->setEndTime(currentUtc);

        TimeRecord* tr = hasTimePoint(currentUtc);
        if (tr && tr != mActiveTimeRecord)
        {
            mActiveTimeRecord->setEndTime(currentUtc)
                              .save();
            mActiveTimeRecord = nullptr;
        }
        if (saveToDb && mActiveTimeRecord)
            mActiveTimeRecord->save();

        mTotalTime += delta;
    }
    else
    {
        if (!hasTimePoint(currentUtc))
        {
            // Start new record here
            mActiveTimeRecord = makeNewRecord(currentUtc, currentUtc);
        }
    }
}

void TimeLine::load()
{
    Storage::instance().loadTimeLine(*this);
}

void TimeLine::save()
{
    // No need to save anything here - everything is saved right on creation/update
}

Id TimeLine::taskId()
{
    return mTaskId;
}

TimeLine& TimeLine::setTaskId(Id id)
{
    mTaskId = id;
    return *this;
}

TimeArray& TimeLine::data()
{
    return mData;
}

std::set<int> TimeLine::getYears()
{
    std::set<int> r;
    if (mData.empty())
        return r;

    date t1 = date::fromTimestamp(mData.front().startTime(), date::To_LocalTime),
         t2 = date::fromTimestamp(mData.back().startTime(), date::To_LocalTime);

    r.insert(t1.mYear);

    // Find higher bound of years - it is last time record
    if (t1.mYear == t2.mYear)
        return r;

    // Try to find next year by binary search
    for (int year = t1.mYear + 1; year <= t2.mYear; year++)
    {
        date yearStart(year, 1, 1);
        TimeArray::iterator iter = std::lower_bound(mData.begin(), mData.end(), yearStart,
                                                    [&](const TimeRecord& lhs, date rhs)
        {
            return date::fromTimestamp(lhs.endTime(), date::To_LocalTime) < rhs;
        });

        if (iter != mData.end())
        {
            // Get found year
            date found_date = date::fromTimestamp(iter->startTime(), date::To_LocalTime);
            if (found_date.mYear <= year)
                r.insert(year);
        }
    }

    return r;
}

std::set<int> TimeLine::getMonthes(int year)
{
    std::set<int> r;

    for (int month = 1; month <= 12; month++)
    {
        date monthBegin(year, month, 1);
        date monthEnd(year, month, date::daysInMonth(year, month));

        // Find range for every month in year [lowest, higher)
        TimeArray::iterator lowest = std::lower_bound(mData.begin(), mData.end(), monthBegin,
                                                      [] (const TimeRecord& tr, const date& d)

        {
            return date::fromTimestamp(tr.endTime(), date::To_LocalTime) < d;
        });

        //TimeArray::iterator higher = std::upper_bound(mData.begin(), mData.end(), monthEnd, [] (const TimeRecord& tr, const QDate& d) { return tr.startTime().toLocalTime().date() < d;});
        if (lowest != mData.end())
        {
            // Current date is local time!
            helper::date currentDate = helper::date::fromTimestamp(lowest->startTime(), helper::date::To_LocalTime);
            if (currentDate.mYear > year)
                continue;
            if (currentDate.mYear == year && currentDate.mMonth > month)
                continue;

            r.insert(month);
        }
    }

    return r;
}

std::set<int> TimeLine::getDays(int year, int month)
{
    std::set<int> r;
    date monthBegin(year, month, 1);
    for (int day = 1; day <= date::daysInMonth(year, month); day++)
    {
        date currentDay(year, month, day);

        TimeArray::iterator lowest = std::lower_bound(mData.begin(), mData.end(), currentDay,
                                                      [] (const TimeRecord& tr, const date& d)
                                                      {
                                                          return date::fromTimestamp(tr.endTime(), date::To_LocalTime) < d;
                                                      });
        if (lowest != mData.end())
        {
            helper::date startDate = helper::date::fromTimestamp(lowest->startTime(), helper::date::To_LocalTime);
            if (startDate.mYear > year)
                continue;
            if (startDate.mYear == year && startDate.mMonth > month)
                continue;
            if (startDate.mYear == year && startDate.mMonth == month && startDate.mDay > day)
                continue;

            r.insert(day);
        }
    }
    return r;
}

int TimeLine::getTime(int year, int month, int day, std::vector<TimeRecord>* intervals)
{
    int result = 0;
    QDate d(year, month, day);

    // Find range of related records [lowest, higher)
    TimeArray::iterator lowest = std::lower_bound(mData.begin(), mData.end(), d, [] (const TimeRecord& tr, const QDate& d)
                                                                                 {
                                                                                    return tr.endTime() < QDateTime(d, QTime()).toSecsSinceEpoch();
                                                                                 });
    //TimeArray::iterator higher = std::upper_bound(mData.begin(), mData.end(), d, [] (const QDate& d, const TimeRecord& tr) { return tr.startTime().toLocalTime().date() < d;});

    for (;lowest != mData.end();/*&& lowest != higher;*/ lowest++)
    {
        TimeRecord& tr = *lowest;

        helper::date startDate = helper::date::fromTimestamp(tr.startTime(), date::To_LocalTime);

        if (startDate.mYear > year)
            break;
        if (startDate.mYear == year && startDate.mMonth > month)
            break;

        if (startDate.mYear == year &&
            startDate.mMonth == month &&
            startDate.mDay > day)
            break;

        time_t dayBegin = helper::date{year, month, day}.toTimestamp();
        time_t dayEnd = dayBegin + 86399;

        if (tr.startTime() > dayBegin)
            dayBegin = tr.startTime(); // Time record begin is later than begin of the day

        if (tr.endTime() < dayEnd)
            dayEnd = tr.endTime();

        if (intervals)
        {
            TimeRecord resultingRecord;
            resultingRecord.setStartTime(dayBegin)
                           .setEndTime(dayEnd)
                           .setId(tr.id())
                           .setTaskId(tr.taskId());
            intervals->push_back(resultingRecord);
        }
        result++;
    }
    return result;
}


int TimeLine::today()
{
    int result = 0;

    // Find starting record related to today
    // Today is local time!
    date today = date::today();

    TimeArray::iterator lowIter = std::lower_bound(mData.begin(), mData.end(), today,
                                                   [] (const TimeRecord& lhs, const date& rhs)
    {
        return date::fromTimestamp(lhs.endTime(), date::To_LocalTime) < rhs;
    });

    for (;lowIter != mData.end(); lowIter++)
    {
        if (date::fromTimestamp(lowIter->startTime(), date::To_LocalTime) > today)
            break; // quit the loop

        if (date::fromTimestamp(lowIter->endTime(), date::To_LocalTime) >= today)
        {
            // Both are GMT time
            time_t dayBegin = date::today().toTimestamp();
            time_t dayEnd = dayBegin + 86359;

            int64_t secondsTo = lowIter->startTime() - dayBegin;
            if (secondsTo > 0)
                dayBegin = lowIter->startTime();

            int64_t secondsFrom = lowIter->endTime() - dayEnd;
            if (secondsFrom < 0)
                dayEnd = lowIter->endTime();

            int secondsSpent = dayEnd - dayBegin;
            result += secondsSpent;
        }
    }

    return result;
}

int TimeLine::month()
{
    int result = 0;

    // Find first day of month
    date this_month = date::today();
    this_month.mDay = 1;

    // Find position in time record array close to month begin
    TimeArray::iterator lowIter = std::lower_bound(mData.begin(), mData.end(), this_month,
                                                   [] (const TimeRecord& lhs, const date& rhs)
    {
        return date::fromTimestamp(lhs.endTime(), date::To_LocalTime) < rhs;
    });

    while (lowIter != mData.end())
    {
        // See if start of current time period is later than current month
        if (date::fromTimestamp(lowIter->startTime(), date::To_LocalTime).mMonth > this_month.mMonth)
            break; // quit the loop

        if (date::fromTimestamp(lowIter->endTime(), date::To_LocalTime).mMonth >= this_month.mMonth)
        {
            // GMT time!
            time_t month_begin = this_month.toTimestamp();
            time_t month_end = month_begin + date::daysInMonth(this_month.mYear, this_month.mMonth) * 86400 - 1;

            int64_t secondsTo = lowIter->startTime() - month_begin;
            if (secondsTo > 0)
                month_begin = lowIter->startTime();

            int64_t secondsFrom = lowIter->endTime() - month_end;
            if (secondsFrom < 0)
                month_end = lowIter->endTime();

            int64_t secondsSpent = month_end - month_begin;
            result += secondsSpent;
        }

        lowIter++;
    }

    return result;
}

int TimeLine::getSum(const date& start, const date& finish)
{
    int result = 0;

    TimeArray::iterator lowest = std::lower_bound(mData.begin(), mData.end(), start,
                                                  [] (const TimeRecord& tr, const date& d)
    {
        return date::fromTimestamp(tr.endTime(), date::To_LocalTime) < d;
    });

    for (;lowest != mData.end(); lowest++)
    {
        TimeRecord& tr = *lowest;
        if (date::fromTimestamp(tr.startTime(), date::To_LocalTime) > finish)
            break;

        time_t day_begin = start.toTimestamp();
        time_t day_end = finish.toTimestamp() + 86400 - 1;

        if (day_begin - tr.startTime() < 0)
            day_begin = tr.startTime(); // Time record begin is later than begin of the interval

        if (day_end - tr.endTime() > 0)
            day_end = tr.endTime();

        result += day_end - day_begin;
    }
    return result;
}

bool TimeLine::duplicateDetected() const
{
    return mActiveTimeRecord == nullptr;
}

void TimeLine::putDebugRecord()
{
    time_t current = ::time(nullptr);
    time_t end = current + 600;

    TimeRecord* r = makeNewRecord(current, end);
    r->save();
}

// -------------------- Task --------------------------
static Task::ModelId ModelIdGenerator = 0;

Task::Task()
    :mId(0), mTitleModified(false), mHtmlModified(false), mHtmlLoaded(false), mModelId(++ModelIdGenerator),
      mParentId(-1), mIndex(-1), mChecked(false), mIndexModified(false),
      mParentModified(false), mReportedTime(0), mChildrenReportedTime(0), mAttachmentCount(0),
      mDocument(nullptr), mCursorPosition(0)
{
}

Task::~Task()
{}

Task::Id Task::id() const
{
    return mId;
}

Task& Task::setId(Id id)
{
    mId = id;
    return *this;
}

Task::Id Task::parentId() const
{
    return mParentId;
}

Task& Task::setParentId(Id id)
{
    if (mParentId != id)
    {
        mParentModified = true;
        mParentId = id;
    }
    return *this;
}

WorldId Task::worldId() const
{
    return mWorldId;
}

Task& Task::setWorldId(const WorldId& id)
{
    mWorldId = id;
    return *this;
}

Task::ModelId Task::modelId() const
{
    return mModelId;
}

Task& Task::setModelId(ModelId id)
{
    mModelId = id;
    return *this;
}

int Task::index() const
{
    return mIndex;
}

Task& Task::setIndex(int index, bool modified)
{
    if (index != mIndex)
    {
        if (modified)
            mIndexModified = true;
        mIndex = index;
    }
    return *this;
}

QString Task::html() const
{
    return mHtml;
}

Task& Task::setHtml(const QString &html)
{
    if (mHtml != html)
    {
        mHtml = html;
        mHtmlModified = true;
    }
    return *this;
}

QString Task::title() const
{
    return mTitle;
}

Task& Task::setTitle(const QString &title, bool modified)
{
    if (mTitle != title)
    {
        mTitle = title;
        if (modified)
            mTitleModified = true;
    }
    return *this;
}

void Task::save()
{
    Storage::instance().saveTask(*this);
    mIndexModified = mTitleModified = mHtmlModified = false;
}

void Task::saveAnyway()
{
    Storage::instance().saveTask(*this, Storage::Save_Forced);
    mIndexModified = mTitleModified = mHtmlModified = false;
}

QString Task::path() const
{
    QString result = this->title();
    if (parent())
        result.insert(0, parent()->path() + " / ");

    return result;
}

PTask Task::parent() const
{
    return mParent;
}

Task& Task::setParent(PTask task, bool modified)
{
    if (mParent != task)
    {
        mParent = task;
        if (task)
            mParentId = task->id();
        else
            mParentId = -1;
        if (modified)
            mTitleModified = true; // To force save()
    }
    return *this;
}

TaskArray& Task::children()
{
    return mChildren;
}

void Task::loadContent()
{
    Storage::instance().loadTaskContent(*this);
}

bool Task::isContentLoaded() const
{
    return mHtmlLoaded;
}

void Task::unloadContent(bool includeTimeline)
{
    mHtml.clear();
    mHtmlModified = mHtmlLoaded = false;
    if (includeTimeline)
        mTimeLine.clear();
}

PTimeLine Task::timeline()
{
    return mTimeLine;
}

int Task::getAttachmentCount()
{
    return mAttachmentCount;
}

Task& Task::setAttachmentCount(int count)
{
    mAttachmentCount = count;
    return *this;
}

int Task::preloadAttachmentCount()
{
    mAttachmentCount = Storage::instance().findAttachmentCountOnTask(*this);
    return mAttachmentCount;
}

bool Task::isChecked() const
{
    return mChecked;
}

Task& Task::setChecked(bool checked)
{
    mChecked = checked;
    return *this;
}

int Task::getReportedTime() const
{
    return mReportedTime;
}

Task& Task::setReportedTime(int t)
{
    mReportedTime = t;
    return *this;
}

int Task::getChildrenReportedTime() const
{
    return mChildrenReportedTime;
}

Task& Task::setChildrenReportedTime(int t)
{
    mChildrenReportedTime = t;
    return *this;
}

int Task::flags() const
{
    return mFlags;
}

Task& Task::setFlags(int value)
{
    mFlags = value;
    saveAnyway();

    return *this;
}

int Task::cursorPosition() const
{
    return mCursorPosition;
}

Task& Task::setCursorPosition(int position)
{
    mCursorPosition = position;
    return *this;
}

// ----------- Attachment -------------
Attachment::Attachment()
    :mId(0), mTaskId(0)
{

}

Attachment::~Attachment()
{

}

Task::Id Attachment::id()
{
    return mId;
}

Attachment& Attachment::setId(Task::Id id)
{
    mId = id;
    return *this;
}

Task::Id Attachment::taskId()
{
    return mTaskId;
}

Attachment& Attachment::setTaskId(Task::Id id)
{
    mTaskId = id;
    return *this;
}

WorldId Attachment::worldId() const
{
    return mWorldId;
}

Attachment& Attachment::setWorldId(const WorldId& id)
{
    mWorldId = id;
    return *this;
}

int Attachment::index()
{
    return mIndex;
}

Attachment& Attachment::setIndex(int index)
{
    //TODO: introduce mIndexModified field and corresponding login in save()
    mIndex = index;
    return *this;
}

QByteArray Attachment::loadContent()
{
    return Storage::instance().loadContent(*this);
}

Attachment& Attachment::saveContent(const QByteArray& content)
{
    Storage::instance().saveContent(*this, content);
    return *this;
}

Attachment& Attachment::setFilename(const QString& filename)
{
    mFilename = filename;
    return *this;
}

QString Attachment::filename()
{
    return mFilename;
}

Attachment& Attachment::saveMetadata()
{
    auto id = Storage::instance().saveMetadata(*this);
    if (id != 0)
        mId = id;

    return *this;
}

Attachment& Attachment::loadMetadata()
{
    return *this;
}
