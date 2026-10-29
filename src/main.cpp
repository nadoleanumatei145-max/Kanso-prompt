#include <iostream>
#include <sol/table.hpp>
#include <string>
#include <filesystem>
#include <cstdlib>
#include <array>
#include <fstream>
#include <sol/sol.hpp>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using namespace std;
using json = nlohmann::json;
using namespace sol;

struct CustomModule {
    std::string icon;
    std::string color;
    sol::object value_obj; // Păstrăm obiectul generic (poate fi string sau funcție)
};

std::vector<CustomModule> user_modules;

std::string clean_string(std::string str) {
    if (str.empty()) return str;
    // Elimină whitespace-urile de la final (\n, \r, spații)
    str.erase(str.find_last_not_of(" \t\n\r") + 1);
    // Elimină whitespace-urile de la început
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start != std::string::npos) {
        str = str.substr(start);
    } else {
        str = "";
    }
    return str;
}

std::string lua_exec(std::string command) {
    std::array<char, 128> buffer;
    std::string result;
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return "";
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    pclose(pipe);
    return clean_string(result); // REPARAT: C++ curăță automat rezultatul oricărei comenzi exec!
}

std::string get_git_branch() {
    if (!fs::exists(".git") && !fs::exists("../.git")) return ""; 
    std::array<char, 128> buffer;
    std::string result;
    FILE* pipe = popen("git branch --show-current 2>/dev/null", "r");
    if (!pipe) return "";
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    pclose(pipe);
    if (!result.empty() && result.back() == '\n') result.pop_back();
    return result;
}

void add_module_from_lua(sol::table mod_table) {
    if (!mod_table.valid()) return;

    CustomModule mod;
    mod.icon = mod_table.get_or("icon", std::string(""));
    mod.color = mod_table.get_or("color", std::string("76"));
    mod.value_obj = mod_table["value"];

    user_modules.push_back(mod); // Stocăm în ordinea exactă a apelării!
}

