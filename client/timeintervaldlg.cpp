#include "timeintervaldlg.h"
#include "ui_timeintervaldlg.h"
#include "timetreemodel.h"
#include <QtGui/QCloseEvent>
#include <assert.h>
#include <QPushButton>
#include <QLocale>

TimeIntervalDlg::TimeIntervalDlg(QWidget *parent, TimeTreeModel* model, PTimeLine timeline, Type type, const TimeRecord& interval) :
  QDialog(parent, Qt::Sheet),
  ui(new Ui::TimeIntervalDlg),
  mTimeline(timeline), mType(type), mInterval(interval), mModel(model)
{
  ui->setupUi(this);
  connect(ui->mStartTimeEdit, SIGNAL(dateTimeChanged(QDateTime)), this, SLOT(dataChanged(QDateTime)));
  connect(ui->mFinishTimeEdit, SIGNAL(dateTimeChanged(QDateTime)), this, SLOT(dataChanged(QDateTime)));
  QLocale currentLocale;
  ui->mStartTimeEdit->setDisplayFormat(currentLocale.dateTimeFormat(QLocale::ShortFormat));
  ui->mFinishTimeEdit->setDisplayFormat(currentLocale.dateTimeFormat(QLocale::ShortFormat));
}

TimeIntervalDlg::~TimeIntervalDlg()
{
  delete ui;
}

void TimeIntervalDlg::setStartDate(const QDateTime& t)
{
  ui->mStartTimeEdit->setDateTime(t);
}

QDateTime TimeIntervalDlg::startDate()
{
  return ui->mStartTimeEdit->dateTime();
}

void TimeIntervalDlg::setFinishDate(const QDateTime& t)
{
  ui->mFinishTimeEdit->setDateTime(t);
}

QDateTime TimeIntervalDlg::finishDate()
{
  return ui->mFinishTimeEdit->dateTime();
}

void TimeIntervalDlg::accept()
{
  // Insert interval
  mModel->insertInterval(TimeRecord(startDate().toUTC().toSecsSinceEpoch(),
                                    finishDate().toUTC().toSecsSinceEpoch(),
                                    mTimeline->taskId()));
  emit accepted();
  close();
}

void TimeIntervalDlg::reject()
{
  // Bring back deleted interval
  if (mType == Type::Change)
    mTimeline->insertInterval(mInterval);
  close();
}

void TimeIntervalDlg::closeEvent(QCloseEvent *event)
{
  event->accept();
}

bool TimeIntervalDlg::validate()
{
  assert(mTimeline);
  return !mTimeline->hasIntersection(TimeRecord(startDate().toUTC().toSecsSinceEpoch(),
                                                finishDate().toUTC().toSecsSinceEpoch(),
                                                mTimeline->taskId()));
}

void TimeIntervalDlg::dataChanged(const QDateTime& changed)
{
  ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(validate());
}

