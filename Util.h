#pragma once
#include <string>
#include "yaml-cpp/yaml.h"
namespace util {

    struct URL {
        std::string protocol;
        std::string host;
        std::string path;
    };

    URL ParseURL(const std::string& url);

    YAML::Node Navigate(YAML::Node& base, const std::vector<std::string>& keys);

    std::vector<std::string> SplitAndDecode(const std::string& ref);

    bool IsJsonMimeType(const std::string& type);

    std::vector<std::string> GetSupportedMimeTypes(const YAML::Node& content);

    std::vector<std::string> GetMediaRanges(const YAML::Node& content);

    std::vector<std::string> GetMediaTypes(const std::vector<std::string>& mediaRanges);

    void FixRefs(YAML::Node& obj);
}