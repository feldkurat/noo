#include "storage.h"
#include <QVariant>
#include <assert.h>
#include <encryption.h>
#include <iostream>

#ifdef TARGET_WIN
# include <WinSock2.h>
#endif
enum class DbProperty
{
    Version = 1,
    InternalCipher = 2
};

#define CURRENT_DBVERSION "0"

// Function to make encryption. Input is raw content blob, output is blob with IV, original length and encrypted data.
/*static void EncryptBlob(const QByteArray& input, QByteArray& output)
{
    TwofishCipher cipher;

    // Generate 16 bytes IV
    QByteArray iv(16, 0);
    IV::Generate(iv);
    cipher.setIV(iv);

    // Make padding
    int padded = (input.length() % cipher.blocksize()) ? (cipher.blocksize() - input.length() % cipher.blocksize()) : 0;

    // Prepare output buffer
    output.resize(iv.length() + 4 + input.length() + padded);

    // Copy IV
    memcpy(output.data(), iv.data(), iv.length());

    // Copy original length in network byte order
    uint32_t lengthNbo = htonl(input.length());
    memcpy(output.data() + iv.length(), &lengthNbo, 4);

    // Encrypt
    cipher.encrypt(input, 0, output, iv.length() + 4);
}*/

/*static void DecryptBlob(const QByteArray& input, QByteArray& output)
{
    TwofishCipher cipher;
    assert(input.length() >= cipher.blocksize() + 4);

    // Read IV
    QByteArray iv(input.data(), cipher.blocksize());

    // Read original length
    uint32_t lengthNbo;
    memcpy(&lengthNbo, input.data() + iv.length(), 4);

    // Prepare output buffer
    output.resize(input.length() - iv.length() - 4);

    // Set IV
    cipher.setIV(iv);

    // Decrypt data
    cipher.decrypt(input, iv.length() + 4, output, 0);

    // Truncate to original size
    output.truncate(ntohl(lengthNbo));
}*/


Storage::Storage()
{
}

Storage::~Storage()
{
    close();
}

QString Storage::path()
{
    return mPath;
}

void Storage::setPath(const QString &path)
{
    mPath = path;
}

QString Storage::key()
{
    return mKey;
}

void Storage::setKey(const QString &key)
{
    mKey = key;
}

SQLite::Database& Storage::database()
{
    return *mDatabase;
}