int main(int argc, char* argv[]) {
    int last_exit_code = (argc > 1) ? std::stoi(argv[1]) : 0;
    double duration = (argc > 2) ? std::stod(argv[2]) : 0.0;

    user_modules.clear();

    char* home = std::getenv("HOME");
    std::string home_str = home ? std::string(home) : "";

    // Construim căile absolute sigure pentru fișierele tale de configurare
    std::string json_path = home_str + "/.config/kanso/kanso.json";
    std::string lua_path = home_str + "/.config/kanso/kanso.lua";

    json kanso_json = json::object(); // REPARAT: Inițializăm explicit ca obiect gol {}, NU ca null
    std::ifstream file(json_path);
    if (file.is_open()) {
        try {
            kanso_json = json::parse(file);
        } catch (const json::parse_error& e) {
            std::cerr << "Couldn't parse kanso.json: " << e.what() << "\n";
            kanso_json = json::object(); // Resetăm la obiect gol dacă fișierul e corupt
        }
    }

    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::string, sol::lib::table, sol::lib::os, sol::lib::io);

    lua["newline"] = kanso_json.value("newline", true);
    lua["exit_code"] = last_exit_code;
    lua["duration"] = duration;
    lua["exec"] = &lua_exec;

    std::string cwd;
    try { cwd = fs::current_path().string(); } catch (...) { cwd = "."; }
    if (home && cwd.find(home) == 0) {
        cwd.replace(0, std::string(home).length(), "~");
    }
    lua["current_dir"] = cwd;

    std::string branch = get_git_branch();
    lua["branch"] = branch;

    sol::table gui = lua.create_named_table("gui");

    gui["add_module"] = &add_module_from_lua;

    sol::table location_module = lua.create_table();
    location_module["enabled"] = kanso_json.value("location_module", true);
    location_module["icon"]    = kanso_json.value("location_icon", " ");
    location_module["color"]   = kanso_json.value("location_color", "33");
    gui["location_module"] = location_module;

    sol::table git_module = lua.create_table();
    git_module["enabled"] = kanso_json.value("git_status_module", true);
    git_module["icon"]    = kanso_json.value("git_status_icon", " ");
    git_module["color"]   = kanso_json.value("git_status_color", "171");
    gui["git_module"] = git_module;

    sol::table duration_module = lua.create_table();
    duration_module["enabled"] = kanso_json.value("duration_module", true);
    duration_module["icon"]    = kanso_json.value("duration_icon", "󰔛 ");
    duration_module["color"]   = kanso_json.value("duration_color", "214");
    gui["duration_module"] = duration_module;

    sol::table arrow_module = lua.create_table();
    arrow_module["enabled"]      = kanso_json.value("arrow", true);
    arrow_module["icon"]         = kanso_json.value("arrow_icon", "󰘍 ");
    arrow_module["color_succes"] = kanso_json.value("arrow_color_succes", "76");
    arrow_module["color_fail"]   = kanso_json.value("arrow_color_fail", "196");
    gui["arrow_module"] = arrow_module;

    if (fs::exists(lua_path)) {
        try {
            lua.script_file(lua_path);
        } catch (const sol::error& e) {
            // Prindem doar erorile reale de sintaxă din interiorul scriptului utilizatorului
            std::cerr << "Warning kanso.lua (syntax error): " << e.what() << "\n";
        }
    }

    sol::table res_loc = gui["location_module"];
    bool loc_enabled   = res_loc["enabled"];
    std::string loc_icon  = res_loc["icon"];
    std::string loc_color = res_loc["color"];
    
    sol::table res_git = gui["git_module"];
    bool git_enabled = res_git["enabled"];
    std::string git_icon = res_git["icon"];
    std::string git_color = res_git["color"];

    sol::table res_dur = gui["duration_module"];
    bool dur_enabled   = res_dur["enabled"];
    std::string dur_icon  = res_dur["icon"];
    std::string dur_color = res_dur["color"];

    sol::table res_arr = gui["arrow_module"];
    bool arr_enabled   = res_arr["enabled"];
    std::string arr_icon  = res_arr["icon"];
    std::string arr_succ  = res_arr["color_succes"];
    std::string arr_fail  = res_arr["color_fail"];

    bool res_newline = lua["newline"];

    if (loc_enabled) {
        std::cout << "%F{" << loc_color << "}" << loc_icon << cwd << "%f";
    }

    if(git_enabled && !branch.empty()) {
        cout << " %F{" << git_color << "}" << git_icon << branch << "%f";
    }

    // 3. Timpul de execuție
    if (dur_enabled && duration > 0.05) {
        std::cout << " %F{" << dur_color << "}" << dur_icon << duration << "s%f";
    }

    for (const auto& mod : user_modules) {
        std::string raw_content = "";

        if (mod.value_obj.is<sol::protected_function>()) {
            // Dacă e o funcție, o executăm din C++
            sol::protected_function value_func = mod.value_obj.as<sol::protected_function>();
            auto result = value_func();
            if (result.valid()) {
                raw_content = result.get<std::string>();
            }
        } else if (mod.value_obj.is<std::string>()) {
            // Dacă e un string static sau rezultatul direct al unui exec("...") apelat în script
            raw_content = mod.value_obj.as<std::string>();
        }

        raw_content = clean_string(raw_content);

        if (!raw_content.empty()) {
            // C++ formatează totul perfect securizat, adăugând spațiul regulamentar
            std::cout << " %F{" << mod.color << "}" << mod.icon << raw_content << " %f";
        }
    }
    
    if (res_newline) cout << "\n";

    // 4. Indicatorul de prompt (Săgeata)
    if (arr_enabled) {
        std::string final_arrow_color = (last_exit_code == 0) ? arr_succ : arr_fail;
        std::cout << "%F{" << final_arrow_color << "}" << arr_icon << "%f";
    }

    return 0;
}
