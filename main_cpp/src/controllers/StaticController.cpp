#include "controllers/Controllers.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace library {

namespace {

std::string getMimeType(const std::string& path) {
    std::string ext = fs::path(path).extension().string();
    for (auto& c : ext) c = tolower(c);

    if (ext == ".html" || ext == ".htm") return "text/html; charset=utf-8";
    if (ext == ".css") return "text/css; charset=utf-8";
    if (ext == ".js") return "application/javascript; charset=utf-8";
    if (ext == ".json") return "application/json; charset=utf-8";
    if (ext == ".png") return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".gif") return "image/gif";
    if (ext == ".svg") return "image/svg+xml";
    if (ext == ".ico") return "image/x-icon";
    if (ext == ".woff") return "font/woff";
    if (ext == ".woff2") return "font/woff2";
    if (ext == ".ttf") return "font/ttf";
    return "application/octet-stream";
}

crow::response serveFile(const fs::path& filePath) {
    if (!fs::exists(filePath) || !fs::is_regular_file(filePath)) {
        return crow::response(404, "File Not Found");
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        return crow::response(500, "Unable to read file");
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();

    crow::response res(200, content);
    res.add_header("Content-Type", getMimeType(filePath.string()));
    res.add_header("Cache-Control", "no-cache");
    return res;
}

} // anonymous namespace

void registerStaticRoutes(crow::SimpleApp& app, const std::string& publicDir) {
    fs::path baseDir = fs::absolute(publicDir);
    std::cout << "[Static] Registering static routes from: " << baseDir << std::endl;

    // Root -> index.html
    CROW_ROUTE(app, "/")
    ([baseDir]() {
        return serveFile(baseDir / "index.html");
    });

    CROW_ROUTE(app, "/index.html")
    ([baseDir]() {
        return serveFile(baseDir / "index.html");
    });

    // Page shortcuts
    CROW_ROUTE(app, "/admin")
    ([baseDir]() {
        return serveFile(baseDir / "admin.html");
    });

    CROW_ROUTE(app, "/my-loans")
    ([baseDir]() {
        return serveFile(baseDir / "my-loans.html");
    });

    CROW_ROUTE(app, "/login")
    ([baseDir]() {
        return serveFile(baseDir / "login.html");
    });

    CROW_ROUTE(app, "/register")
    ([baseDir]() {
        return serveFile(baseDir / "register.html");
    });

    // Catch-all static file server
    CROW_ROUTE(app, "/<path>")
    ([baseDir](const std::string& subpath) {
        fs::path target = baseDir / subpath;
        if (fs::exists(target) && fs::is_regular_file(target)) {
            return serveFile(target);
        }
        // Fallback for HTML pages without extension
        if (fs::exists(target.string() + ".html")) {
            return serveFile(target.string() + ".html");
        }
        return crow::response(404, "Page Not Found");
    });
}

} // namespace library