bool Storage::create()
{
    if (mPath.isEmpty())
        return false;

    assert(!mPath.isEmpty());
    try
    {
        mDatabase = QSharedPointer<SQLite::Database>(new SQLite::Database(mPath.toStdString().c_str(), SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE ));
    }
    catch(std::exception& e)
    {
        return false;
    }

#ifdef USE_ENCRYPTED_DB
    try
    {
        std::string keyQuery = "pragma key='" + mKey.toStdString() + "'";
        mDatabase->exec(keyQuery.c_str());
        mDatabase->exec("pragma locking_mode=EXCLUSIVE");
        mDatabase->exec("pragma journal_mode=MEMORY");
        mDatabase->exec("pragma temp_store=MEMORY");
    }
    catch(std::exception& e)
    {
        std::cout << e.what() << std::endl;
        return false;
    }
#endif

    try
    {
        // Synctime & timestamp are always milliseconds from the start of UNIX epoch.

        // Timeline
        mDatabase->exec("CREATE TABLE timeline ("
                        "id         INTEGER PRIMARY KEY, "
                        "removed    INTEGER, "
                        "taskid     INTEGER, "
                        "starttime  TEXT, "
                        "endtime    TEXT, "
                        "timestamp  INTEGER)");
        mDatabase->exec("CREATE INDEX timeline_taskid_index ON timeline(taskid ASC)");

        // Tasks tree
        mDatabase->exec("CREATE TABLE task ("
                        "type       INTEGER, "
                        "removed    INTEGER, "
                        "id         INTEGER PRIMARY KEY, "
                        "parentid   INTEGER, "
                        "orderid    INTEGER, "
                        "title      TEXT, "
                        "html       TEXT, "
                        "flags      INTEGER, "
                        "timestamp  INTEGER)");

        mDatabase->exec("CREATE INDEX task_parentid_index ON task(parentid ASC)");

        // Tasks history
        mDatabase->exec("CREATE TABLE history_task ("
                        "removed_old    INTEGER, "
                        "removed_new    INTEGER, "
                        "timestamp_old  INTEGER, "
                        "timestamp_new  INTEGER, "
                        "id             INTEGER PRIMARY KEY, "
                        "parent_id_old  INTEGER, "
                        "parent_id_new  INTEGER, "
                        "order_id_old   INTEGER, "
                        "order_id_new   INTEGER, "
                        "diff_title     TEXT, "
                        "diff_html      TEXT, "
                        "flags_old      INTEGER, "
                        "flags_new      INTEGER)");

        // Attachments
        mDatabase->exec("CREATE TABLE file ("
                        "id         INTEGER PRIMARY KEY, "
                        "removed    INTEGER, "
                        "taskid     INTEGER, "
                        "filename   TEXT, "
                        "content    BLOB, "
                        "orderid    INTEGER, "
                        "timestamp  INTEGER)");

        mDatabase->exec("CREATE TABLE history_file ("
                        "id             INTEGER PRIMARY KEY, "
                        "removed_old    INTEGER, "
                        "removed_new    INTEGER, "
                        "taskid_old     INTEGER, "
                        "taskid_new     INTEGER, "
                        "filename_old   TEXT, "
                        "filename_new   TEXT,"
                        "content        BLOB, "
                        "order_id_old   INTEGER, "
                        "order_id_new   INTEGER, "
                        "timestamp_old  INTEGER, "
                        "timestamp_new  INTEGER)");

        mDatabase->exec("CREATE INDEX file_taskid_index ON file(taskid ASC)");

        // Sync status
        mDatabase->exec("CREATE TABLE syncs (timestamp INT, status TEXT)");


    }
    catch(std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return false;
    }

    return true;
}

bool Storage::open()
{
    assert(!mPath.isEmpty());

    try
    {
        mDatabase = QSharedPointer<SQLite::Database>(new SQLite::Database(mPath.toStdString().c_str(), SQLITE_OPEN_READWRITE));
        mDatabase->exec("pragma key='" + mKey.toStdString() + "'");
        SQLite::Statement q(*mDatabase, "select count(*) from sqlite_master");
        if (!q.executeStep())
            return false;
        mDatabase->exec("pragma locking_mode=EXCLUSIVE");
        mDatabase->exec("pragma journal_mode=MEMORY");
        mDatabase->exec("pragma temp_store=MEMORY");

    }
    catch(std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return false;
    }

    loadTaskTree();
    return true;
}

void Storage::close()
{
    if (mDatabase)
    {
        mDatabase.clear();
    }
}

bool Storage::hasTable(const QString& tablename)
{
    SQLite::Statement tableQuery(*mDatabase, "SELECT name FROM sqlite_master WHERE type='table' AND name=:tablename");
    tableQuery.bind(":tablename", tablename.toStdString());
    return tableQuery.executeStep();
}

