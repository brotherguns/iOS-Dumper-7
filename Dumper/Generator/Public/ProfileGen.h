#pragma once

#include <filesystem>
#include <fstream>
#include <string>

#include "OffsetFinder/Offsets.h"
#include "Settings.h"
#include "Utils.h"
#include "Menu/Logger.h"

namespace fs = std::filesystem;

class ProfileGen
{
private:
    friend class Generator;

    ProfileGen() = default;

public:
    struct Profile
    {
        int32_t UEVERSION = 0;
        bool UseFNamePool = false;
        bool IsUsingFChunkedFixedUObjectArray = false;
        uintptr_t GNamesOffset = 0;
        uintptr_t GObjectsOffset = 0;
        uintptr_t GWorldOffset = 0;
        uintptr_t ProcessEventOffset = 0;
        uintptr_t StaticLoadObjectOffset = 0;
        uintptr_t SpawnActorFTransOffset = 0;
        uintptr_t InitGameStateOffset = 0;
        uintptr_t BeginPlayOffset = 0;
        uintptr_t CallFunctionByNameWithArgumentsOffset = 0;
        std::string GameName;
        std::string GameVersion;
    };

    static ProfileGen& GetInstance()
    {
        static ProfileGen Instance;
        return Instance;
    }

    static const Profile& GetProfile()
    {
        return GetInstance().m_Profile;
    }

    void Generate(const fs::path& DumperFolder)
    {
        PopulateProfile();

        const fs::path OutPath = DumperFolder / "DumpName.profile";
        std::ofstream Out(OutPath);
        if (!Out)
        {
            LogError("ProfileGen::Generate: failed to open %s", OutPath.string().c_str());
            return;
        }

        const uintptr_t ImageBase = GetModuleBase();

        Out << "{\n";
        Out << "    \"UEVERSION\": " << m_Profile.UEVERSION << ",\n";
        Out << "    \"UseFNamePool\": " << (m_Profile.UseFNamePool ? "true" : "false") << ",\n";
        Out << "    \"IsUsingFChunkedFixedUObjectArray\": " << (m_Profile.IsUsingFChunkedFixedUObjectArray ? "true" : "false") << ",\n";
        Out << "    \"GNamesOffset\": \"0x" << std::hex << std::showbase << m_Profile.GNamesOffset << "\",\n";
        Out << "    \"GObjectsOffset\": \"0x" << std::hex << std::showbase << m_Profile.GObjectsOffset << "\",\n";
        Out << "    \"GWorldOffset\": \"0x" << std::hex << std::showbase << m_Profile.GWorldOffset << "\",\n";
        Out << "    \"ProcessEventOffset\": \"0x" << std::hex << std::showbase << m_Profile.ProcessEventOffset << "\",\n";
        Out << "    \"StaticLoadObjectOffset\": \"0x" << std::hex << std::showbase << m_Profile.StaticLoadObjectOffset << "\",\n";
        Out << "    \"SpawnActorFTransOffset\": \"0x" << std::hex << std::showbase << m_Profile.SpawnActorFTransOffset << "\",\n";
        Out << "    \"InitGameStateOffset\": \"0x" << std::hex << std::showbase << m_Profile.InitGameStateOffset << "\",\n";
        Out << "    \"BeginPlayOffset\": \"0x" << std::hex << std::showbase << m_Profile.BeginPlayOffset << "\",\n";
        Out << "    \"CallFunctionByNameWithArgumentsOffset\": \"0x" << std::hex << std::showbase << m_Profile.CallFunctionByNameWithArgumentsOffset << "\",\n";
        Out << std::dec;
        Out << "    \"GameName\": \"" << m_Profile.GameName << "\",\n";
        Out << "    \"GameVersion\": \"" << m_Profile.GameVersion << "\"\n";
        Out << "}\n";

        Out.close();
        LogSuccess("ProfileGen::Generate: wrote %s", OutPath.string().c_str());
    }

private:
    Profile m_Profile;

    void PopulateProfile()
    {
        const uintptr_t ImageBase = GetModuleBase();

        m_Profile.UEVERSION = UEVERSION;
        m_Profile.UseFNamePool = Settings::Internal::bUseNamePool;
        m_Profile.IsUsingFChunkedFixedUObjectArray = Off::FUObjectArray::bIsChunked;
        m_Profile.GNamesOffset = ImageBase + Off::InSDK::NameArray::GNames;
        m_Profile.GObjectsOffset = ImageBase + Off::InSDK::ObjArray::GObjects;
        m_Profile.GWorldOffset = ImageBase + Off::InSDK::World::GWorld;
        m_Profile.ProcessEventOffset = ImageBase + Off::InSDK::ProcessEvent::PEOffset;
        m_Profile.StaticLoadObjectOffset = 0;
        m_Profile.SpawnActorFTransOffset = 0;
        m_Profile.InitGameStateOffset = 0;
        m_Profile.BeginPlayOffset = 0;
        m_Profile.CallFunctionByNameWithArgumentsOffset = 0;
        m_Profile.GameName = Settings::Generator::GameName;
        m_Profile.GameVersion = Settings::Generator::GameVersion;
    }
};
