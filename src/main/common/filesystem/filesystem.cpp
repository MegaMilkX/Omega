#include "filesystem.hpp"

#include <assert.h>

#define WIN32_LEAN_AND_MEAN
#include "common/platform/win32/module.hpp"
#include <shlwapi.h>


fs_path::fs_path() {

}
fs_path::fs_path(const char* path)
: fs_path(std::string(path)) {

}
fs_path::fs_path(const std::string& path) {
    std::vector<std::string> tokens;
    std::string token;
    for(int i = 0; i < path.size(); ++i) {
        auto c = path[i];
        auto c1 = path[i + 1];
        if (c == '\\' || c == '/') {
            
        } else {
            while(c != '\0' && c != '\\' && c != '/') {
                token.push_back(c);
                ++i;
                c = path[i];
            }
            if(!token.empty()) {
                if(token == ".." && !tokens.empty() && tokens.back() != "..") {
                    tokens.pop_back();
                } else {
                    tokens.push_back(token);
                }
                token.clear();
            }
        }
    }

    if(tokens.empty()) {
        return;
    }

    stack = tokens;

    str += tokens[0];
    for(int i = 1; i < tokens.size(); ++i) {
        str += "/" + tokens[i];
    }
}

fs_path fs_path::relative(const fs_path& other) {
    assert(!other.stack.empty());
    assert(!stack.empty());
    fs_path res;

    int separation_index = -1;
    for(int i = 0; i < stack.size() && i < other.stack.size(); ++i) {
        if(stack[i] != other.stack[i]) {
            break;
        }
        separation_index = i;
    }

    assert(separation_index != -1);
    
    std::string appendage;
    if(separation_index < stack.size() - 1) {
        int count = stack.size() - separation_index - 1;
        for(int i = 0; i < count; ++i) {
            appendage += std::string("../");
        }
    }

    std::string str;
    str += appendage;
    for(int i = separation_index + 1; i < other.stack.size(); ++i) {
        str += other.stack[i] + ((i == other.stack.size() - 1) ? "" : "/");
    }

    res = fs_path(str);

    return res;
}

const std::string& fs_path::string() const {
    return str;
}
const char*        fs_path::c_str() const {
    return str.c_str();
}


std::string fsGetExtension(const std::string& path) {
    size_t dot_pos = path.find_last_of(".");
    if(dot_pos == std::string::npos) {
        return "";
    }
    if(dot_pos == path.size() - 1) {
        return "";
    }
    std::string ext = path.substr(dot_pos + 1);
    return ext;
}

std::string fsReplaceReservedChars(const std::string& path, char r) {
    std::string result = path;
    for(size_t i = 0; i < path.size(); ++i) {
        const char& c = path[i];
        if (c == '/' || c == '\\' || c == '?' || c == '%' ||
            c == '*' || c == ':' || c == '|' || c == '"' ||
            c == '<' || c == '>'
        ) {
            result[i] = r;
        }
    }
    return result;
}

std::string fsGetDirPath(const std::string& path) {
    return std::string(
        path.begin(), 
        path.begin() + path.find_last_of("\\")
    );
}

std::string fsGetModulePath() {
    std::string filename;
    char buf[512];
    GetModuleFileNameA(win32GetThisModuleHandle(), buf, 512);
    filename = buf;
    return filename;
}

std::string fsGetModuleDir() {
    std::string filename = fsGetModulePath();
    filename = fsGetDirPath(filename);
    return filename;
}

bool fsFileCopy(const std::string& from, const std::string& to) {
    if(CopyFileA(
        from.c_str(),
        to.c_str(),
        false
    ) == FALSE)
    {
        return false;
    }
    return true;
}

void fsFindFiles(const std::string& dir, const std::string& filter, std::vector<std::string>& out) {
    std::string full_filter = dir + "\\*";
    WIN32_FIND_DATAA data;
    HANDLE hFind = FindFirstFileA(full_filter.c_str(), &data);

    char buf[260];
    DWORD len = GetFullPathNameA(full_filter.c_str(), 260, buf, 0);
    std::string dirpath(buf, len);

    if ( hFind != INVALID_HANDLE_VALUE ) 
    {
        do 
        {
            if(std::string(data.cFileName) == "." || 
                std::string(data.cFileName) == "..")
            {
                continue;
            }

            if(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                fsFindFiles(dir + "\\" + std::string(data.cFileName), filter, out);
                continue;
            }

            if(PathMatchSpecA(data.cFileName, filter.c_str())) {
                out.push_back(dir + "\\" + std::string(data.cFileName));
            }
        } while (FindNextFileA(hFind, &data));
        FindClose(hFind);
    }
}

std::vector<std::string> fsFindAllFiles(const std::string& dir, const std::string& filter) {
    std::vector<std::string> names;
    fsFindFiles(dir, filter, names);
    return names;
}

void fsCreateDirRecursive(const std::string& p) {
    auto sanitizeString = [](const std::string& str)->std::string {
        std::string name = str;
        for(size_t i = 0; i < name.size(); ++i) {
            name[i] = (std::tolower(name[i]));
            if(name[i] == '\\') {
                name[i] = '/';
            }
        }
        return name;
    };
    std::string path = sanitizeString(p); 

    size_t offset = 0;
    offset = path.find_first_of("/", offset);
    while(offset != path.npos) {
        std::string part(path.begin(), path.begin() + offset);
        CreateDirectoryA(part.c_str(), 0);
        offset = path.find_first_of("/", offset + 1); 
    }
    CreateDirectoryA(path.c_str(), 0);
}

std::string fsToRelativePath(const std::string& path, const std::string& relative_to) {
    //static_assert(false, "TODO");
    assert(false);
    return "";
}