bool Storage::upgrade()
{
    int currentVersion = 0;

    // Check if properties table is here
    if (!mDatabase)
        return false;

    if (!hasTable("properties"))
    {
        // If not - create it and consider database version 0
        SQLite::Statement createQuery(*mDatabase, "create table properties(type INTEGER, value TEXT)");
        if (!createQuery.exec())
            return false;

        // Insert version 0
        SQLite::Statement insertVersionQuery(*mDatabase, "insert into properties(type, value) values(:proptype, :proptext)");
        insertVersionQuery.bind(":proptype", (int)DbProperty::Version);
        insertVersionQuery.bind(":proptext", "0");
    }
    else
    {
        // If yes - get database version from 'dbversion' key
        SQLite::Statement versionQuery(*mDatabase, "select value from properties where type=:proptype");
        versionQuery.bind(":proptype", (int)DbProperty::Version);
        if (!versionQuery.executeStep())
            return false;

        QString dbVersionText = versionQuery.getColumn(0).getText("");
        if (dbVersionText.isEmpty())
            return false;

        bool ok = false;
        currentVersion = dbVersionText.toInt(&ok);
        if (!ok)
            return false;
    }

    // Depending on obtained version upgrade database
    switch (currentVersion)
    {
    case 0:
        if (!upgradeFromVersion0())
            return false;
        break;
    }

    // Update version number after upgrade
    SQLite::Statement updateVersion(*mDatabase, "update table properties set value=:proptext where type=:proptype");
    updateVersion.bind(":proptext", CURRENT_DBVERSION);
    updateVersion.bind(":proptype", (int)DbProperty::Version);
    updateVersion.exec();

    return true;
}

bool Storage::upgradeFromVersion0()
{
    /*
  // Upgrade tasks table
  SQLite::Statement addEncryptedTitle(*mDatabase, "alter table task add column title_encrypted BLOB");
  if (!addEncryptedTitle.exec())
    return false;

  SQLite::Statement addEncryptedContent(*mDatabase, "alter table task add column content_encrypted BLOB");
  if (!addEncryptedContent.exec())
    return false;

  SQLite::Statement addEncryptedAttachments(*mDatabase, "alter table attachment add column content_encrypted BLOB");
  if (!addEncryptedAttachments.exec())
    return false;

  // Database scheme is upgraded here, convert the data

  SQLite::Statement tasks(*mDatabase, "select id, title, html from task");
  while (tasks.executeStep())
  {
    // TODO
  }
  // Iterate all notes and attachments and make them Twofish encrypted
  mTaskToUpgrade.resize(topOfTaskTree().size());
  std::copy(topOfTaskTree().begin(), topOfTaskTree().end(), mTaskToUpgrade);
  for (PTask task: mTaskToUpgrade)
  {
    encryptTaskContent(task);
    //encryptTaskAttachment(task);

    // Add children to list
    int tl = mTaskToUpgrade.length();
    mTaskToUpgrade.resize(mTaskToUpgrade.size() + task->children().size());
    std::copy(task->children().begin(), task->children().end(), mTaskToUpgrade.begin() + tl);
  }
  */
    return true;
}

bool Storage::encryptTask(PTask /*task*/)
{
    return false;
}

bool Storage::encryptTaskContent(PTask /*task*/)
{
    return false;
}

bool Storage::encryptTaskAttachment(PAttachment /*attachment*/)
{
    return false;
}


PTask Storage::createTask(int index)
{
    if (!mDatabase)
        return PTask();

    SQLite::Statement insertQuery(*mDatabase, "insert into task(id, title, html, parentid, orderid, type, flags) values(NULL, :title, :html, NULL, :orderid, 0, 0)");
    insertQuery.bind(":title", "");
    insertQuery.bind(":html", "");
    insertQuery.bind(":orderid", index);
    if (insertQuery.exec())
    {
        PTask result(new Task());
        result->setId(database().getLastInsertRowid())
               .setIndex(index);

        mTaskModelIdMap[result->modelId()] = result;
        mTaskIdMap[result->id()] = result;
        if (index > mTopTasks.size())
            mTopTasks.push_back(result);
        else
        {
            mTopTasks.insert(mTopTasks.begin() + index, result);
            // Assign new indexes for top tasks
            for (int i=0; i<mTopTasks.size(); i++)
            {
                mTopTasks[i]->setIndex(i)
                             .save();
            }
        }
        return result;
    }
    else
        return PTask();
}

