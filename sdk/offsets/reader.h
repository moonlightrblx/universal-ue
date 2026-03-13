#pragma once
#include <string>
#include <unordered_map>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <optional>
#include <json/json.hpp>
#include <iostream>

namespace offsets {

    struct offcfg {
        std::unordered_map<std::string, uintptr_t> Bases;
        std::unordered_map<std::string, uintptr_t> Functions;
        struct { uint32_t Class; uint32_t Name; uint32_t Outer; } UObject;
        struct { uint32_t DefaultObject; uint32_t CastFlags; } UClass;
        struct { uint32_t Number; } FName;
        struct { uint32_t Length; uint32_t MaxLength; } FString;
        struct { uint32_t Count; uint32_t Max; } TArray;
        std::string Version;
        std::string Notes;
        std::string LastUpdated;
    };

    namespace globals {
        offcfg data;

        uintptr_t hex(const std::string& s) {
            uintptr_t v = 0;
            std::stringstream ss;
            ss << std::hex << s;
            ss >> v;
            return v;
        }

        void load(const std::string& offset_path) {
            using json = nlohmann::json;
            std::ifstream file(offset_path);
            if (!file.is_open()) {
                std::cerr << "[offsets::globals] Failed to open offset file: " << offset_path << std::endl;
                throw std::runtime_error("Failed to open offset file");
            }

            json j;
            file >> j;

            for (auto& [k, v] : j["bases"].items())
                data.Bases[k] = hex(v.get<std::string>());

            data.UObject.Class = hex(j["offsets"]["UObject"]["Class"]);
            data.UObject.Name = hex(j["offsets"]["UObject"]["Name"]);
            data.UObject.Outer = hex(j["offsets"]["UObject"]["Outer"]);

            data.UClass.DefaultObject = hex(j["offsets"]["UClass"]["DefaultObject"]);
            data.UClass.CastFlags = hex(j["offsets"]["UClass"]["CastFlags"]);

            data.FName.Number = hex(j["offsets"]["FName"]["Number"]);

            data.FString.Length = hex(j["offsets"]["FString"]["Length"]);
            data.FString.MaxLength = hex(j["offsets"]["FString"]["MaxLength"]);

            data.TArray.Count = hex(j["offsets"]["TArray"]["Count"]);
            data.TArray.Max = hex(j["offsets"]["TArray"]["Max"]);

            for (auto& [k, v] : j["functions"].items())
                data.Functions[k] = hex(v.get<std::string>());

            data.Version = j.value("version", "");
            data.Notes = j.value("notes", "");
            data.LastUpdated = j.value("last_updated", "");

            std::cout << "[offsets::globals] Loaded offsets from: " << offset_path << std::endl;
        }

        std::optional<uintptr_t> get(const std::string& key) {
            if (key == "UObject.Class") return data.UObject.Class;
            if (key == "UObject.Name") return data.UObject.Name;
            if (key == "UObject.Outer") return data.UObject.Outer;
            if (key == "UClass.DefaultObject") return data.UClass.DefaultObject;
            if (key == "UClass.CastFlags") return data.UClass.CastFlags;
            if (key == "FName.Number") return data.FName.Number;
            if (key == "FString.Length") return data.FString.Length;
            if (key == "FString.MaxLength") return data.FString.MaxLength;
            if (key == "TArray.Count") return data.TArray.Count;
            if (key == "TArray.Max") return data.TArray.Max;

            auto it = data.Bases.find(key);
            if (it != data.Bases.end()) return it->second;

            auto fit = data.Functions.find(key);
            if (fit != data.Functions.end()) return fit->second;

            std::cerr << "[offsets::globals] Key not found: " << key << std::endl;
            return std::nullopt;
        }
    }

    namespace sdk {
        struct MemberInfo {
            std::string Name;
            uintptr_t Offset;
            uintptr_t Size;
            std::string Type;
        };

        struct ClassInfo {
            uintptr_t Size;
            std::vector<MemberInfo> Members;
        };

        std::unordered_map<std::string, ClassInfo> Classes;

        uintptr_t hex(const std::string& s) {
            uintptr_t v = 0;
            std::stringstream ss;
            ss << std::hex << s;
            ss >> v;
            return v;
        }

        void load(const std::string& sdk_json_path) {
            using json = nlohmann::json;
            std::ifstream sdk_file(sdk_json_path);
            if (!sdk_file.is_open()) {
                std::cerr << "[offsets::sdk] Failed to open SDK file: " << sdk_json_path << std::endl;
                throw std::runtime_error("Failed to open SDK JSON file");
            }

            json sdk_j;
            sdk_file >> sdk_j;
            Classes.clear();

            for (auto& cls : sdk_j) {
                if (!cls.contains("N") || !cls.contains("S") || !cls.contains("M")) continue;

                ClassInfo info;
                info.Size = static_cast<uintptr_t>(cls["S"].get<int>());

                for (auto& m : cls["M"]) {
                    if (!m.contains("N") || !m.contains("O") || !m.contains("S")) continue;

                    MemberInfo member;
                    member.Name = m["N"].get<std::string>();
                    member.Offset = hex(m["O"].get<std::string>());
                    member.Size = hex(m["S"].get<std::string>());
                    member.Type = m.value("T", "");

                    info.Members.push_back(member);
                }

                Classes[cls["N"].get<std::string>()] = info;
            }

            std::cout << "[offsets::sdk] Loaded SDK from: " << sdk_json_path
                << " with " << Classes.size() << " classes." << std::endl;
        }

        std::optional<MemberInfo> get_member(const std::string& class_name, const std::string& member_name) {
            auto it = Classes.find(class_name);
            if (it == Classes.end()) {
                std::cerr << "[offsets::sdk] Class not found: " << class_name << std::endl;
                return std::nullopt;
            }

            for (auto& m : it->second.Members) {
                if (m.Name == member_name) return m;
            }

            std::cerr << "[offsets::sdk] Member not found: " << member_name
                << " in class " << class_name << std::endl;

            std::cout << "Available members in " << class_name << ":" << std::endl;
            for (auto& m : it->second.Members)
                std::cout << "  " << m.Name << " (" << m.Type << ") at offset 0x"
                << std::hex << m.Offset << std::endl;

            return std::nullopt;
        }

        std::optional<uintptr_t> get_class_size(const std::string& class_name) {
            auto it = Classes.find(class_name);
            if (it == Classes.end()) {
                std::cerr << "[offsets::sdk] Class not found: " << class_name << std::endl;
                return std::nullopt;
            }
            return it->second.Size;
        }

        void log_class(const std::string& class_name) {
            auto it = Classes.find(class_name);
            if (it == Classes.end()) {
                std::cerr << "[offsets::sdk] Class not found: " << class_name << std::endl;
                return;
            }

            std::cout << "Class " << class_name << " (size 0x" << std::hex << it->second.Size << "):" << std::endl;
            for (auto& m : it->second.Members)
                std::cout << "  " << m.Name << " (" << m.Type << ") at offset 0x"
                << std::hex << m.Offset << std::endl;
        }
    }

}
