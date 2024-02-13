#include <iostream>
#include <fstream>
#include "curl/curl.h"

// Function to generate a random URL slug
std::string generateUrlSlug(const int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    std::string tmp_s;
    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i) {
        tmp_s += alphanum[std::rand() % (sizeof(alphanum) - 1)];
    }
    return tmp_s;
}

int main() {
    CURL *curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize libcurl" << std::endl;
        return 1;
    }

    std::cout << "Program running. Don't worry, I will notify you when I found something. :)" << '\n';

    std::string urlPrefix = "https://we.tl/t-";
    long response_code;
    std::ofstream outFile("redirected_urls.txt", std::ios::app);

    while (true) {
        std::string strUrl = generateUrlSlug(10);
        std::string url = urlPrefix + strUrl;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/121.0.0.0 Safari/537.36");

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            continue; // Continue to the next iteration
        }

        res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (res == CURLE_OK) {
            if ((response_code / 100) != 3) {
                fprintf(stderr, "Not a redirect (HTTP status code: %ld)\n", response_code);
                outFile << response_code << std::endl;
            } else {
                char *location;
                res = curl_easy_getinfo(curl, CURLINFO_REDIRECT_URL, &location);
                if ((res == CURLE_OK) && location) {
                    if (strcmp(location, "https://www.wetransfer.com/redirect/error") != 0) {
                        std::cout << "WORKING LINK FOUND: " << location << '\n';
                        outFile << location << std::endl;
                    }
                }
            }
        } else {
            fprintf(stderr, "Failed to get response code\n");
        }
    }

    // Close the file
    outFile.close();

    // Cleanup curl handle
    curl_easy_cleanup(curl);

    return 0;
}