PTask Storage::createTask(const PTask& parent, int index)
{
    if (!parent)
        return createTask(index);
    if (!mDatabase)
        return PTask();

    SQLite::Statement insertQuery(database(), "insert into task(id, title, html, parentid, orderid, type, flags) values(NULL, :title, :html, :parentid, :orderid, 0, 0)");
    insertQuery.bind(":title", "");
    insertQuery.bind(":html", "");
    insertQuery.bind(":parentid", (sqlite3_int64)parent->id());
    insertQuery.bind(":orderid", index);
    if (insertQuery.exec())
    {
        PTask result(new Task());
        result->setId(database().getLastInsertRowid());
        result->setIndex(index);
        result->setParent(parent);
        if (index > parent->children().size())
            parent->children().push_back(result);
        else
        {
            parent->children().insert(parent->children().begin() + index, result);
            for (int i=0; i<parent->children().size(); i++)
            {
                parent->children()[i]->setIndex(i)
                                      .save();
            }
        }

        mTaskModelIdMap[result->modelId()] = result;
        mTaskIdMap[result->id()] = result;
        return result;
    }
    return PTask();
}
/*
PTask Storage::loadTask(Task::Id id, PTask parent)
{
  PTask result(new Task());
  result->setId(id);
  result->load();
  result->setParent(parent);
  mTaskIdMap[result->modelId()] = result;

  return result;
}
*/

void Storage::saveTask(PTask task, Depth depth)
{
    if (depth == depthSingleTask)
        saveSingleTask(task);
    else
    {
        saveSingleTask(task);
        TaskArray& children = task->children();
        foreach(PTask child, children)
            saveTask(child, depthRecursive);
    }
}

bool Storage::moveTask(PTask task, PTask newParent, int indexToInsert)
{
    bool result = false;
    task->setParent(newParent);
    if (newParent)
    {
        SQLite::Statement updateParent(database(), "update task set parentid=:parentid where id=:id");
        updateParent.bind(":parentid", (sqlite3_int64)newParent->id());
        updateParent.bind(":id", (sqlite3_int64)task->id());
        result = updateParent.exec();

        // Insert into children list
        TaskArray& children = newParent->children();
        if (indexToInsert >= children.size())
            children.push_back(task);
        else
            children.insert(indexToInsert, task);
        for (int i=0; i<children.size(); i++)
        {
            children[i]->setIndex(i);
            children[i]->save();
        }
    }
    else
    {
        SQLite::Statement updateToRoot(database(), "update task set parentid = NULL where id=:id");
        updateToRoot.bind(":id", (sqlite3_int64)task->id());
        result = updateToRoot.exec();
        if (result)
            task->setParentId(0);

        // Insert into root list
        if (topOfTaskTree().size() > indexToInsert)
            topOfTaskTree().insert(indexToInsert, task);
        else
            topOfTaskTree().push_back(task);

        for (int i=0; i<topOfTaskTree().size(); i++)
        {
            topOfTaskTree()[i]->setIndex(i)
                               .save();
        }
    }

    if (result)
    {
        task->save();
    }

    return result;
}

bool Storage::isOpened()
{
    return mDatabase != nullptr;
}

void Storage::save()
{
    foreach (PTask task, mTopTasks)
        saveTask(task, depthRecursive);

    database().exec("delete from task where removed = 1");
    database().exec("delete from timeline where removed = 1");
    database().exec("delete from file where removed = 1");
    //database().exec("delete from change where removed = 1");
}

void Storage::saveSingleTask(PTask task)
{
    task->save();
}

void Storage::loadTaskRecord(Task& t, SQLite::Statement& q)
{
    t.mId               = q.getColumn(0).getInt64();
    t.mTitle            = q.getColumn(1).getText();
    t.mIndex            = q.getColumn(2).getInt();
    t.mFlags            = q.getColumn(3).getInt();
    t.mAttachmentCount  = q.getColumn(4).getInt();
}

