// Copyright (c) 2017 Vivaldi Technologies AS. All rights reserved
//
// Based on code that is:
//
// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CALENDAR_CALENDAR_BACKEND_H_
#define CALENDAR_CALENDAR_BACKEND_H_

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "base/cancelable_callback.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/observer_list.h"
#include "base/supports_user_data.h"
#include "base/task/cancelable_task_tracker.h"
#include "base/task/single_thread_task_runner.h"
#include "calendar/calendar_backend_notifier.h"
#include "calendar/calendar_database.h"
#include "calendar/event_type.h"
#include "calendar/notification_type.h"
#include "calendar/recurrence_exception_type.h"
#include "sql/init_status.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace calendar {

class CalendarDatabase;
struct CalendarDatabaseParams;
class CalendarBackendHelper;
class EventDatabase;

// Internal calendar implementation which does most of the work of the calendar
// system. This runs on a custom created db thread (to not block the browser
// when we do expensive operations) and is NOT threadsafe, so it must only be
// called from message handlers on the background thread.
//
// Most functions here are just the implementations of the corresponding
// functions in the calendar service. These functions are not documented
// here, see the calendar service for behavior.
class CalendarBackend
    : public base::RefCountedThreadSafe<calendar::CalendarBackend>,
      public CalendarBackendNotifier {
 public:
  // Interface implemented by the owner of the CalendarBackend object. Normally,
  // the calendar service implements this to send stuff back to the main thread.
  // The unit tests can provide a different implementation if they don't have
  // a calendar service object.
  class CalendarDelegate {
   public:
    virtual ~CalendarDelegate() {}

    virtual void NotifyEventCreated(const EventResult& event) = 0;
    virtual void NotifyNotificationChanged(const NotificationRow& row) = 0;
    virtual void NotifyCalendarChanged() = 0;

    // Invoked when the backend has finished loading the db.
    virtual void DBLoaded() = 0;
  };

  explicit CalendarBackend(CalendarDelegate* delegate);

  // This constructor is fast and does no I/O, so can be called at any time.
  CalendarBackend(CalendarDelegate* delegate,
                  scoped_refptr<base::SequencedTaskRunner> task_runner);

  // Must be called after creation but before any objects are created. If this
  // fails, all other functions will fail as well. (Since this runs on another
  // thread, we don't bother returning failure.)
  //
  // |force_fail| can be set during unittests to unconditionally fail to init.
  void Init(bool force_fail,
            const CalendarDatabaseParams& calendar_database_params);

  // Notification that the calendar system is shutting down. This will break
  // the refs owned by the delegate and any pending transaction so it will
  // actually be deleted.
  void Closing();

  void CancelScheduledCommit();

  void Commit();

  // Creates an Event
  void CreateCalendarEvent(EventRow ev,
                           bool notify,
                           std::shared_ptr<EventResultCB> result);

  // Creates multiple events
  void CreateCalendarEvents(std::vector<calendar::EventRow> events,
                            std::shared_ptr<CreateEventsResult> result);

  void GetAllEvents(std::shared_ptr<EventQueryResults> results);

  // Updates an event
  void UpdateEvent(EventID event_id,
                   const EventRow& event,
                   std::shared_ptr<EventResultCB> result);
  void DeleteEvent(EventID event_id, std::shared_ptr<DeleteEventResult> result);

  void CreateRecurrenceException(RecurrenceExceptionRow row,
                                 std::shared_ptr<EventResultCB> result);
  void UpdateRecurrenceException(RecurrenceExceptionID recurrence_id,
                                 const RecurrenceExceptionRow& recurrence,
                                 std::shared_ptr<EventResultCB> result);
  void DeleteEventRecurrenceException(RecurrenceExceptionID exception_id,
                                      std::shared_ptr<EventResultCB> result);

  // Creates an Calendar
  void CreateCalendar(CalendarRow ev,
                      std::shared_ptr<CreateCalendarResult> result);
  void GetAllCalendars(std::shared_ptr<CalendarQueryResults> results);
  void UpdateCalendar(CalendarID calendar_id,
                      const Calendar& calendar,
                      std::shared_ptr<UpdateCalendarResult> result);
  void DeleteCalendar(CalendarID calendar_id,
                      std::shared_ptr<DeleteCalendarResult> result);

  void GetAllEventTypes(std::shared_ptr<EventTypeRows> results);
  void CreateEventType(EventTypeRow event_type_row,
                       std::shared_ptr<CreateEventTypeResult> result);
  void UpdateEventType(EventTypeID event_id,
                       const EventType& event_type,
                       std::shared_ptr<UpdateEventTypeResult> result);
  void DeleteEventType(EventTypeID event_type_id,
                       std::shared_ptr<DeleteEventTypeResult> result);

  void GetAllNotifications(std::shared_ptr<GetAllNotificationResult> results);

  void CreateNotification(calendar::NotificationRow row,
                          std::shared_ptr<NotificationResult> result);

  void UpdateNotification(calendar::UpdateNotificationRow row,
                          std::shared_ptr<NotificationResult> result);

  void DeleteNotification(NotificationID notification_id,
                          std::shared_ptr<DeleteNotificationResult> result);

  void CreateInvite(calendar::InviteRow row,
                    std::shared_ptr<InviteResult> result);

  void DeleteInvite(InviteID invite_id,
                    std::shared_ptr<DeleteInviteResult> result);

  void UpdateInvite(calendar::UpdateInviteRow row,
                    std::shared_ptr<InviteResult> result);

  void CreateAccount(AccountRow account_row,
                     std::shared_ptr<CreateAccountResult> result);

  void DeleteAccount(calendar::AccountID account_id,
                     std::shared_ptr<DeleteAccountResult> result);

  void UpdateAccount(calendar::AccountRow account_row,
                     std::shared_ptr<calendar::UpdateAccountResult> result);

  void GetAllAccounts(std::shared_ptr<AccountRows> results);

  void GetAllEventTemplates(std::shared_ptr<EventQueryResults> results);

  EventResult FillEvent(EventID id);

  void NotifyEventCreated(const EventResult& event) override;
  void NotifyNotificationChanged(const NotificationRow& row) override;
  void NotifyCalendarChanged() override;

 protected:
  ~CalendarBackend() override;
  CalendarBackend(const CalendarBackend&) = delete;
  CalendarBackend& operator=(const CalendarBackend&) = delete;

 private:
  friend class base::RefCountedThreadSafe<CalendarBackend>;

  // Does the work of Init.
  void InitImpl(const CalendarDatabaseParams& calendar_database_params);
  void GetEvents(EventRows rows, std::shared_ptr<EventQueryResults> results);
  // Closes all databases managed by CalendarBackend. Commits any pending
  // transactions.
  void CloseAllDatabases();

  // Querying ------------------------------------------------------------------

  // Directory where database files will be stored, empty until Init is called.
  base::FilePath calendar_dir_;

  // Delegate. See the class definition above for more information. This will
  // be null before Init is called and after Cleanup, but is guaranteed
  // non-null in between.
  std::unique_ptr<CalendarDelegate> delegate_;

  // A commit has been scheduled to occur sometime in the future. We can check
  // !IsCancelled() to see if there is a commit scheduled in the future (note
  // that CancelableClosure starts cancelled with the default constructor), and
  // we can use Cancel() to cancel the scheduled commit. There can be only one
  // scheduled commit at a time (see ScheduleCommit).
  base::CancelableOnceClosure scheduled_commit_;

  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  // The calendar database. Either may be null if the database could
  // not be opened, all users must first check for null and return immediately
  // if it is.
  std::unique_ptr<calendar::CalendarDatabase> db_;
};

}  // namespace calendar

#endif  // CALENDAR_CALENDAR_BACKEND_H_
