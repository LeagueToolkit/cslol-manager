#include "wadlib.h"
#include "file.hpp"
#include "link.h"
#include <Windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <iostream>

namespace fs = std::filesystem;
using fspath = fs::path;


void msg_done() {
    puts("Done!");
    MessageBoxA(nullptr, "Done!", "Done!", MB_OK);
}
void msg_error(std::string msg) {
    puts("Error!");
    puts(msg.c_str());
    MessageBoxA(nullptr, msg.c_str(), "Error!", MB_OK);
}
void msg_error(std::exception const& except) {
    msg_error(except.what());
}




char const* browse_filter_wxy = "Wooxy file\0"
                                "*.wxy\0\0";

char const* browse_filter_wad = "Wad file\0"
                                "*.wad;*.wad.client\0\0";
char const* browse_filter_wad_or_hashtable = "Wad file\0"
                                             "*.wad;*.wad.client;*.txt;*.hashtable\0"
                                             "Hashtable\0"
                                             "*.wad;*.wad.client;*.txt;*.hashtable\0";

char const* browse_filter_lol_exe = "League of Legends\0"
                                     "LoL*.lnk;"
                                     "League*.lnk;"
                                     "League of Legends.exe;"
                                     "LeagueClient.exe;"
                                     "LeagueClientUx.exe;"
                                     "LeagueClientUxRender.exe;"
                                     "lol.launcher.exe;"
                                     "lol.launcher.admin.exe\0\0";


fspath open_dir_browse(fspath defval, std::string title) {
    // auto result = defval.generic_string();
    BROWSEINFO bi = {};
    bi.lpszTitle = title.c_str();
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    bi.lpfn = nullptr;
    bi.lParam = 0;

    if (auto pidl = SHBrowseForFolderA(&bi); pidl != nullptr) {
        char path[MAX_PATH];
        SHGetPathFromIDListA(pidl, path);
        if (IMalloc* imalloc = nullptr; SUCCEEDED(SHGetMalloc(&imalloc))) {
            imalloc->Free(pidl);
            imalloc->Release();
        }
        return path;
    }
    return defval;
}

fspath open_file_browse(char const* extensions, fspath defval, std::string title) {
    OPENFILENAME ofn = {};
    char szFile[MAX_PATH] = { 0 };
    auto defExt = defval.extension().generic_string();
    auto sname = defval.filename().generic_string();
    auto dname = defval.parent_path().generic_string();
    if(dname.empty()) {
        dname = fs::current_path().generic_string();
    }
    std::copy(sname.begin(), sname.end(), szFile);

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = extensions;
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.lpstrTitle = title.c_str();
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_DONTADDTORECENT | OFN_EXPLORER;
    //ofn.lpstrDefExt = defExt.c_str();


    if (GetOpenFileNameA(&ofn))
    {
        return ofn.lpstrFile;
    }
    return {};
}

fspath save_file_browse(char const* extensions, fspath defval, std::string title) {
    OPENFILENAME ofn = {};
    char szFile[MAX_PATH] = { 0 };
    auto defExt = defval.extension().generic_string();
    auto sname = defval.filename().generic_string();
    auto dname = defval.parent_path().generic_string();
    if(dname.empty()) {
        dname = fs::current_path().generic_string();
    }
    std::copy(sname.begin(), sname.end(), szFile);

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = extensions;
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.lpstrTitle = title.c_str();
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = dname.c_str();
    ofn.lpstrDefExt = nullptr;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_DONTADDTORECENT | OFN_EXPLORER;
    //ofn.lpstrDefExt = defExt.c_str();

    if (GetSaveFileNameA(&ofn))
    {
         return ofn.lpstrFile;
    }
    return defval;
}

extern std::string get_input_string_dialog(std::string defval, std::string message) {
    puts(message.c_str());
    std::string val;
    std::getline(std::cin, val);
    if(val.empty() || val[0] == '\n' || val[0] == '\r') {
        return defval;
    }
    return val;
}

