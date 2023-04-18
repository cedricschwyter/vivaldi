// Copyright (c) 2018 Vivaldi Technologies AS. All rights reserved

#include "extensions/api/mail/mail_private_api.h"

#include "base/containers/span.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "components/db/mail_client/mail_client_service_factory.h"
#include "extensions/browser/api/file_handlers/app_file_handler_util.h"
#include "extensions/schema/mail_private.h"

using base::Value;
using mail_client::MailClientService;
using mail_client::MailClientServiceFactory;

namespace {
const base::FilePath::CharType kMailDirectory[] = FILE_PATH_LITERAL("Mail");

bool deleteFile(base::FilePath file_path,
                base::FilePath::StringType file_name) {
  if (file_name.length() > 0) {
    file_path = file_path.Append(file_name);
  }

  if (!file_path.IsAbsolute()) {
    return false;
  }

  if (!base::PathExists(file_path)) {
    return false;
  }

  return base::DeleteFile(file_path);
}

base::FilePath::StringType FilePathAsString(const base::FilePath& path) {
#if BUILDFLAG(IS_WIN)
  return path.value();
#else
  return path.value().c_str();
#endif
}

base::FilePath::StringType StringToStringType(const std::string& str) {
#if BUILDFLAG(IS_WIN)
  return base::UTF8ToWide(str);
#else
  return str;
#endif
}

std::string StringTypeToString(const base::FilePath::StringType& str) {
#if BUILDFLAG(IS_WIN)
  return base::WideToUTF8(str);
#else
  return str;
#endif
}

std::vector<base::FilePath::StringType> FindMailFiles(
    base::FilePath file_path) {
  base::FileEnumerator file_enumerator(
      file_path, true, base::FileEnumerator::FILES, FILE_PATH_LITERAL("*"),
      base::FileEnumerator::FolderSearchPolicy::ALL);

  std::vector<base::FilePath::StringType> string_paths;
  for (base::FilePath locale_file_path = file_enumerator.Next();
       !locale_file_path.empty(); locale_file_path = file_enumerator.Next()) {
    string_paths.push_back(FilePathAsString(locale_file_path));
  }

  return string_paths;
}

}  // namespace

namespace extensions {

namespace mail_private = vivaldi::mail_private;

Profile* MailPrivateAsyncFunction::GetProfile() const {
  return Profile::FromBrowserContext(browser_context());
}

MailClientService* MailPrivateAsyncFunction::GetMailClientService() {
  return MailClientServiceFactory::GetForProfile(GetProfile());
}

ExtensionFunction::ResponseAction MailPrivateGetFilePathsFunction::Run() {
  std::unique_ptr<mail_private::GetFilePaths::Params> params(
      mail_private::GetFilePaths::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  base::FilePath file_path = base::FilePath::FromUTF8Unsafe(params->path);

  if (!file_path.IsAbsolute()) {
    return RespondNow(Error(base::StringPrintf(
        "Path must be absolute %s", file_path.AsUTF8Unsafe().c_str())));
  }

  if (!base::DirectoryExists(file_path)) {
    return RespondNow(Error(base::StringPrintf(
        "Directory does not exist %s", file_path.AsUTF8Unsafe().c_str())));
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&FindMailFiles, file_path),
      base::BindOnce(&MailPrivateGetFilePathsFunction::OnFinished, this));

  return RespondLater();
}

ExtensionFunction::ResponseAction MailPrivateGetFullPathFunction::Run() {
  std::unique_ptr<mail_private::GetFullPath::Params> params(
      mail_private::GetFullPath::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  const std::string& filesystem_name = params->filesystem;
  const std::string& filesystem_path = params->path;

  base::FilePath file_path;
  std::string error;
  if (!app_file_handler_util::ValidateFileEntryAndGetPath(
          filesystem_name, filesystem_path, source_process_id(), &file_path,
          &error)) {
    return RespondNow(Error(std::move(error)));
  }

  return RespondNow(OneArgument(base::Value(file_path.AsUTF8Unsafe())));
}

void MailPrivateGetFilePathsFunction::OnFinished(
    const std::vector<base::FilePath::StringType>& results) {
  std::vector<std::string> string_paths;
  for (const auto& result : results) {
    string_paths.push_back(StringTypeToString(result));
  }
  Respond(
      ArgumentList(mail_private::GetFilePaths::Results::Create(string_paths)));
}

ExtensionFunction::ResponseAction MailPrivateGetMailFilePathsFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  base::FilePath file_path = profile->GetPath();

  file_path = file_path.Append(kMailDirectory);

  if (!file_path.IsAbsolute()) {
    return RespondNow(Error(base::StringPrintf(
        "Path must be absolute %s", file_path.AsUTF8Unsafe().c_str())));
  }

  if (!base::DirectoryExists(file_path)) {
    return RespondNow(Error(base::StringPrintf(
        "Directory does not exist %s", file_path.AsUTF8Unsafe().c_str())));
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&FindMailFiles, file_path),
      base::BindOnce(&MailPrivateGetMailFilePathsFunction::OnFinished, this));

