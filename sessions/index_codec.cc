// Copyright (c) 2023 Vivaldi Technologies AS. All rights reserved

#include "sessions/index_codec.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

#include "base/guid.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
// The next two For Decode that read files() from OS
#include "browser/sessions/vivaldi_session_service.h"
#include "components/sessions/vivaldi_session_service_commands.h"
#include "sessions/index_model.h"
#include "sessions/index_node.h"


namespace sessions {

std::string GetWorkspaceId(std::string ext_data) {
  size_t start = ext_data.find("\"workspaceId\":");
  if (start == std::string::npos) {
    return "";
  }
  size_t data_start = start + 14;
  size_t i = data_start;
  for (; ext_data[i] >= '0' && ext_data[i] <= '9'; i++);

  return ext_data.substr(data_start, i - data_start);
}

void AddToParent(Index_Node* parent, const base::FilePath& directory,
                 bool deleted) {
  ::vivaldi::VivaldiSessionService service;
  base::FileEnumerator iter(directory, false, base::FileEnumerator::FILES,
      deleted ? FILE_PATH_LITERAL("*.del") : FILE_PATH_LITERAL("*.bin"));
  for (base::FilePath name = iter.Next(); !name.empty(); name = iter.Next()) {
#if BUILDFLAG(IS_POSIX)
    const std::string filename = name.BaseName().value();
#elif BUILDFLAG(IS_WIN)
    const std::string filename = base::WideToUTF8(name.BaseName().value());
#endif
    // We have no name information when iterating files. So we use the filename
    // in its place (excluding the extension).
    std::string title = filename;
    size_t ext = filename.find_last_of('.');
    if (ext != std::string::npos) {
      // Get rid of the extension
      title.erase(ext, std::string::npos);
    }

    int num_windows = 0;
    int num_tabs = 0;
    IndexCodec::StringToIntMap map;
    IndexCodec::GetSessionContentInfo(name, &num_windows, &num_tabs, &map);
    base::FileEnumerator::FileInfo info = iter.GetInfo();
    base::Time modified = info.GetLastModifiedTime();

    std::unique_ptr<Index_Node> node =
        std::make_unique<Index_Node>(base::GenerateGUID(),
                                     Index_Node::GetNewId());
    node->SetFilename(filename);
    node->SetTitle(base::UTF8ToUTF16(title));
    node->SetCreateTime(modified.ToJsTime());
    node->SetModifyTime(modified.ToJsTime());
    node->SetWindowsCount(num_windows);
    node->SetTabsCount(num_tabs);

    // The map contains a <workspace id><number of tabs> mapping. We only use
    // the workspace id.
    base::Value::List workspaces;
    for (IndexCodec::StringToIntMap::iterator it = map.begin(); it != map.end();
         ++it) {
      base::Value::Dict dict;
      dict.Set("id", std::stod(it->first));
      dict.Set("name", "Recovered workspace");
      workspaces.Append(std::move(dict));
    }
    node->SetWorkspaces(std::move(workspaces));

    parent->Add(std::move(node));
  }
}

IndexCodec::IndexCodec() = default;
IndexCodec::~IndexCodec() = default;

bool IndexCodec::GetVersion(std::string* version, const base::Value& value) {
  if (!value.is_list()) {
    LOG(ERROR) << "Session Index Codec: No list";
    return false;
  }

  for (const auto& session : value.GetList()) {
    if (session.is_dict()) {
      const std::string* entry = session.FindStringPath("version");
      if (entry) {
        *version = *entry;
        return true;
      }
    }
  }
  return false;
}

bool IndexCodec::Decode(Index_Node* items, const base::FilePath& directory,
                        const base::FilePath::StringPieceType& index_name) {
  AddToParent(items, directory, false);
  std::unique_ptr<Index_Node> trash =
      std::make_unique<Index_Node>(Index_Node::trash_node_guid(),
                                   Index_Node::trash_node_id(),
                                   Index_Node::kFolder);
  AddToParent(trash.get(), directory, true);
  items->Add(std::move(trash));

  return true;
}

bool IndexCodec::Decode(Index_Node* items, Index_Node* backup,
                        const base::Value& value) {
  if (!value.is_list()) {
    LOG(ERROR) << "Session Index Codec: No list";
    return false;
  }

  // Currently, we only allow one element in the top level list - the 'items'
  // node that holds all elements including trash.
  for (const auto& entry : value.GetList()) {
    if (entry.is_dict()) {
      const std::string* guid = entry.FindStringPath("guid");
      bool guid_valid = guid && !guid->empty() && base::IsValidGUID(*guid);
      if (guid_valid) {
        std::map<std::string, bool>::iterator it = guids_.find(*guid);
        if (it != guids_.end()) {
          LOG(ERROR) << "Session Index Codec: guid collision " << *guid;
#if defined(OFFICIAL_BUILD)
          return false;
#else
          return true;  // Do not stop parsing in devel mode.
#endif
        }
        guids_[*guid] = true;

        if (*guid == Index_Node::items_node_guid()) {
          bool parsing_ok = true;
          const base::Value* children = entry.FindPath("children");
          if (children) {
            parsing_ok = DecodeNode(items, *children);
            if (!parsing_ok) {
              LOG(ERROR) << "Session Index Codec: Failed to read all children for " << *guid;
            }
          }
        } else if (*guid == Index_Node::backup_node_guid()) {
          // We just populate the existing node. There are no children
          SetNodeFields(backup, nullptr, entry);
        } else {
          LOG(ERROR) << "Session Index Codec: Illegal top level guid";
        }
      } else {
        LOG(ERROR) << "Session Index Codec: Guid missing or not valid";
#if !defined(OFFICIAL_BUILD)
        LOG(ERROR) << "Session Index Codec: Developer - Missing in profile file, "
                      "remove that file.";
#endif
      }
    } else { // element is not a dict
      LOG(ERROR) << "Session Index Codec: Wrong list format";
      return false;
    }
  }
  return true;
}

bool IndexCodec::DecodeNode(Index_Node* parent,
                           const base::Value& value) {
  if (value.is_list()) {
    for (const auto& entry : value.GetList()) {
      if (!DecodeNode(parent, entry)) {
        return false;
      }
    }
  } else if (value.is_dict()) {
    const std::string* guid = value.FindStringPath("guid");
    bool guid_valid = guid && !guid->empty() && base::IsValidGUID(*guid);
    if (guid_valid) {
      std::map<std::string, bool>::iterator it = guids_.find(*guid);
      if (it != guids_.end()) {
        LOG(ERROR) << "Session Index Codec: guid collision " << *guid;
#if defined(OFFICIAL_BUILD)
        return false;
#else
        return true;    // Do not stop parsing in devel mode.
#endif
      }
      guids_[*guid] = true;
    }
    bool is_special_node =
      *guid == Index_Node::trash_node_guid() ||
      *guid == Index_Node::autosave_node_guid();
    int id = is_special_node
      ? (*guid == Index_Node::trash_node_guid()
          ? Index_Node::trash_node_id()
          : Index_Node::autosave_node_id())
      : Index_Node::GetNewId();
    const int type = value.FindIntKey("type").value_or(
        is_special_node && *guid == Index_Node::trash_node_guid()
          ? Index_Node::kFolder
          : Index_Node::kNode);
    if (type != Index_Node::kFolder && type != Index_Node::kNode) {
      LOG(ERROR) << "Session Index Codec: Unknown node type " << type;
      return false;
    }
    std::unique_ptr<Index_Node> node = std::make_unique<Index_Node>(
        *guid, id, static_cast<Index_Node::Type>(type));
    const std::string* title = value.FindStringPath("title");
    if (title) {
      node->SetTitle(base::UTF8ToUTF16(*title));
    }
    if (type == Index_Node::kNode) {
      SetNodeFields(node.get(), parent, value);
    }
    // An Index_Node::kNode can have children (it is then a container) as well
    // as a regular Index_Node::Folder
    const base::Value* children = value.FindPath("children");
    if (children) {
      if (!DecodeNode(node.get(), *children)) {
        return false;
      }
    }
    parent->Add(std::move(node));
  } else {
    LOG(ERROR) << "Session Index Codec: Illegal category";
    return false;
  }
  return true;
}

void IndexCodec::SetNodeFields(Index_Node* node, Index_Node* parent,
                               const base::Value& value) {
  const std::string* filename = value.FindStringPath("filename");
  const std::string* container_guid = value.FindStringPath("containerguid");
  const double create_time = value.FindDoubleKey("createtime").value_or(0);
  const double modify_time = value.FindDoubleKey("modifytime").value_or(0);
  const int windows_count = value.FindIntKey("windowscount").value_or(0);
  const int tabs_count = value.FindIntKey("tabscount").value_or(0);
  const base::Value::List* workspaces = value.GetIfDict()
    ? value.GetIfDict()->FindList("workspaces")
    : nullptr;
  if (filename) {
    node->SetFilename(*filename);
  }
  if (container_guid) {
    node->SetContainerGuid(*container_guid);
  } else if (parent && parent->is_container()) {
    node->SetContainerGuid(parent->guid());
  }
  node->SetCreateTime(create_time);
  node->SetModifyTime(modify_time);
  node->SetWindowsCount(windows_count);
  node->SetTabsCount(tabs_count);
  if (workspaces) {
    base::Value::List list(workspaces->Clone());
    node->SetWorkspaces(std::move(list));
  }
}

base::Value IndexCodec::Encode(Index_Model* model) {
  base::Value list = base::Value(base::Value::Type::LIST);
  list.GetList().Append(EncodeNode(model->items_node()));
  if (model->backup_node()) {
    list.GetList().Append(EncodeNode(model->backup_node()));
  }
  return list;
}

base::Value IndexCodec::EncodeNode(Index_Node* node) {
  if (node->is_folder() || node->is_container()) {
    base::Value dict(base::Value::Type::DICT);
    dict.SetKey("guid", base::Value(node->guid()));
    dict.SetKey("type", base::Value(node->type()));
    //dict.SetKey("containerguid", base::Value(node->container_guid()));
    if (node->is_container()) {
      dict.SetKey("filename", base::Value(node->filename()));
      dict.SetKey("title", base::Value(node->GetTitle()));
      dict.SetKey("createtime", base::Value(node->create_time()));
      dict.SetKey("modifytime", base::Value(node->modify_time()));
      dict.SetKey("windowscount", base::Value(node->windows_count()));
      dict.SetKey("tabscount", base::Value(node->tabs_count()));
      base::Value::List workspaces(node->workspaces().Clone());
      dict.SetKey("workspaces", base::Value(std::move(workspaces)));
    }
    base::Value list(base::Value::Type::LIST);
    for (const auto& child : node->children()) {
      list.GetList().Append(EncodeNode(child.get()));
    }
    dict.SetKey("children", base::Value(std::move(list)));
    return dict;
  }

  base::Value dict(base::Value::Type::DICT);
  dict.SetKey("guid", base::Value(node->guid()));
  dict.SetKey("type", base::Value(node->type()));
  dict.SetKey("containerguid", base::Value(node->container_guid()));
  dict.SetKey("filename", base::Value(node->filename()));
  dict.SetKey("title", base::Value(node->GetTitle()));
  dict.SetKey("createtime", base::Value(node->create_time()));
  dict.SetKey("modifytime", base::Value(node->modify_time()));
  dict.SetKey("windowscount", base::Value(node->windows_count()));
  dict.SetKey("tabscount", base::Value(node->tabs_count()));
  base::Value::List workspaces(node->workspaces().Clone());
  dict.SetKey("workspaces", base::Value(std::move(workspaces)));

  return dict;
}

// static
void IndexCodec::GetSessionContentInfo(base::FilePath name, int* num_windows,
    int* num_tabs, StringToIntMap* workspaces) {
  ::vivaldi::VivaldiSessionService service;
  auto cmds = service.LoadSettingInfo(name);
  std::vector<std::unique_ptr<sessions::SessionCommand>> commands;
  commands.swap(cmds);
  sessions::IdToSessionTab tabs;
  sessions::TokenToSessionTabGroup tab_groups;
  sessions::IdToSessionWindow windows;
  SessionID active_window_id = SessionID::InvalidValue();
  sessions::VivaldiCreateTabsAndWindows(commands, &tabs,
                                        &tab_groups, &windows,
                                        &active_window_id);
  *num_windows = windows.size();
  *num_tabs = tabs.size();
  if (workspaces) {
    for (sessions::IdToSessionTab::iterator it = tabs.begin();
         it != tabs.end(); ++it) {
      std::string workspace = GetWorkspaceId(it->second->viv_ext_data);
      if (!workspace.empty()) {
        StringToIntMap::iterator ws = workspaces->find(workspace);
        (*workspaces)[workspace] = ws == workspaces->end() ? 1
            : ws->second + 1;
      }
    }
  }
}

}  // namespace sessions