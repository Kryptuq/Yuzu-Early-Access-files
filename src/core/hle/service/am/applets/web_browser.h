// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <filesystem>
#include <optional>

#include "common/common_funcs.h"
#include "common/common_types.h"
#include "core/file_sys/vfs_types.h"
#include "core/hle/result.h"
#include "core/hle/service/am/applets/applets.h"
#include "core/hle/service/am/applets/web_types.h"

namespace Core {
class System;
}

namespace FileSys {
enum class ContentRecordType : u8;
}

namespace Service::AM::Applets {

class WebBrowser final : public Applet {
public:
    WebBrowser(Core::System& system_, LibraryAppletMode applet_mode_,
               const Core::Frontend::WebBrowserApplet& frontend_);

    ~WebBrowser() override;

    void Initialize() override;

    bool TransactionComplete() const override;
    ResultCode GetStatus() const override;
    void ExecuteInteractive() override;
    void Execute() override;

    void ExtractOfflineRomFS();

    void WebBrowserExit(WebExitReason exit_reason, std::string last_url = "");

private:
    bool InputTLVExistsInMap(WebArgInputTLVType input_tlv_type) const;

    std::optional<std::vector<u8>> GetInputTLVData(WebArgInputTLVType input_tlv_type);

    // Initializers for the various types of browser applets
    void InitializeShop();
    void InitializeLogin();
    void InitializeOffline();
    void InitializeShare();
    void InitializeWeb();
    void InitializeWifi();
    void InitializeLobby();

    // Executors for the various types of browser applets
    void ExecuteShop();
    void ExecuteLogin();
    void ExecuteOffline();
    void ExecuteShare();
    void ExecuteWeb();
    void ExecuteWifi();
    void ExecuteLobby();

    const Core::Frontend::WebBrowserApplet& frontend;

    bool complete{false};
    ResultCode status{ResultSuccess};

    WebAppletVersion web_applet_version{};
    WebArgHeader web_arg_header{};
    WebArgInputTLVMap web_arg_input_tlv_map;

    u64 title_id{};
    FileSys::ContentRecordType nca_type{};
    std::filesystem::path offline_cache_dir;
    std::filesystem::path offline_document;
    FileSys::VirtualFile offline_romfs;

    std::string external_url;

    Core::System& system;
};

} // namespace Service::AM::Applets