  return RespondLater();
}

void MailPrivateGetMailFilePathsFunction::OnFinished(
    const std::vector<base::FilePath::StringType>& results) {
  std::vector<std::string> string_paths;
  for (const auto& result : results) {
    string_paths.push_back(StringTypeToString(result));
  }
  Respond(
      ArgumentList(mail_private::GetFilePaths::Results::Create(string_paths)));
}

base::FilePath GetSavePath(base::FilePath file_path,
                           std::vector<base::FilePath::StringType> string_paths,
                           base::FilePath::StringType file_name) {
  base::FilePath data_dir;
  size_t count = string_paths.size();

  file_path = file_path.Append(kMailDirectory);
  for (size_t i = 0; i < count; i++) {
    file_path = file_path.Append(string_paths[i]);
    if (!base::DirectoryExists(file_path))
      base::CreateDirectory(file_path);
  }

  if (file_name.length() > 0) {
    file_path = file_path.Append(file_name);
  }

  return file_path;
}

bool Save(base::FilePath file_path,
          std::vector<base::FilePath::StringType> string_paths,
          base::FilePath::StringType file_name,
          std::string data,
          bool append) {
  file_path = GetSavePath(file_path, string_paths, file_name);
  if (!file_path.IsAbsolute()) {
    return false;
  }

  // AppendToFile returns true if all data was written, whereas WriteToFile
  // returns number of bytes and -1 on failure.
  if (append) {
    return base::AppendToFile(file_path, data.data());
  } else {
    int size = data.size();
    int wrote = base::WriteFile(file_path, data.data(), size);
    return size == wrote;
  }
}

bool SaveBuffer(base::FilePath file_path,
                std::vector<base::FilePath::StringType> string_paths,
                base::FilePath::StringType file_name,
                const std::vector<uint8_t>& data,
                bool append) {
  file_path = GetSavePath(file_path, string_paths, file_name);
  if (!file_path.IsAbsolute()) {
    return false;
  }
  if (append) {
    return base::AppendToFile(file_path, data);
  } else {
    return base::WriteFile(file_path, data);
  }
}

GetDirectoryResult CreateDirectory(base::FilePath file_path,
                                   std::string directory) {
  GetDirectoryResult result;

  file_path = file_path.Append(kMailDirectory);
  file_path = file_path.AppendASCII(directory);

  if (!file_path.IsAbsolute()) {
    result.success = false;
  }

  if (!base::DirectoryExists(file_path)) {
    result.success = base::CreateDirectory(file_path);
    result.path = FilePathAsString(file_path);
  }

  return result;
}

ExtensionFunction::ResponseAction
MailPrivateWriteTextToMessageFileFunction::Run() {
  std::unique_ptr<mail_private::WriteTextToMessageFile::Params> params(
      mail_private::WriteTextToMessageFile::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  std::vector<base::FilePath::StringType> string_paths;
  for (const auto& path : params->paths) {
    string_paths.push_back(StringToStringType(path));
  }

  base::FilePath::StringType file_name = StringToStringType(params->file_name);
  std::string data = params->raw;
  Profile* profile = Profile::FromBrowserContext(browser_context());
  base::FilePath file_path = profile->GetPath();

  bool append = params->append ? *(params->append) : false;

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&Save, file_path, string_paths, file_name, data, append),
      base::BindOnce(&MailPrivateWriteTextToMessageFileFunction::OnFinished,
                     this));

  return RespondLater();
}

