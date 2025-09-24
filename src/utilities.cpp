#include <chrono>
#include <iomanip>
#include <sstream>

#ifdef __APPLE__
#include <string>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include <CoreFoundation/CoreFoundation.h>

std::string getResourcesPath() {
    CFBundleRef bundle = CFBundleGetMainBundle();
    CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(bundle);
    char path[PATH_MAX];
    CFURLGetFileSystemRepresentation(resourcesURL, TRUE, reinterpret_cast<UInt8 *>(path), PATH_MAX);
    CFRelease(resourcesURL);
    return std::string(path) + "/";
}

std::string getAppDataPath() {
    // Get home directory
    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        home = pw->pw_dir;
    }

    std::string dataPath = std::string(home) + "/Library/Application Support/ADD_Todo_App";

    // Create directory if it doesn't exist
    mkdir(dataPath.c_str(), 0755);

    return dataPath;
}

std::string getDatabasePath() {
    return getAppDataPath() + "/todos.db";
}

#else
std::string getResourcesPath() {
    return "./assets/";
}

std::string getDatabasePath() {
    return "./todos.db";  // Fallback for other platforms
}
#endif

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}