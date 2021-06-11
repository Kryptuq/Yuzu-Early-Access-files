// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <atomic>
#include <map>
#include <optional>
#include "core/hle/service/bcat/backend/backend.h"

namespace Service::AM::Applets {
class AppletManager;
}

namespace Service::BCAT {

struct EventStatus {
    std::optional<std::string> header;
    std::optional<std::string> footer;
    std::vector<std::string> events;
};

/// Boxcat is yuzu's custom backend implementation of Nintendo's BCAT service. It is free to use and
/// doesn't require a switch or nintendo account. The content is controlled by the yuzu team.
class Boxcat final : public Backend {
    friend void SynchronizeInternal(AM::Applets::AppletManager& applet_manager,
                                    DirectoryGetter dir_getter, TitleIDVersion title,
                                    ProgressServiceBackend& progress,
                                    std::optional<std::string> dir_name);

public:
    explicit Boxcat(AM::Applets::AppletManager& applet_manager_, DirectoryGetter getter);
    ~Boxcat() override;

    bool Synchronize(TitleIDVersion title, ProgressServiceBackend& progress) override;
    bool SynchronizeDirectory(TitleIDVersion title, std::string name,
                              ProgressServiceBackend& progress) override;

    bool Clear(u64 title_id) override;

    void SetPassphrase(u64 title_id, const Passphrase& passphrase) override;

    std::optional<std::vector<u8>> GetLaunchParameter(TitleIDVersion title) override;

    enum class StatusResult {
        Success,
        Offline,
        ParseError,
        BadClientVersion,
    };

    static StatusResult GetStatus(std::optional<std::string>& global,
                                  std::map<std::string, EventStatus>& games);

private:
    std::atomic_bool is_syncing{false};

    class Client;
    std::unique_ptr<Client> client;
    AM::Applets::AppletManager& applet_manager;
};

} // namespace Service::BCAT