void MailPrivateWriteTextToMessageFileFunction::OnFinished(bool result) {
  if (result == true) {
    Respond(NoArguments());
  } else {
    Respond(Error("Error saving file"));
  }
}

ExtensionFunction::ResponseAction
MailPrivateWriteBufferToMessageFileFunction::Run() {
  std::unique_ptr<mail_private::WriteBufferToMessageFile::Params> params(
      mail_private::WriteBufferToMessageFile::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  std::vector<base::FilePath::StringType> string_paths;
  for (const auto& path : params->paths) {
    string_paths.push_back(StringToStringType(path));
  }

  base::FilePath::StringType file_name = StringToStringType(params->file_name);
  Profile* profile = Profile::FromBrowserContext(browser_context());
  base::FilePath file_path = profile->GetPath();

  bool append = params->append ? *(params->append) : false;

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&SaveBuffer, file_path, string_paths, file_name,
                     params->raw, append),
      base::BindOnce(&MailPrivateWriteBufferToMessageFileFunction::OnFinished,
                     this));

  return RespondLater();
}
void MailPrivateWriteBufferToMessageFileFunction::OnFinished(bool result) {
  if (result == true) {
    Respond(NoArguments());
  } else {
    Respond(Error("Error saving file"));
  }
}

bool Delete(base::FilePath file_path, base::FilePath::StringType file_name) {
  return deleteFile(file_path, file_name);
}

ExtensionFunction::ResponseAction MailPrivateDeleteMessageFileFunction::Run() {
  std::unique_ptr<mail_private::DeleteMessageFile::Params> params(
      mail_private::DeleteMessageFile::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  std::vector<std::string>& string_paths = params->paths;
  base::FilePath::StringType file_name = StringToStringType(params->file_name);

  Profile* profile = Profile::FromBrowserContext(browser_context());
  base::FilePath file_path = profile->GetPath();

  file_path = file_path.Append(kMailDirectory);

  size_t count = string_paths.size();

  for (size_t i = 0; i < count; i++) {
    file_path = file_path.AppendASCII(string_paths[i]);
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&Delete, file_path, file_name),
      base::BindOnce(&MailPrivateDeleteMessageFileFunction::OnFinished, this));

  return RespondLater();
}

void MailPrivateDeleteMessageFileFunction::OnFinished(bool result) {
  if (result == true) {
    Respond(NoArguments());
  } else {
    Respond(Error(base::StringPrintf("Error deleting file")));
  }
}

ReadFileResult Read(base::FilePath file_path) {
  std::string raw = "";
  ReadFileResult result;

  if (!base::PathExists(file_path)) {
    result.success = false;
    return result;
  }

  if (base::ReadFileToString(file_path, &raw)) {
    result.raw = raw;
    result.success = true;

  } else {
    result.success = false;
  }
  return result;
}

ExtensionFunction::ResponseAction MailPrivateReadFileToBufferFunction::Run() {
  std::unique_ptr<mail_private::ReadFileToBuffer::Params> params(
      mail_private::ReadFileToBuffer::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  base::FilePath file_path = base::FilePath::FromUTF8Unsafe(params->file_name);

  if (!file_path.IsAbsolute()) {
    return RespondNow(Error(base::StringPrintf(
        "Path must be absolute %s", file_path.AsUTF8Unsafe().c_str())));
  }

  if (!base::PathExists(file_path)) {
    return RespondNow(Error(base::StringPrintf(
        "File path does not exist %s", file_path.AsUTF8Unsafe().c_str())));
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&Read, file_path),
      base::BindOnce(&MailPrivateReadFileToBufferFunction::OnFinished, this));

  return RespondLater();
}

