// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/crypto/key_manager.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/es/es.h"
#include "core/hle/service/service.h"

namespace Service::ES {

constexpr Result ERROR_INVALID_ARGUMENT{ErrorModule::ETicket, 2};
constexpr Result ERROR_INVALID_RIGHTS_ID{ErrorModule::ETicket, 3};

class ETicket final : public ServiceFramework<ETicket> {
public:
    explicit ETicket(Core::System& system_) : ServiceFramework{system_, "es"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {1, &ETicket::ImportTicket, "ImportTicket"},
            {2, nullptr, "ImportTicketCertificateSet"},
            {3, nullptr, "DeleteTicket"},
            {4, nullptr, "DeletePersonalizedTicket"},
            {5, nullptr, "DeleteAllCommonTicket"},
            {6, nullptr, "DeleteAllPersonalizedTicket"},
            {7, nullptr, "DeleteAllPersonalizedTicketEx"},
            {8, &ETicket::GetTitleKey, "GetTitleKey"},
            {9, &ETicket::CountCommonTicket, "CountCommonTicket"},
            {10, &ETicket::CountPersonalizedTicket, "CountPersonalizedTicket"},
            {11, &ETicket::ListCommonTicketRightsIds, "ListCommonTicketRightsIds"},
            {12, &ETicket::ListPersonalizedTicketRightsIds, "ListPersonalizedTicketRightsIds"},
            {13, nullptr, "ListMissingPersonalizedTicket"},
            {14, &ETicket::GetCommonTicketSize, "GetCommonTicketSize"},
            {15, &ETicket::GetPersonalizedTicketSize, "GetPersonalizedTicketSize"},
            {16, &ETicket::GetCommonTicketData, "GetCommonTicketData"},
            {17, &ETicket::GetPersonalizedTicketData, "GetPersonalizedTicketData"},
            {18, nullptr, "OwnTicket"},
            {19, nullptr, "GetTicketInfo"},
            {20, nullptr, "ListLightTicketInfo"},
            {21, nullptr, "SignData"},
            {22, nullptr, "GetCommonTicketAndCertificateSize"},
            {23, nullptr, "GetCommonTicketAndCertificateData"},
            {24, nullptr, "ImportPrepurchaseRecord"},
            {25, nullptr, "DeletePrepurchaseRecord"},
            {26, nullptr, "DeleteAllPrepurchaseRecord"},
            {27, nullptr, "CountPrepurchaseRecord"},
            {28, nullptr, "ListPrepurchaseRecordRightsIds"},
            {29, nullptr, "ListPrepurchaseRecordInfo"},
            {30, nullptr, "CountTicket"},
            {31, nullptr, "ListTicketRightsIds"},
            {32, nullptr, "CountPrepurchaseRecordEx"},
            {33, nullptr, "ListPrepurchaseRecordRightsIdsEx"},
            {34, nullptr, "GetEncryptedTicketSize"},
            {35, nullptr, "GetEncryptedTicketData"},
            {36, nullptr, "DeleteAllInactiveELicenseRequiredPersonalizedTicket"},
            {37, nullptr, "OwnTicket2"},
            {38, nullptr, "OwnTicket3"},
            {39, nullptr, "DeleteAllInactivePersonalizedTicket"},
            {40, nullptr, "DeletePrepurchaseRecordByNintendoAccountId"},
            {501, nullptr, "Unknown501"},
            {502, nullptr, "Unknown502"},
            {503, nullptr, "GetTitleKey"},
            {504, nullptr, "Unknown504"},
            {508, nullptr, "Unknown508"},
            {509, nullptr, "Unknown509"},
            {510, nullptr, "Unknown510"},
            {511, nullptr, "Unknown511"},
            {1001, nullptr, "Unknown1001"},
            {1002, nullptr, "Unknown1001"},
            {1003, nullptr, "Unknown1003"},
            {1004, nullptr, "Unknown1004"},
            {1005, nullptr, "Unknown1005"},
            {1006, nullptr, "Unknown1006"},
            {1007, nullptr, "Unknown1007"},
            {1009, nullptr, "Unknown1009"},
            {1010, nullptr, "Unknown1010"},
            {1011, nullptr, "Unknown1011"},
            {1012, nullptr, "Unknown1012"},
            {1013, nullptr, "Unknown1013"},
            {1014, nullptr, "Unknown1014"},
            {1015, nullptr, "Unknown1015"},
            {1016, nullptr, "Unknown1016"},
            {1017, nullptr, "Unknown1017"},
            {1018, nullptr, "Unknown1018"},
            {1019, nullptr, "Unknown1019"},
            {1020, nullptr, "Unknown1020"},
            {1021, nullptr, "Unknown1021"},
            {1501, nullptr, "Unknown1501"},
            {1502, nullptr, "Unknown1502"},
            {1503, nullptr, "Unknown1503"},
            {1504, nullptr, "Unknown1504"},
            {1505, nullptr, "Unknown1505"},
            {1506, nullptr, "Unknown1506"},
            {2000, nullptr, "Unknown2000"},
            {2001, nullptr, "Unknown2001"},
            {2002, nullptr, "Unknown2002"},
            {2003, nullptr, "Unknown2003"},
            {2100, nullptr, "Unknown2100"},
            {2501, nullptr, "Unknown2501"},
            {2502, nullptr, "Unknown2502"},
            {2601, nullptr, "Unknown2601"},
            {3001, nullptr, "Unknown3001"},
            {3002, nullptr, "Unknown3002"},
        };
        // clang-format on
        RegisterHandlers(functions);