void Storage::loadTaskTree()
{
    mTopTasks.clear();

    SQLite::Statement q(database(), "select id, title, orderid, flags, (select count(*) from file where (file.taskid = task.id) and ((file.removed is null) or (file.removed = 0))) from task where (parentid is null) and ((removed != 1) or (removed is null)) order by orderid");
    int currentIndex = 0;
    while(q.executeStep())
    {
        PTask t(new Task());
        loadTaskRecord(*t, q);
        t->setIndex(currentIndex++);
        mTaskModelIdMap[t->modelId()] = t;
        mTaskIdMap[t->id()] = t;
        mTopTasks.push_back(t);
    }

    foreach (PTask task, mTopTasks)
        loadTaskChildren(task);
}

void Storage::loadTaskChildren(PTask task)
{
    SQLite::Statement q(database(), "select id, title, orderid, flags, (select count(*) from file where (file.taskid = task.id) and ((file.removed is null) or (file.removed = 0))) from task where (task.parentid = :parentid) and ((task.removed != 1) or (task.removed is null)) order by task.orderid");
    q.bind(":parentid", (sqlite3_int64)task->id());
    int currentIndex = 0;
    while (q.executeStep())
    {
        PTask t(new Task());
        loadTaskRecord(*t, q);
        t->setIndex(currentIndex++)
          .setParent(task, false);

        loadTaskChildren(t);
        mTaskModelIdMap[t->modelId()] = t;
        mTaskIdMap[t->id()] = t;
        task->children().push_back(t);
    }
}

PTask Storage::findTaskByModelId(Task::ModelId id)
{
    auto taskIter = mTaskModelIdMap.find(id);
    if (taskIter != mTaskModelIdMap.end())
        return *taskIter;
    else
        return PTask();
}

PTask Storage::findTaskById(Task::Id id)
{
    auto taskIter = mTaskIdMap.find(id);
    if (taskIter != mTaskIdMap.end())
        return *taskIter;
    else
        return PTask();
}

int Storage::findTaskIndexInParent(PTask task)
{
    if (task->parent())
        return task->parent()->children().indexOf(task);
    else
        return topOfTaskTree().indexOf(task);
}

TaskArray& Storage::topOfTaskTree()
{
    return mTopTasks;
}

void Storage::loadAttachments(PTask task, AttachmentArray& output)
{
    SQLite::Statement q(database(), "select id, filename, orderid from file where (taskid = :taskid) and ((removed != 1) or (removed is null)) order by orderid");
    q.bind(":taskid", (sqlite3_int64)task->id());
    while (q.executeStep())
    {
        PAttachment att(new Attachment());
        att->setId(q.getColumn(0).getInt64())
            .setFilename(q.getColumn(1).getText())
            .setTaskId(task->id())
            .setIndex(q.getColumn(2).getInt());
        output.push_back(att);
    }
}

void Storage::deleteAttachment(PAttachment att)
{
    SQLite::Statement q(database(), "update file set removed=1 where id=:id");
    q.bind(":id", (sqlite3_int64)att->id());
    q.exec();
}

void Storage::undeleteAttachment(PAttachment att)
{
    SQLite::Statement q(database(), "update file set removed=0 where id=:id");
    q.bind(":id", (sqlite3_int64)att->id());
    q.exec();
}

Id Storage::saveMetadata(const Attachment &a)
{
    if (a.mId)
    {
        SQLite::Statement q(Storage::instance().database(), "update file set filename = :filename, orderid = :orderid where id = :id");
        q.bind(":filename", a.mFilename.toStdString().c_str());
        q.bind(":orderid", a.mIndex);
        q.bind(":id", (sqlite3_int64)a.mId);

        if (q.exec())
            return a.mId;
    }
    else
    {
        SQLite::Statement q(Storage::instance().database(), "insert into file (filename, taskid, orderid, removed) values(:filename, :taskid, :orderid, 0)");
        q.bind(":filename", a.mFilename.toStdString().c_str());
        q.bind(":taskid", (sqlite3_int64)a.mTaskId);
        q.bind(":orderid", a.mIndex);
        if (q.exec())
        {
            return Storage::instance().database().getLastInsertRowid();
        }
    }

    return Id(0);
}