void MailPrivateReadFileToBufferFunction::OnFinished(ReadFileResult result) {
  if (result.success == true) {
    Respond(
        OneArgument(base::Value(base::as_bytes(base::make_span(result.raw)))));
  } else {
    Respond(Error(base::StringPrintf("Error reading file")));
  }
}
ExtensionFunction::ResponseAction MailPrivateMessageFileExistsFunction::Run() {
  std::unique_ptr<mail_private::MessageFileExists::Params> params(
      mail_private::MessageFileExists::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  std::vector<std::string>& string_paths = params->paths;
  std::string file_name = params->file_name;

  Profile* profile = Profile::FromBrowserContext(browser_context());
  base::FilePath file_path = profile->GetPath();

  file_path = file_path.Append(kMailDirectory);

  size_t count = string_paths.size();

  for (size_t i = 0; i < count; i++) {
    file_path = file_path.AppendASCII(string_paths[i]);
  }

  if (file_name.length() > 0) {
    file_path = file_path.AppendASCII(file_name);
  }
  bool exists = base::PathExists(file_path);
  return RespondNow(
      ArgumentList(mail_private::MessageFileExists::Results::Create(exists)));
}

ExtensionFunction::ResponseAction
MailPrivateReadMessageFileToBufferFunction::Run() {
  std::unique_ptr<mail_private::ReadMessageFileToBuffer::Params> params(
      mail_private::ReadMessageFileToBuffer::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  std::vector<std::string>& string_paths = params->paths;
  std::string file_name = params->file_name;

  Profile* profile = Profile::FromBrowserContext(browser_context());
  base::FilePath file_path = profile->GetPath();

  file_path = file_path.Append(kMailDirectory);

  size_t count = string_paths.size();

  for (size_t i = 0; i < count; i++) {
    file_path = file_path.AppendASCII(string_paths[i]);
  }

  if (file_name.length() > 0) {
    file_path = file_path.AppendASCII(file_name);
  }

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&Read, file_path),
      base::BindOnce(&MailPrivateReadMessageFileToBufferFunction::OnFinished,
                     this));

  return RespondLater();
}

void MailPrivateReadMessageFileToBufferFunction::OnFinished(
    ReadFileResult result) {
  if (result.success == true) {
    Respond(
        OneArgument(base::Value(base::as_bytes(base::make_span(result.raw)))));
  } else {
    Respond(Error(base::StringPrintf("Error reading file")));
  }
}

ExtensionFunction::ResponseAction MailPrivateReadFileToTextFunction::Run() {
  std::unique_ptr<mail_private::ReadFileToText::Params> params(
      mail_private::ReadFileToText::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  std::string path = params->path;
  base::FilePath base_path;
  base::FilePath file_path = base_path.AppendASCII(path);

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&Read, file_path),
      base::BindOnce(&MailPrivateReadFileToTextFunction::OnFinished, this));

  return RespondLater();
}

void MailPrivateReadFileToTextFunction::OnFinished(ReadFileResult result) {
  if (result.success == true) {
    Respond(ArgumentList(
        mail_private::ReadFileToText::Results::Create(result.raw)));
  } else {
    Respond(Error(base::StringPrintf("Error reading file")));
  }
}

GetDirectoryResult GetDirectory(base::FilePath file_path) {
  std::string path = "";
  GetDirectoryResult result;

  if (!base::PathExists(file_path)) {
    result.success = false;
    return result;
  }

  result.path = FilePathAsString(file_path);
  result.success = true;

  return result;
}

ExtensionFunction::ResponseAction MailPrivateGetFileDirectoryFunction::Run() {
  std::unique_ptr<mail_private::GetFileDirectory::Params> params(
      mail_private::GetFileDirectory::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  base::FilePath::StringType hashed_account_id =
      StringToStringType(params->hashed_account_id);

  Profile* profile = Profile::FromBrowserContext(browser_context());
  base::FilePath file_path = profile->GetPath();

  file_path = file_path.Append(kMailDirectory);
  file_path = file_path.Append(hashed_account_id);

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&GetDirectory, file_path),
      base::BindOnce(&MailPrivateGetFileDirectoryFunction::OnFinished, this));

  return RespondLater();
}

