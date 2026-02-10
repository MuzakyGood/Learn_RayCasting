#include "File.hpp"

#include <iostream>
#include <filesystem>

const char *File::getPathFile(const char *path, bool isShowPath)
{
    bool isFileNotfound = false;
    static std::string finalPath;
    std::filesystem::path path1 = std::filesystem::current_path() / path;
    std::filesystem::path path2 = std::filesystem::current_path().parent_path() / path;

    if (std::filesystem::exists(path1)) finalPath = path1.string();
    else if (std::filesystem::exists(path2)) finalPath = path2.string();
    else
    {
        finalPath.clear();
        isFileNotfound = true;
    }

    if (isShowPath)
    {
        if (isFileNotfound) std::cerr << "[File] Error: File or assets not found!" << std::endl;
        else std::cout << "[File] Path: " << finalPath << std::endl;
    }
    
    return finalPath.c_str();
}