        keys.PopulateTickets();
        keys.SynthesizeTickets();
    }

private:
    bool CheckRightsId(Kernel::HLERequestContext& ctx, const u128& rights_id) {
        if (rights_id == u128{}) {
            LOG_ERROR(Service_ETicket, "The rights ID was invalid!");
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ERROR_INVALID_RIGHTS_ID);
            return false;
        }

        return true;
    }

    void ImportTicket(Kernel::HLERequestContext& ctx) {
        const auto ticket = ctx.ReadBuffer();
        const auto cert = ctx.ReadBuffer(1);

        if (ticket.size() < sizeof(Core::Crypto::Ticket)) {
            LOG_ERROR(Service_ETicket, "The input buffer is not large enough!");
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ERROR_INVALID_ARGUMENT);
            return;
        }

        Core::Crypto::Ticket raw{};
        std::memcpy(&raw, ticket.data(), sizeof(Core::Crypto::Ticket));

        if (!keys.AddTicketPersonalized(raw)) {
            LOG_ERROR(Service_ETicket, "The ticket could not be imported!");
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ERROR_INVALID_ARGUMENT);
            return;
        }

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void GetTitleKey(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const auto rights_id = rp.PopRaw<u128>();

        LOG_DEBUG(Service_ETicket, "called, rights_id={:016X}{:016X}", rights_id[1], rights_id[0]);

        if (!CheckRightsId(ctx, rights_id))
            return;

        const auto key =
            keys.GetKey(Core::Crypto::S128KeyType::Titlekey, rights_id[1], rights_id[0]);

        if (key == Core::Crypto::Key128{}) {
            LOG_ERROR(Service_ETicket,
                      "The titlekey doesn't exist in the KeyManager or the rights ID was invalid!");
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ERROR_INVALID_RIGHTS_ID);
            return;
        }

        ctx.WriteBuffer(key);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void CountCommonTicket(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_ETicket, "called");

        const u32 count = static_cast<u32>(keys.GetCommonTickets().size());

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push<u32>(count);
    }

    void CountPersonalizedTicket(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_ETicket, "called");

        const u32 count = static_cast<u32>(keys.GetPersonalizedTickets().size());

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push<u32>(count);
    }

    void ListCommonTicketRightsIds(Kernel::HLERequestContext& ctx) {
        u32 out_entries;
        if (keys.GetCommonTickets().empty())
            out_entries = 0;
        else
            out_entries = static_cast<u32>(ctx.GetWriteBufferSize() / sizeof(u128));

        LOG_DEBUG(Service_ETicket, "called, entries={:016X}", out_entries);

        keys.PopulateTickets();
        const auto tickets = keys.GetCommonTickets();
        std::vector<u128> ids;
        std::transform(tickets.begin(), tickets.end(), std::back_inserter(ids),
                       [](const auto& pair) { return pair.first; });

        out_entries = static_cast<u32>(std::min<std::size_t>(ids.size(), out_entries));
        ctx.WriteBuffer(ids.data(), out_entries * sizeof(u128));

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push<u32>(out_entries);
    }

    void ListPersonalizedTicketRightsIds(Kernel::HLERequestContext& ctx) {
        u32 out_entries;
        if (keys.GetPersonalizedTickets().empty())
            out_entries = 0;
        else
            out_entries = static_cast<u32>(ctx.GetWriteBufferSize() / sizeof(u128));

        LOG_DEBUG(Service_ETicket, "called, entries={:016X}", out_entries);

        keys.PopulateTickets();
        const auto tickets = keys.GetPersonalizedTickets();
        std::vector<u128> ids;
        std::transform(tickets.begin(), tickets.end(), std::back_inserter(ids),
                       [](const auto& pair) { return pair.first; });

        out_entries = static_cast<u32>(std::min<std::size_t>(ids.size(), out_entries));
        ctx.WriteBuffer(ids.data(), out_entries * sizeof(u128));

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push<u32>(out_entries);
    }

    void GetCommonTicketSize(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const auto rights_id = rp.PopRaw<u128>();

        LOG_DEBUG(Service_ETicket, "called, rights_id={:016X}{:016X}", rights_id[1], rights_id[0]);

        if (!CheckRightsId(ctx, rights_id))
            return;

        const auto ticket = keys.GetCommonTickets().at(rights_id);

        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(ResultSuccess);
        rb.Push<u64>(ticket.GetSize());
    }

    void GetPersonalizedTicketSize(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const auto rights_id = rp.PopRaw<u128>();

        LOG_DEBUG(Service_ETicket, "called, rights_id={:016X}{:016X}", rights_id[1], rights_id[0]);

        if (!CheckRightsId(ctx, rights_id))
            return;

        const auto ticket = keys.GetPersonalizedTickets().at(rights_id);

        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(ResultSuccess);
        rb.Push<u64>(ticket.GetSize());
    }

    void GetCommonTicketData(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const auto rights_id = rp.PopRaw<u128>();

        LOG_DEBUG(Service_ETicket, "called, rights_id={:016X}{:016X}", rights_id[1], rights_id[0]);

        if (!CheckRightsId(ctx, rights_id))
            return;

        const auto ticket = keys.GetCommonTickets().at(rights_id);

        const auto write_size = std::min<u64>(ticket.GetSize(), ctx.GetWriteBufferSize());
        ctx.WriteBuffer(&ticket, write_size);

        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(ResultSuccess);
        rb.Push<u64>(write_size);
    }

    void GetPersonalizedTicketData(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const auto rights_id = rp.PopRaw<u128>();

        LOG_DEBUG(Service_ETicket, "called, rights_id={:016X}{:016X}", rights_id[1], rights_id[0]);

        if (!CheckRightsId(ctx, rights_id))
            return;

        const auto ticket = keys.GetPersonalizedTickets().at(rights_id);

        const auto write_size = std::min<u64>(ticket.GetSize(), ctx.GetWriteBufferSize());
        ctx.WriteBuffer(&ticket, write_size);

        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(ResultSuccess);
        rb.Push<u64>(write_size);
    }

    Core::Crypto::KeyManager& keys = Core::Crypto::KeyManager::Instance();
};

void InstallInterfaces(SM::ServiceManager& service_manager, Core::System& system) {
    std::make_shared<ETicket>(system)->InstallAsService(service_manager);
}

} // namespace Service::ES