void Storage::saveContent(const Attachment &a, const QByteArray& content)
{
    SQLite::Statement q(Storage::instance().database(), "update file set content = :content where id = :id");
    q.bind(":content", content.data(), content.size());
    q.bind(":id", (sqlite3_int64)a.mId);
    if (q.exec())
        ;
}

QByteArray Storage::loadContent(const Attachment &a)
{
    SQLite::Statement q(Storage::instance().database(), "select content from file where id = :id");
    q.bind(":id", (sqlite3_int64)a.mId);
    if (q.executeStep())
        return QByteArray((const char*)q.getColumn(0).getBlob(), q.getColumn(0).size());
    else
        return QByteArray();
}

Id Storage::saveTimeRecord(const TimeRecord &r)
{
    if (!r.id())
    {
        SQLite::Statement q(Storage::instance().database(), "insert into timeline(id, starttime, endtime, taskid, removed) values (NULL, :starttime, :endtime, :taskid, :removed)");

        q.bind(":starttime", helper::chrono::timeToStr(r.startTime()));
        q.bind(":endtime", helper::chrono::timeToStr(r.endTime()));
        q.bind(":taskid", static_cast<sqlite3_int64>(r.taskId()));
        q.bind(":removed", 0);
        if (q.exec())
            return Storage::instance().database().getLastInsertRowid();
    }
    else
    {
        SQLite::Statement q(Storage::instance().database(),
                            "update timeline set starttime = :starttime, endtime = :endtime, taskid = :taskid, removed = 0 where id = :id");
        q.bind(":starttime", helper::chrono::timeToStr(r.startTime()));
        q.bind(":endtime", helper::chrono::timeToStr(r.endTime()));
        q.bind(":taskid", static_cast<sqlite3_int64>(r.taskId()));
        q.bind(":id", static_cast<sqlite3_int64>(r.id()));
        if (q.exec())
            return r.id();
    }
    return (Id)0;
}

void Storage::deleteTimeRecord(const TimeRecord &r)
{
    SQLite::Statement q(Storage::instance().database(), "update timeline set removed = 1 where id = :id");
    q.bind(":id", static_cast<sqlite3_int64>(r.id()));
    q.exec();
}

void Storage::loadTimeLine(TimeLine& l)
{
    SQLite::Statement q(Storage::instance().database(), "select id, starttime, endtime from timeline where (taskid = :taskid) and ((removed is null) or (removed != 1)) order by id asc");
    q.bind(":taskid", static_cast<sqlite3_int64>(l.taskId()));
    while (q.executeStep())
    {
        time_t start = helper::chrono::strToTime(q.getColumn(1).getText());
        time_t stop = helper::chrono::strToTime(q.getColumn(2).getText());

        TimeRecord tr;
        tr.setId(q.getColumn(0).getInt64())
          .setStartTime(start)
          .setEndTime(stop)
          .setTaskId(l.taskId());
        l.data().push_back(tr);
    }

    // Sort time intervals
    l.sortData();

    // Find current total time length
    l.mTotalTime = l.findTotalTime();
}

void Storage::saveTimeLime(const TimeLine& l)
{

}

Storage& Storage::instance()
{
    static Storage _instance;
    return _instance;
}


void Storage::deleteTask(const PTask& task, DeleteOption option)
{
    if (option != DeleteOption_FromParent)
    {
        // Remove from hash
        removeTask(task);
    }

    TaskArray& siblings = task->parent() ? task->parent()->children() : mTopTasks;
    siblings.removeOne(task);

    // Remove from database
    if (option == DeleteOption_Total)
    {
        SQLite::Statement q(database(), "update task set removed = 1 where id = :id");
        q.bind(":id", (sqlite3_int64)task->id());
        q.exec();
    }

#if 0
    int taskIndex = -1;
    // Update indexes
    for (int i=0; i<siblings.size(); i++)
    {
        if (siblings[i] == task)
            taskIndex = i;

        Task& t = *siblings[i];
        if (t.index() > task->index())
        {
            t.setIndex(t.index() - 1);
            t.save();
        }
    }

    // Remove from tree
    if (taskIndex >= 0)
        siblings.remove(taskIndex);
#endif
}