void MailPrivateGetFileDirectoryFunction::OnFinished(
    GetDirectoryResult result) {
  if (result.success == true) {
    Respond(ArgumentList(mail_private::GetFileDirectory::Results::Create(
        StringTypeToString(result.path))));
  } else {
    Respond(Error("Directory not found"));
  }
}

ExtensionFunction::ResponseAction
MailPrivateCreateFileDirectoryFunction::Run() {
  std::unique_ptr<mail_private::CreateFileDirectory::Params> params(
      mail_private::CreateFileDirectory::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  std::string hashed_account_id = params->hashed_account_id;

  Profile* profile = Profile::FromBrowserContext(browser_context());
  base::FilePath file_path = profile->GetPath();

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&CreateDirectory, file_path, hashed_account_id),
      base::BindOnce(&MailPrivateCreateFileDirectoryFunction::OnFinished,
                     this));

  return RespondLater();
}

void MailPrivateCreateFileDirectoryFunction::OnFinished(
    GetDirectoryResult result) {
  if (result.success == true) {
    Respond(ArgumentList(mail_private::CreateFileDirectory::Results::Create(
        StringTypeToString(result.path))));
  } else {
    Respond(Error("Directory not created"));
  }
}

mail_client::MessageRow GetMessageRow(const mail_private::Message& message) {
  mail_client::MessageRow row;
  row.searchListId = message.search_list_id;

  if (message.to) {
    row.to = base::UTF8ToUTF16(*message.to);
  }

  if (message.body) {
    row.body = base::UTF8ToUTF16(*message.body);
  }

  if (message.subject) {
    row.subject = base::UTF8ToUTF16(*message.subject);
  }

  if (message.from) {
    row.from = base::UTF8ToUTF16(*message.from);
  }

  if (message.cc) {
    row.cc = base::UTF8ToUTF16(*message.cc);
  }

  if (message.reply_to) {
    row.replyTo = base::UTF8ToUTF16(*message.reply_to);
  }

  return row;
}

ExtensionFunction::ResponseAction MailPrivateCreateMessagesFunction::Run() {
  std::unique_ptr<mail_private::CreateMessages::Params> params(
      mail_private::CreateMessages::Params::Create(args()));

  EXTENSION_FUNCTION_VALIDATE(params.get());

  std::vector<mail_private::Message>& emails = params->messages;
  size_t count = emails.size();
  EXTENSION_FUNCTION_VALIDATE(count > 0);

  std::vector<mail_client::MessageRow> message_rows;

  for (size_t i = 0; i < count; ++i) {
    mail_private::Message& email_details = emails[i];
    mail_client::MessageRow em = GetMessageRow(email_details);
    message_rows.push_back(em);
  }

  MailClientService* client_service =
      MailClientServiceFactory::GetForProfile(GetProfile());

  client_service->CreateMessages(
      message_rows,
      base::BindOnce(&MailPrivateCreateMessagesFunction::CreateMessagesComplete,
                     this),
      &task_tracker_);

  return RespondLater();  // CreateMessagesComplete() will be called
                          // asynchronously.
}

void MailPrivateCreateMessagesFunction::CreateMessagesComplete(
    std::shared_ptr<bool> result) {
  Respond(ArgumentList(mail_private::CreateMessages::Results::Create(*result)));
}

