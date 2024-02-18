#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

class URL {
public:
    explicit URL(const std::string& url);
    [[nodiscard]] std::string request() const;

private:
    [[nodiscard]] static std::string identity_decompress(std::string& compressed_text) { return compressed_text; };
    [[nodiscard]] static std::string gzip_decompress(const std::string& compressed_text);
    std::string m_scheme;
    std::string m_host;
    std::string m_path;
    const int maxdatasize = 100;
    const std::unordered_map<std::string, std::function<std::string(std::string&)>> available_decompressions = {
            {"identity", URL::identity_decompress},
            //{"gzip", URL::gzip_decompress}
    };
};
