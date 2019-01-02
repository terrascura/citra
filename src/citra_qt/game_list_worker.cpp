// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <QDir>
#include <QFileInfo>
#include "citra_qt/compatibility_list.h"
#include "citra_qt/game_list.h"
#include "citra_qt/game_list_p.h"
#include "citra_qt/game_list_worker.h"
#include "citra_qt/ui_settings.h"
#include "common/common_paths.h"
#include "common/file_util.h"
#include "core/hle/service/am/am.h"
#include "core/hle/service/fs/archive.h"
#include "core/loader/loader.h"
#include "core/settings.h"

namespace {
bool HasSupportedFileExtension(const std::string& file_name) {
    const QFileInfo file = QFileInfo(QString::fromStdString(file_name));
    return GameList::supported_file_extensions.contains(file.suffix(), Qt::CaseInsensitive);
}
} // Anonymous namespace

GameListWorker::GameListWorker(QList<UISettings::GameDir>& game_dirs,
                               const CompatibilityList& compatibility_list)
    : game_dirs(game_dirs), compatibility_list(compatibility_list) {}

GameListWorker::~GameListWorker() = default;

void GameListWorker::AddFstEntriesToGameList(const std::string& dir_path, unsigned int recursion,
                                             GameListDir* parent_dir) {
    const auto callback = [this, recursion, parent_dir](u64* num_entries_out,
                                                        const std::string& directory,
                                                        const std::string& virtual_name) -> bool {
        if (stop_processing) {
            // Breaks the callback loop.
            return false;
        }

        const std::string physical_name = directory + DIR_SEP + virtual_name;
        const bool is_dir = FileUtil::IsDirectory(physical_name);
        if (!is_dir && HasSupportedFileExtension(physical_name)) {
            std::unique_ptr<Loader::AppLoader> loader = Loader::GetLoader(physical_name);
            if (!loader)
                return true;

            u64 program_id = 0;
            loader->ReadProgramId(program_id);

            u64 extdata_id = 0;
            loader->ReadExtdataId(extdata_id);

            std::vector<u8> smdh = [program_id, &loader]() -> std::vector<u8> {
                std::vector<u8> original_smdh;
                loader->ReadIcon(original_smdh);

                if (program_id < 0x0004000000000000 || program_id > 0x00040000FFFFFFFF)
                    return original_smdh;

                std::string update_path = Service::AM::GetTitleContentPath(
                    Service::FS::MediaType::SDMC, program_id + 0x0000000E00000000);

                if (!FileUtil::Exists(update_path))
                    return original_smdh;

                std::unique_ptr<Loader::AppLoader> update_loader = Loader::GetLoader(update_path);

                if (!update_loader)
                    return original_smdh;

                std::vector<u8> update_smdh;
                update_loader->ReadIcon(update_smdh);
                return update_smdh;
            }();

            if (!Loader::IsValidSMDH(smdh) && UISettings::values.game_list_hide_no_icon) {
                // Skip this invalid entry
                return true;
            }

            auto it = FindMatchingCompatibilityEntry(compatibility_list, program_id);

            // The game list uses this as compatibility number for untested games
            QString compatibility("99");
            if (it != compatibility_list.end())
                compatibility = it->second.first;

            emit EntryReady(
                {
                    new GameListItemPath(QString::fromStdString(physical_name), smdh, program_id,
                                         extdata_id),
                    new GameListItemCompat(compatibility),
                    new GameListItemRegion(smdh),
                    new GameListItem(
                        QString::fromStdString(Loader::GetFileTypeString(loader->GetFileType()))),
                    new GameListItemSize(FileUtil::GetSize(physical_name)),
                },
                parent_dir);

        } else if (is_dir && recursion > 0) {
            watch_list.append(QString::fromStdString(physical_name));
            AddFstEntriesToGameList(physical_name, recursion - 1, parent_dir);
        }

        return true;
    };

    FileUtil::ForeachDirectoryEntry(nullptr, dir_path, callback);
}

void GameListWorker::run() {
    stop_processing = false;
    for (UISettings::GameDir& game_dir : game_dirs) {
        if (game_dir.path == "INSTALLED") {
            QString path =
                QString::fromStdString(
                (Settings::values.sdmc_dir.empty() ? FileUtil::GetUserPath(FileUtil::UserPath::SDMCDir)
                : std::string(Settings::values.sdmc_dir + "/")) +
                "Nintendo "
                "3DS/00000000000000000000000000000000/"
                "00000000000000000000000000000000/title/00040000");
            watch_list.append(path);
            GameListDir* game_list_dir = new GameListDir(game_dir, GameListItemType::InstalledDir);
            emit DirEntryReady({game_list_dir});
            AddFstEntriesToGameList(path.toStdString(), 2, game_list_dir);
        } else if (game_dir.path == "SYSTEM") {
            QString path =
                QString::fromStdString(FileUtil::GetUserPath(FileUtil::UserPath::NANDDir)) +
                "00000000000000000000000000000000/title/00040010";
            watch_list.append(path);
            GameListDir* game_list_dir = new GameListDir(game_dir, GameListItemType::SystemDir);
            emit DirEntryReady({game_list_dir});
            AddFstEntriesToGameList(path.toStdString(), 2, game_list_dir);
        } else {
            watch_list.append(game_dir.path);
            GameListDir* game_list_dir = new GameListDir(game_dir);
            emit DirEntryReady({game_list_dir});
            AddFstEntriesToGameList(game_dir.path.toStdString(), game_dir.deep_scan ? 256 : 0,
                                    game_list_dir);
        }
    };
    emit Finished(watch_list);
}

void GameListWorker::Cancel() {
    this->disconnect();
    stop_processing = true;
}
