#include "Util.h"

#include <iostream>
#include <regex>


namespace util {

    const std::regex r_application_json(R"(^(application/json|[^;\/ \t]+\/[^;\/ \t]+[+]json)[ \t]*(;.*)?$)", std::regex_constants::icase);
    const std::regex r_url(R"(^(https?)://([^/]+)(/.*)?$)");

    URL ParseURL(const std::string& url) {

        std::smatch match;

        URL result;
        if (std::regex_match(url, match, r_url)) {
            result.protocol = match[1].str();  // "http" or "https"
            result.host = match[2].str();      // "demo.example.com"
            result.path = match[3].matched ? match[3].str() : "/"; // Default to "/" if no path
        }
        return result;
    }

    std::vector<std::string> SplitAndDecode(const std::string& ref) {
        std::vector<std::string> keys;
        std::stringstream ss(ref);
        std::string item;

        while (std::getline(ss, item, '/')) {
            size_t pos;
            while ((pos = item.find("~1")) != std::string::npos) {
                item.replace(pos, 2, "/");
            }
            while ((pos = item.find("~0")) != std::string::npos) {
                item.replace(pos, 2, "~");
            }
            keys.push_back(item);
        }

        if (!keys.empty()) {
            keys.erase(keys.begin());
        }

        return keys;
    }

    YAML::Node Navigate(YAML::Node& base, const std::vector<std::string>& keys) {
        YAML::Node cur = base;
        for (const auto& key : keys) {
            if (cur[key]) {
                cur = cur[key];
            }
            else {
                std::cerr << "Key not found: " << key << std::endl;
                return YAML::Node();
            }
        }
        return cur;
    }

    bool IsJsonMimeType(const std::string& type) {
        return std::regex_match(type, r_application_json);
    }

    std::vector<std::string> GetSupportedMimeTypes(const YAML::Node& content) {
        std::vector<std::string> supported;
        for (const auto& it : content) {
            std::string key = it.first.as<std::string>();
            if (key == "application/x-www-form-urlencoded" || key == "multipart/form-data" || IsJsonMimeType(key)) {
                supported.push_back(key);
            }
        }
        return supported;
    }

    std::vector<std::string> GetMediaRanges(const YAML::Node& content) {
        std::vector<std::string> mediaRanges;
        for (const auto& it : content) {
            std::string mediaRange = it.first.as<std::string>();
            if (mediaRange.find('/') != std::string::npos) {
                mediaRanges.push_back(mediaRange);
            }
        }
        return mediaRanges;
    }

    std::vector<std::string> GetMediaTypes(const std::vector<std::string>& mediaRanges) {
        std::vector<std::string> mediaTypes;
        for (const auto& range : mediaRanges) {
            if (range.find('*') == std::string::npos) {
                mediaTypes.push_back(range);
            }
        }
        return mediaTypes;
    }
    std::string FixRef(const std::string& ref) {
        std::string fixedRef = ref;
        fixedRef = std::regex_replace(fixedRef, std::regex(R"(#\/components\/schemas\/)"), "#/definitions/");
        fixedRef = std::regex_replace(fixedRef, std::regex(R"(#\/components\/)"), "#/x-components/");
        return fixedRef;
    }
    void FixRefs(YAML::Node& obj) {
        if (obj.IsSequence()) {
            for (auto item : obj) {
                FixRefs(item);
            }
        }
        else if (obj.IsMap()) {
            for (auto it = obj.begin(); it != obj.end(); ++it) {
                std::string key = it->first.as<std::string>();
                if (key == "$ref") {
                    obj["$ref"] = FixRef(it->second.as<std::string>());
                }
                else {
                    FixRefs(it->second);
                }
            }
        }
    }
}