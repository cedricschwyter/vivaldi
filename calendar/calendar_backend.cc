// Copyright (c) 2017 Vivaldi Technologies AS. All rights reserved
//
// Based on code that is:
//
// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "calendar/calendar_backend.h"

#include <algorithm>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "base/compiler_specific.h"
#include "base/debug/dump_without_crashing.h"
#include "base/feature_list.h"
#include "base/files/file_enumerator.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/single_thread_task_runner.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "calendar/calendar_constants.h"
#include "calendar/calendar_database.h"
#include "calendar/calendar_database_params.h"
#include "calendar/event_type.h"
#include "calendar/notification_type.h"
#include "calendar/recurrence_exception_type.h"
#include "sql/error_delegate_util.h"

using base::Time;
using base::TimeDelta;
using base::TimeTicks;

namespace calendar {

// CalendarBackend -----------------------------------------------------------
CalendarBackend::CalendarBackend(
    CalendarDelegate* delegate,
    scoped_refptr<base::SequencedTaskRunner> task_runner)
    : delegate_(delegate) {}

CalendarBackend::CalendarBackend(CalendarDelegate* delegate) {}

CalendarBackend::~CalendarBackend() {}

void CalendarBackend::NotifyEventCreated(const EventResult& event) {
  if (delegate_)
    delegate_->NotifyEventCreated(event);
}

void CalendarBackend::NotifyNotificationChanged(const NotificationRow& row) {
  if (delegate_)
    delegate_->NotifyNotificationChanged(row);
}

void CalendarBackend::NotifyCalendarChanged() {
  if (delegate_)
    delegate_->NotifyCalendarChanged();
}

void CalendarBackend::Init(
    bool force_fail,
    const CalendarDatabaseParams& calendar_database_params) {
  // CalendarBackend is created on the UI thread by CalendarService, then the
  // CalendarBackend::Init() method is called on the DB thread. Create the
  // base::SupportsUserData on the DB thread since it is not thread-safe.

  if (!force_fail)
    InitImpl(calendar_database_params);
  delegate_->DBLoaded();
}

void CalendarBackend::Closing() {
  // TODO(arnar): Add
  //  CancelScheduledCommit();

  // Release our reference to the delegate, this reference will be keeping the
  // calendar service alive.
  delegate_.reset();
}

void CalendarBackend::InitImpl(
    const CalendarDatabaseParams& calendar_database_params) {
  DCHECK(!db_) << "Initializing CalendarBackend twice";
  // In the rare case where the db fails to initialize a dialog may get shown
  // the blocks the caller, yet allows other messages through. For this reason
  // we only set db_ to the created database if creation is successful. That
  // way other methods won't do anything as db_ is still null.

  // Compute the file names.
  calendar_dir_ = calendar_database_params.calendar_dir;
  base::FilePath calendar_name = calendar_dir_.Append(kCalendarFilename);

  // Calendar database.
  db_.reset(new CalendarDatabase());

  sql::InitStatus status = db_->Init(calendar_name);
  switch (status) {
    case sql::INIT_OK:
      break;
    case sql::INIT_FAILURE: {
      // A null db_ will cause all calls on this object to notice this error
      // and to not continue. If the error callback scheduled killing the
      // database, the task it posted has not executed yet. Try killing the
      // database now before we close it.
      [[fallthrough]];
    }  // Falls through.
    case sql::INIT_TOO_NEW: {
      LOG(ERROR) << "INIT_TOO_NEW";

      return;
    }
    default:
      NOTREACHED();
  }
}

void CalendarBackend::GetAllEvents(std::shared_ptr<EventQueryResults> results) {
  EventRows rows;
  db_->GetAllCalendarEvents(&rows);
  GetEvents(rows, results);
}

void CalendarBackend::GetAllEventTemplates(
    std::shared_ptr<EventQueryResults> results) {
  EventRows rows;
  db_->GetAllCalendarEventTemplates(&rows);
  GetEvents(rows, results);
}

void CalendarBackend::GetEvents(EventRows rows,
                                std::shared_ptr<EventQueryResults> results) {
  RecurrenceExceptionRows recurrence_exception_rows;
  db_->GetAllRecurrenceExceptions(&recurrence_exception_rows);

  // Now add them and the URL rows to the results.
  for (size_t i = 0; i < rows.size(); i++) {
    EventRow eventRow = rows[i];
    std::set<RecurrenceExceptionID> events_with_exceptions;
    for (auto& exception_row : recurrence_exception_rows) {
      events_with_exceptions.insert(exception_row.parent_event_id);
    }

    const bool has_exception = events_with_exceptions.find(eventRow.id) !=
                               events_with_exceptions.end();

    if (has_exception) {
      RecurrenceExceptionRows exception_rows;
      for (auto& exception : recurrence_exception_rows) {
        if (exception.parent_event_id == eventRow.id) {
          exception_rows.push_back(exception);
        }
      }
      eventRow.recurrence_exceptions = exception_rows;
    }

    NotificationRows notification_rows;
    db_->GetAllNotificationsForEvent(eventRow.id, &notification_rows);
    eventRow.notifications = notification_rows;

    InviteRows invite_rows;
    db_->GetInvitesForEvent(eventRow.id, &invite_rows);
    eventRow.invites = invite_rows;

    EventResult result(eventRow);
    results->AppendEventBySwapping(&result);
  }
}

void CalendarBackend::CreateCalendarEvents(
    std::vector<calendar::EventRow> events,
    std::shared_ptr<CreateEventsResult> result) {
  int success_counter = 0;
  int failed_counter = 0;

  size_t count = events.size();

  std::shared_ptr<EventResultCB> event_result =
      std::shared_ptr<EventResultCB>(new EventResultCB());

  for (size_t i = 0; i < count; i++) {
    EventRow ev = events[i];

    CreateCalendarEvent(ev, false, event_result);

    if (event_result->success) {
      success_counter++;
    } else {
      failed_counter++;
    }
  }

  result->number_success = success_counter;
  result->number_failed = failed_counter;
  NotifyCalendarChanged();
}

void CalendarBackend::CreateCalendarEvent(
    EventRow ev,
    bool notify,
    std::shared_ptr<EventResultCB> result) {
  if (!db_->DoesCalendarIdExist(ev.calendar_id)) {
    result->success = false;
    result->message = "Calendar does not exist.";
    return;
  }

  EventID id = db_->CreateCalendarEvent(ev);

  if (id) {
    ev.id = id;
    if (ev.event_exceptions.size() > 0) {
      for (const auto& exception : ev.event_exceptions) {
        RecurrenceExceptionRow row;
        row.exception_event_id = exception.exception_event_id;
        row.parent_event_id = id;
        row.exception_day = exception.exception_day;
        row.cancelled = exception.cancelled;
        db_->CreateRecurrenceException(row);
      }
    }

    if (ev.notifications_to_create.size() > 0) {
      for (const auto& notification : ev.notifications_to_create) {
        NotificationRow notification_row;
        notification_row.event_id = id;
        notification_row.name = notification.name;
        notification_row.when = notification.when;
        db_->CreateNotification(notification_row);
      }
    }

    if (ev.invites_to_create.size() > 0) {
      for (const auto& invite : ev.invites_to_create) {
        InviteRow invite_row;
        invite_row.event_id = id;
        invite_row.name = invite.name;
        invite_row.partstat = invite.partstat;
        invite_row.address = invite.address;
        db_->CreateInvite(invite_row);
      }
    }

    result->success = true;
    EventResult res = FillEvent(id);
    result->event = res;
    if (notify) {
      NotifyCalendarChanged();
    }
  } else {
    result->success = false;
  }
}

EventResult CalendarBackend::FillEvent(EventID id) {
  EventRow event_row;
  db_->GetRowForEvent(id, &event_row);
  NotificationRows notification_rows;
  db_->GetAllNotificationsForEvent(id, &notification_rows);
  event_row.notifications = notification_rows;

  InviteRows invite_rows;
  db_->GetInvitesForEvent(id, &invite_rows);
  event_row.invites = invite_rows;

  RecurrenceExceptionRows recurrence_exception_rows;
  db_->GetAllRecurrenceExceptions(&recurrence_exception_rows);
  std::set<RecurrenceExceptionID> events_with_exceptions;
  for (auto& exception_row : recurrence_exception_rows) {
    events_with_exceptions.insert(exception_row.parent_event_id);
  }

  const bool has_exception =
      events_with_exceptions.find(id) != events_with_exceptions.end();

  if (has_exception) {
    RecurrenceExceptionRows exception_rows;
    for (auto& exception : recurrence_exception_rows) {
      if (exception.parent_event_id == id) {
        exception_rows.push_back(exception);
      }
    }
    event_row.recurrence_exceptions = exception_rows;
  }

  EventResult res(event_row);
  return res;
}

void CalendarBackend::CreateRecurrenceException(
    RecurrenceExceptionRow row,
    std::shared_ptr<EventResultCB> result) {
  if (!db_->DoesEventIdExist(row.parent_event_id)) {
    result->success = false;
    result->message = "Event does not exist.";
    return;
  }

  RecurrenceExceptionID id = db_->CreateRecurrenceException(row);
  EventID parent_event_id = row.parent_event_id;

  if (id) {
    result->event = FillEvent(parent_event_id);
    result->success = true;
    NotifyCalendarChanged();
  } else {
    result->success = false;
  }
}

void CalendarBackend::UpdateRecurrenceException(
    RecurrenceExceptionID recurrence_id,
    const RecurrenceExceptionRow& recurrence,
    std::shared_ptr<EventResultCB> result) {
  RecurrenceExceptionRow recurrence_row;
  if (db_->GetRecurrenceException(recurrence_id, &recurrence_row)) {
    if (recurrence.updateFields & calendar::CANCELLED) {
      recurrence_row.cancelled = recurrence.cancelled;
    }

    if (recurrence.updateFields & calendar::EXCEPTION_EVENT_ID) {
      recurrence_row.exception_event_id = recurrence.exception_event_id;
    }

    if (recurrence.updateFields & calendar::EXCEPTION_DAY) {
      recurrence_row.exception_day = recurrence.exception_day;
    }

    if (recurrence.updateFields & calendar::PARENT_EVENT_ID) {
      recurrence_row.parent_event_id = recurrence.parent_event_id;
    }
  }

  result->success = db_->UpdateRecurrenceExceptionRow(recurrence_row);

  if (result->success) {
    result->event = FillEvent(recurrence_row.parent_event_id);
    NotifyCalendarChanged();
  } else {
    result->success = false;
    result->message = "Could not find recurrence exception row in DB";
    NOTREACHED() << "Could not find recurrence exception row in DB";
    return;
  }
}

void CalendarBackend::GetAllNotifications(
    std::shared_ptr<GetAllNotificationResult> results) {
  NotificationRows rows;
  db_->GetAllNotifications(&rows);
  for (size_t i = 0; i < rows.size(); i++) {
    const NotificationRow notification_row = rows[i];
    results->notifications.push_back(notification_row);
  }
}

void CalendarBackend::CreateNotification(
    calendar::NotificationRow row,
    std::shared_ptr<NotificationResult> result) {
  NotificationID id = db_->CreateNotification(row);
  if (id) {
    row.id = id;
    result->success = true;
    result->notification_row = row;
    NotifyNotificationChanged(row);
  } else {
    result->success = false;
  }
}

void CalendarBackend::UpdateNotification(
    calendar::UpdateNotificationRow row,
    std::shared_ptr<NotificationResult> result) {
  NotificationRow notification_row;
  if (db_->GetNotificationRow(row.notification_row.id, &notification_row)) {
    if (row.updateFields & calendar::NOTIFICATION_NAME) {
      notification_row.name = row.notification_row.name;
    }

    if (row.updateFields & calendar::NOTIFICATION_DESCRIPTION) {
      notification_row.description = row.notification_row.description;
    }

    if (row.updateFields & calendar::NOTIFICATION_WHEN) {
      notification_row.when = row.notification_row.when;
    }

    if (row.updateFields & calendar::NOTIFICATION_PERIOD) {
      notification_row.period = row.notification_row.period;
    }

    if (row.updateFields & calendar::NOTIFICATION_DELAY) {
      notification_row.delay = row.notification_row.delay;
    }

    result->success = db_->UpdateNotificationRow(notification_row);

    if (result->success) {
      NotificationRow notification_changed_row;
      if (db_->GetNotificationRow(row.notification_row.id,
                                  &notification_changed_row)) {
        result->success = true;
        result->notification_row = notification_changed_row;
        NotifyCalendarChanged();
      }
    }
  } else {
    result->success = false;
    result->message = "Could not find notification  row in DB";
    NOTREACHED() << "Could not find notification row in DB";
    return;
  }
}

void CalendarBackend::DeleteNotification(
    NotificationID notification_id,
    std::shared_ptr<DeleteNotificationResult> result) {
  if (!db_) {
    result->success = false;
    return;
  }

  if (db_->DeleteNotification(notification_id)) {
    result->success = true;
    NotificationRow notification_row;
    NotifyCalendarChanged();
  } else {
    result->success = false;
  }
}

void CalendarBackend::CreateInvite(calendar::InviteRow row,
                                   std::shared_ptr<InviteResult> result) {
  NotificationID id = db_->CreateInvite(row);
  if (id) {
    row.id = id;
    result->success = true;
    result->inviteRow = row;
    EventRow event_row;
    NotifyCalendarChanged();
  } else {
    result->success = false;
  }
}

void CalendarBackend::UpdateInvite(calendar::UpdateInviteRow row,
                                   std::shared_ptr<InviteResult> result) {
  InviteRow invite_row;
  if (db_->GetInviteRow(row.invite_row.id, &invite_row)) {
    if (row.updateFields & calendar::INVITE_ADDRESS) {
      invite_row.address = row.invite_row.address;
    }

    if (row.updateFields & calendar::INVITE_NAME) {
      invite_row.name = row.invite_row.name;
    }

    if (row.updateFields & calendar::INVITE_PARTSTAT) {
      invite_row.partstat = row.invite_row.partstat;
    }

    if (row.updateFields & calendar::INVITE_SENT) {
      invite_row.sent = row.invite_row.sent;
    }

    result->success = db_->UpdateInvite(invite_row);

    if (result->success) {
      EventRow changed_row;
      if (db_->GetRowForEvent(invite_row.event_id, &changed_row)) {
        result->inviteRow = invite_row;
        NotifyCalendarChanged();
      }
    }
  } else {
    result->success = false;
    result->message = "Could not find invite row in DB";
    NOTREACHED() << "Could not find invite row in DB";
    return;
  }
}

void CalendarBackend::DeleteInvite(InviteID invite_id,
                                   std::shared_ptr<DeleteInviteResult> result) {
  if (!db_) {
    result->success = false;
    return;
  }

  if (db_->DeleteInvite(invite_id)) {
    result->success = true;
    EventRow event_row;
    NotifyCalendarChanged();
  } else {
    result->success = false;
  }
}

void CalendarBackend::GetAllCalendars(
    std::shared_ptr<CalendarQueryResults> results) {
  CalendarRows rows;
  db_->GetAllCalendars(&rows);
  for (size_t i = 0; i < rows.size(); i++) {
    const CalendarRow calendarRow = rows[i];
    CalendarResult result(calendarRow);
    results->AppendCalendarBySwapping(&result);
  }
}

void CalendarBackend::UpdateEvent(EventID event_id,
                                  const EventRow& event,
                                  std::shared_ptr<EventResultCB> result) {
  if (!db_) {
    result->success = false;
    return;
  }

  EventRow event_row;
  if (db_->GetRowForEvent(event_id, &event_row)) {
    if (event.updateFields & calendar::CALENDAR_ID) {
      event_row.calendar_id = event.calendar_id;
    }

    if (event.updateFields & calendar::TITLE) {
      event_row.title = event.title;
    }

    if (event.updateFields & calendar::DESCRIPTION) {
      event_row.description = event.description;
    }

    if (event.updateFields & calendar::START) {
      event_row.start = event.start;
    }

    if (event.updateFields & calendar::END) {
      event_row.end = event.end;
    }

    if (event.updateFields & calendar::ALLDAY) {
      event_row.all_day = event.all_day;
    }

    if (event.updateFields & calendar::ISRECURRING) {
      event_row.is_recurring = event.is_recurring;
    }

    if (event.updateFields & calendar::LOCATION) {
      event_row.location = event.location;
    }

    if (event.updateFields & calendar::URL) {
      event_row.url = event.url;
    }

    if (event.updateFields & calendar::ETAG) {
      event_row.etag = event.etag;
    }

    if (event.updateFields & calendar::HREF) {
      event_row.href = event.href;
    }

    if (event.updateFields & calendar::UID) {
      event_row.uid = event.uid;
    }

    if (event.updateFields & calendar::EVENT_TYPE_ID) {
      event_row.event_type_id = event.event_type_id;
    }

    if (event.updateFields & calendar::TASK) {
      event_row.task = event.task;
    }

    if (event.updateFields & calendar::COMPLETE) {
      event_row.complete = event.complete;
    }

    if (event.updateFields & calendar::TRASH) {
      event_row.trash = event.trash;
    }

    if (event.updateFields & calendar::SEQUENCE) {
      event_row.sequence = event.sequence;
    }

    if (event.updateFields & calendar::ICAL) {
      event_row.ical = event.ical;
    }

    if (event.updateFields & calendar::RRULE) {
      event_row.rrule = event.rrule;
    }

    if (event.updateFields & calendar::ORGANIZER) {
      event_row.organizer = event.organizer;
    }

    if (event.updateFields & calendar::TIMEZONE) {
      event_row.timezone = event.timezone;
    }

    if (event.updateFields & calendar::PRIORITY) {
      event_row.priority = event.priority;
    }

    if (event.updateFields & calendar::STATUS) {
      event_row.status = event.status;
    }

    if (event.updateFields & calendar::PERCENTAGE_COMPLETE) {
      event_row.percentage_complete = event.percentage_complete;
    }

    if (event.updateFields & calendar::CATEGORIES) {
      event_row.categories = event.categories;
    }

    if (event.updateFields & calendar::COMPONENT_CLASS) {
      event_row.component_class = event.component_class;
    }

    if (event.updateFields & calendar::ATTACHMENT) {
      event_row.attachment = event.attachment;
    }

    if (event.updateFields & calendar::COMPLETED) {
      event_row.completed = event.completed;
    }

    if (event.updateFields & calendar::SYNC_PENDING) {
      event_row.sync_pending = event.sync_pending;
    }

    if (event.updateFields & calendar::DELETE_PENDING) {
      event_row.delete_pending = event.delete_pending;
    }

    result->success = db_->UpdateEventRow(event_row);

    EventResult updatedEvent = FillEvent(event_id);
    result->event = updatedEvent;

    if (result->success) {
      EventRow changed_row;
      if (db_->GetRowForEvent(event_id, &changed_row)) {
        NotifyCalendarChanged();
      }
    }
  } else {
    result->success = false;
    result->message = "Could not find event row in DB";
    NOTREACHED() << "Could not find event row in DB";
    return;
  }
}

void CalendarBackend::GetAllEventTypes(std::shared_ptr<EventTypeRows> results) {
  EventTypeRows event_type_rows;
  db_->GetAllEventTypes(&event_type_rows);

  for (size_t i = 0; i < event_type_rows.size(); i++) {
    EventTypeRow event_type_row = event_type_rows[i];
    results->push_back(event_type_row);
  }
}

void CalendarBackend::UpdateEventType(
    EventTypeID event_type_id,
    const EventType& event,
    std::shared_ptr<UpdateEventTypeResult> result) {
  if (!db_) {
    result->success = false;
    return;
  }

  EventTypeRow event_row;
  if (db_->GetRowForEventType(event_type_id, &event_row)) {
    if (event.updateFields & calendar::NAME) {
      event_row.set_name(event.name);
    }

    if (event.updateFields & calendar::COLOR) {
      event_row.set_color(event.color);
    }

    if (event.updateFields & calendar::ICONINDEX) {
      event_row.set_iconindex(event.iconindex);
    }

    result->success = db_->UpdateEventTypeRow(event_row);

    if (result->success) {
      EventTypeRow changed_row;
      if (db_->GetRowForEventType(event_type_id, &changed_row)) {
        NotifyCalendarChanged();
      }
    }
  } else {
    result->success = false;
    NOTREACHED() << "Could not find event type row in DB";
    return;
  }
}

void CalendarBackend::DeleteEventType(
    EventTypeID event_type_id,
    std::shared_ptr<DeleteEventTypeResult> result) {
  if (!db_) {
    result->success = false;
    return;
  }

  EventTypeRow event_type_row;
  if (db_->GetRowForEventType(event_type_id, &event_type_row)) {
    result->success = db_->DeleteEventType(event_type_id);
    NotifyCalendarChanged();
  } else {
    result->success = false;
  }
}

void CalendarBackend::DeleteEvent(EventID event_id,
                                  std::shared_ptr<DeleteEventResult> result) {
  if (!db_) {
    result->success = false;
    return;
  }

  EventRow event_row;
  if (db_->GetRowForEvent(event_id, &event_row)) {
    if (db_->DoesRecurrenceExceptionExistForEvent(event_id)) {
      EventIDs event_ids;

      if (db_->GetAllEventExceptionIds(event_id, &event_ids)) {
        for (auto it = event_ids.begin(); it != event_ids.end(); ++it) {
          if (!db_->DeleteEvent(*it)) {
            result->success = false;
            return;
          }
        }
      }
      if (!db_->DeleteRecurrenceExceptions(event_id)) {
        result->success = false;
        return;
      }
    }

    if (!db_->DeleteNotificationsForEvent(event_id)) {
      result->success = false;
      return;
    }

    result->success = db_->DeleteEvent(event_id);
    NotifyCalendarChanged();
  } else {
    result->success = false;
  }
}

void CalendarBackend::DeleteEventRecurrenceException(
    RecurrenceExceptionID exception_id,
    std::shared_ptr<EventResultCB> result) {
  if (!db_) {
    result->success = false;
    return;
  }

  RecurrenceExceptionRow recurrence_exception_row;
  if (!db_->GetRecurrenceException(exception_id, &recurrence_exception_row)) {
    result->success = false;
    return;
  }

  if (!recurrence_exception_row.cancelled) {
    db_->DeleteEvent(recurrence_exception_row.exception_event_id);
  }

  if (db_->DeleteRecurrenceException(exception_id)) {
    EventRow event_row;
    if (db_->GetRowForEvent(recurrence_exception_row.parent_event_id,
                            &event_row)) {
      result->event = event_row;
      result->success = true;
      NotifyCalendarChanged();
    }
    return;
  }
  result->success = false;
}

void CalendarBackend::CreateCalendar(
    CalendarRow calendar,
    std::shared_ptr<CreateCalendarResult> result) {
  CalendarID id = db_->CreateCalendar(calendar);

  if (id) {
    calendar.set_id(id);
    result->success = true;
    result->createdRow = calendar;
    NotifyCalendarChanged();
  } else {
    result->success = false;
  }
}

void CalendarBackend::UpdateCalendar(
    CalendarID calendar_id,
    const Calendar& calendar,
    std::shared_ptr<UpdateCalendarResult> result) {
  if (!db_) {
    result->success = false;
    return;
  }

  CalendarRow calendar_row;
  if (db_->GetRowForCalendar(calendar_id, &calendar_row)) {
    if (calendar.updateFields & calendar::CALENDAR_NAME) {
      calendar_row.set_name(calendar.name);
    }

    if (calendar.updateFields & calendar::CALENDAR_DESCRIPTION) {
      calendar_row.set_description(calendar.description);
    }

    if (calendar.updateFields & calendar::CALENDAR_ORDERINDEX) {
      calendar_row.set_orderindex(calendar.orderindex);
    }

    if (calendar.updateFields & calendar::CALENDAR_COLOR) {
      calendar_row.set_color(calendar.color);
    }

    if (calendar.updateFields & calendar::CALENDAR_HIDDEN) {
      calendar_row.set_hidden(calendar.hidden);
    }

    if (calendar.updateFields & calendar::CALENDAR_ACTIVE) {
      calendar_row.set_active(calendar.active);
    }

    if (calendar.updateFields & calendar::CALENDAR_ICONINDEX) {
      calendar_row.set_iconindex(calendar.iconindex);
    }

    if (calendar.updateFields & calendar::CALENDAR_CTAG) {
      calendar_row.set_ctag(calendar.ctag);
    }

    if (calendar.updateFields & calendar::CALENDAR_LAST_CHECKED) {
      calendar_row.set_last_checked(calendar.last_checked);
    }

    if (calendar.updateFields & calendar::CALENDAR_TIMEZONE) {
      calendar_row.set_timezone(calendar.timezone);
    }

    if (calendar.updateFields & calendar::CALENDAR_SUPPORTED_COMPONENT_SET) {
      calendar_row.set_supported_component_set(
          calendar.supported_component_set);
    }

    result->success = db_->UpdateCalendarRow(calendar_row);

    if (result->success) {
      CalendarRow changed_row;
      if (db_->GetRowForCalendar(calendar_id, &changed_row)) {
        NotifyCalendarChanged();
      }
    }
  } else {
    result->success = false;
    NOTREACHED() << "Could not find calendar row in DB";
    return;
  }
}

void CalendarBackend::DeleteCalendar(
    CalendarID calendar_id,
    std::shared_ptr<DeleteCalendarResult> result) {
  if (!db_) {
    result->success = false;
    return;
  }

  CalendarRow calendar_row;
  if (db_->GetRowForCalendar(calendar_id, &calendar_row)) {
    db_->DeleteRecurrenceExceptionsForCalendar(calendar_id);
    db_->DeleteNotificationsForCalendar(calendar_id);
    db_->DeleteInvitesForCalendar(calendar_id);
    db_->DeleteEventsForCalendar(calendar_id);
    result->success = db_->DeleteCalendar(calendar_id);
    NotifyCalendarChanged();
  } else {
    result->success = false;
  }
}

void CalendarBackend::CreateEventType(
    EventTypeRow event_type_row,
    std::shared_ptr<CreateEventTypeResult> result) {
  EventTypeID id = db_->CreateEventType(event_type_row);

  if (id) {
    event_type_row.set_id(id);
    result->success = true;
    NotifyCalendarChanged();
  } else {
    result->success = false;
  }
}

void CalendarBackend::CreateAccount(
    AccountRow account_row,
    std::shared_ptr<CreateAccountResult> result) {
  EventTypeID id = db_->CreateAccount(account_row);

  if (id) {
    account_row.id = id;
    result->success = true;
    result->createdRow = account_row;
    NotifyCalendarChanged();
  } else {
    result->success = false;
  }
}

void CalendarBackend::DeleteAccount(
    AccountID account_id,
    std::shared_ptr<DeleteAccountResult> result) {
  CalendarIDs calendars;
  db_->GetAllCalendarIdsForAccount(&calendars, account_id);

  for (size_t i = 0; i < calendars.size(); i++) {
    CalendarID calendarId = calendars[i];
    auto delete_calendar_result =
        std::shared_ptr<DeleteCalendarResult>(new DeleteCalendarResult());

    DeleteCalendar(calendarId, delete_calendar_result);

    if (!delete_calendar_result->success) {
      result->success = false;
      result->message = "Error deleting calendar";
      return;
    }
  }

  if (db_->DeleteAccount(account_id)) {
    result->success = true;
    NotifyCalendarChanged();
  } else {
    result->success = false;
  }
}
void CalendarBackend::UpdateAccount(
    AccountRow update_account_row,
    std::shared_ptr<UpdateAccountResult> result) {
  if (!db_) {
    result->success = false;
    return;
  }

  AccountID account_id = update_account_row.id;
  AccountRow account;
  if (db_->GetRowForAccount(account_id, &account)) {
    if (update_account_row.updateFields & calendar::ACCOUNT_NAME) {
      account.name = update_account_row.name;
    }

    if (update_account_row.updateFields & calendar::ACCOUNT_URL) {
      account.url = update_account_row.url;
    }

    if (update_account_row.updateFields & calendar::ACCOUNT_TYPE) {
      account.account_type = update_account_row.account_type;
    }

    if (update_account_row.updateFields & calendar::ACCOUNT_USERNAME) {
      account.username = update_account_row.username;
    }

    if (update_account_row.updateFields & calendar::ACCOUNT_INTERVAL) {
      account.interval = update_account_row.interval;
    }

    if (db_->UpdateAccountRow(account)) {
      result->success = true;
      result->updatedRow = account;
      NotifyCalendarChanged();
    } else {
      result->message = "Error updating account";
      result->success = false;
    }
  }
}

void CalendarBackend::GetAllAccounts(std::shared_ptr<AccountRows> results) {
  AccountRows account_rows;
  db_->GetAllAccounts(&account_rows);

  for (size_t i = 0; i < account_rows.size(); i++) {
    AccountRow account_row = account_rows[i];
    results->push_back(account_row);
  }
}

void CalendarBackend::CloseAllDatabases() {
  if (db_) {
    // Commit the long-running transaction.
    db_->CommitTransaction();
    db_.reset();
  }
}

void CalendarBackend::CancelScheduledCommit() {
  scheduled_commit_.Cancel();
}

void CalendarBackend::Commit() {
  if (!db_)
    return;

#if BUILDFLAG(IS_IOS)
  // Attempts to get the application running long enough to commit the
  // database transaction if it is currently being backgrounded.
  base::ios::ScopedCriticalAction scoped_critical_action;
#endif

  // Note that a commit may not actually have been scheduled if a caller
  // explicitly calls this instead of using ScheduleCommit. Likewise, we
  // may reset the flag written by a pending commit. But this is OK! It
  // will merely cause extra commits (which is kind of the idea). We
  // could optimize more for this case (we may get two extra commits in
  // some cases) but it hasn't been important yet.
  CancelScheduledCommit();

  db_->CommitTransaction();
  DCHECK_EQ(db_->transaction_nesting(), 0)
      << "Somebody left a transaction open";
  db_->BeginTransaction();
}
}  // namespace calendar