ExtensionFunction::ResponseAction MailPrivateDeleteMessagesFunction::Run() {
  std::unique_ptr<mail_private::DeleteMessages::Params> params(
      mail_private::DeleteMessages::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  std::vector<int> search_list_ids = params->search_list_ids;

  mail_client::SearchListIdRows search_list;

  for (size_t i = 0; i < search_list_ids.size(); i++) {
    search_list.push_back(search_list_ids[i]);
  }

  MailClientService* service =
      MailClientServiceFactory::GetForProfile(GetProfile());
  service->DeleteMessages(
      search_list,
      base::BindOnce(&MailPrivateDeleteMessagesFunction::DeleteMessagesComplete,
                     this),
      &task_tracker_);

  return RespondLater();
}

void MailPrivateDeleteMessagesFunction::DeleteMessagesComplete(
    std::shared_ptr<bool> result) {
  Respond(ArgumentList(mail_private::DeleteMessages::Results::Create(*result)));
}

ExtensionFunction::ResponseAction MailPrivateAddMessageBodyFunction::Run() {
  std::unique_ptr<mail_private::AddMessageBody::Params> params(
      mail_private::AddMessageBody::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  std::u16string body;
  body = base::UTF8ToUTF16(params->body);
  mail_client::SearchListID search_list_id = params->search_list_id;

  MailClientService* service =
      MailClientServiceFactory::GetForProfile(GetProfile());

  service->AddMessageBody(
      search_list_id, body,
      base::BindOnce(&MailPrivateAddMessageBodyFunction::AddMessageBodyComplete,
                     this),
      &task_tracker_);

  return RespondLater();
}

void MailPrivateAddMessageBodyFunction::AddMessageBodyComplete(
    std::shared_ptr<mail_client::MessageResult> results) {
  if (!results->success) {
    Respond(Error(results->message));
  } else {
    Respond(ArgumentList(
        mail_private::AddMessageBody::Results::Create(results->success)));
  }
}

ExtensionFunction::ResponseAction MailPrivateSearchMessagesFunction::Run() {
  std::unique_ptr<mail_private::SearchMessages::Params> params(
      mail_private::SearchMessages::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  std::string search = params->search_value;
  std::u16string searchParam = base::UTF8ToUTF16(search);

  MailClientService* service =
      MailClientServiceFactory::GetForProfile(GetProfile());

  service->SearchEmail(
      searchParam,
      base::BindOnce(&MailPrivateSearchMessagesFunction::MessagesSearchComplete,
                     this),
      &task_tracker_);

  return RespondLater();  // MessagesSearchComplete() will be called
                          // asynchronously.
}

void MailPrivateSearchMessagesFunction::MessagesSearchComplete(
    std::shared_ptr<mail_client::SearchListIdRows> rows) {
  std::vector<double> results;

  for (mail_client::SearchListIdRows::iterator it = rows->begin();
       it != rows->end(); ++it) {
    results.push_back(double(*it));
  }

  Respond(ArgumentList(mail_private::SearchMessages::Results::Create(results)));
}

ExtensionFunction::ResponseAction MailPrivateMatchMessageFunction::Run() {
  std::unique_ptr<mail_private::MatchMessage::Params> params(
      mail_private::MatchMessage::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  std::string search = params->search_value;
  std::u16string searchParam = base::UTF8ToUTF16(search);

  mail_client::SearchListID search_list_id = params->search_list_id;

  MailClientService* service =
      MailClientServiceFactory::GetForProfile(GetProfile());

  service->MatchMessage(
      search_list_id, searchParam,
      base::BindOnce(&MailPrivateMatchMessageFunction::MatchMessageComplete,
                     this),
      &task_tracker_);

  return RespondLater();  // MatchMessageComplete() will be called
                          // asynchronously.
}

void MailPrivateMatchMessageFunction::MatchMessageComplete(
    std::shared_ptr<bool> match) {
  Respond(ArgumentList(mail_private::MatchMessage::Results::Create(*match)));
}

ExtensionFunction::ResponseAction
MailPrivateRebuildAndVacuumDatabaseFunction::Run() {
  MailClientService* service =
      MailClientServiceFactory::GetForProfile(GetProfile());

  service->RebuildAndVacuumDatabase(
      base::BindOnce(
          &MailPrivateRebuildAndVacuumDatabaseFunction::RebuildStartedCallback,
          this),
      &task_tracker_);

  return RespondLater();  // RebuildStartedCallback() will be called
                          // asynchronously.
}

void MailPrivateRebuildAndVacuumDatabaseFunction::RebuildStartedCallback(
    std::shared_ptr<bool> started) {
  Respond(ArgumentList(
      mail_private::RebuildAndVacuumDatabase::Results::Create(*started)));
}

}  //  namespace extensions