void Storage::undeleteTask(const PTask& task)
{
    SQLite::Statement q(database(), "update task set removed = 0 where id = :id");
    q.bind(":id", (sqlite3_int64)task->id());
    q.exec();

    // Find place where to insert task
    TaskArray& siblings = task->parent() ? task->parent()->children() : mTopTasks;

    TaskArray::iterator taskIter = std::lower_bound(siblings.begin(), siblings.end(), task->index(), [] (const PTask& t, int index) { return t->index() < index;});
    if (taskIter != siblings.end())
        siblings.insert(taskIter, task);
    else
        siblings.append(task);

    mTaskModelIdMap[task->modelId()] = task;
    mTaskIdMap[task->id()] = task;
}

void Storage::removeTask(const PTask& task)
{
    auto taskModelIter = mTaskModelIdMap.find(task->modelId());
    if (taskModelIter != mTaskModelIdMap.end())
        mTaskModelIdMap.erase(taskModelIter);

    auto taskIter = mTaskIdMap.find(task->id());
    if (taskIter != mTaskIdMap.end())
        mTaskIdMap.erase(taskIter);
}

void Storage::loadTaskContent(Task& task)
{
    SQLite::Statement htmlQuery(Storage::instance().database(), "select html from task where id = :id");
    htmlQuery.bind(":id", (sqlite3_int64)task.mId);
    if (htmlQuery.executeStep())
    {
        task.mHtml = htmlQuery.getColumn(0).getText();
        task.mHtmlLoaded = true;
        task.mHtmlModified = false;
    }

    if (!task.mTimeLine)
    {
        task.mTimeLine = PTimeLine(new TimeLine());
        task.mTimeLine->setTaskId(task.mId);
        task.mTimeLine->load();
    }
}

void Storage::saveTask(const Task& task, SaveOptions options)
{
    const Task& t = task;

    if (!t.mTitleModified && !t.mHtmlModified && !t.mIndexModified && !t.mParentModified && options == Save_Automatic)
        return;

    const char* queryText = nullptr;
    // Minimize changes to DB
    if (t.mTitleModified && t.mHtmlModified)
        queryText = "update task set parentid = :parentid, flags = :flags, title = :title, html = :html, orderid = :orderid where id = :id";
    else
    if (t.mTitleModified)
        queryText = "update task set parentid = :parentid, flags = :flags, title = :title, orderid = :orderid where id = :id";
    else
    if (t.mHtmlModified)
        queryText = "update task set parentid = :parentid, flags = :flags, html = :html, orderid = :orderid where id = :id";
    else
        queryText = "update task set parentid = :parentid, flags = :flags, orderid = :orderid where id = :id";

    SQLite::Statement q(Storage::instance().database(), queryText);
    if (t.mParent)
        q.bind(":parentid", (sqlite3_int64)t.mParent->id());
    else
        q.bind(":parentid");

    q.bind(":flags", t.mFlags);

    if (t.mTitleModified)
        q.bind(":title", t.mTitle.toStdString());
    if (t.mHtmlModified)
        q.bind(":html", t.mHtml.toStdString());

    q.bind(":id", (sqlite3_int64)t.mId);
    q.bind(":orderid", t.mIndex);
    q.exec();
}

int Storage::findAttachmentCountOnTask(const Task &t)
{
    SQLite::Statement q(Storage::instance().database(), "select count(*) from file where (taskid = :taskid) and ((removed = 0) or (removed is null))");
    q.bind(":taskid", (sqlite3_int64)t.id());
    if (q.executeStep())
        return q.getColumn(0).getInt();
    else
        return 0;
}
