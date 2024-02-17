#include <iostream>
#include <fstream>
#include <vector>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include "curl/curl.h"

// WARNING: Shit code incoming. Prepare your eyes

std::mutex mtx; // Mutex for protecting shared resources
std::condition_variable cv; // Condition variable for synchronization
std::queue<std::string> taskQueue; // Queue for holding tasks
bool stopFlag = false; // Flag to signal threads to stop

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

void performCurlRequest(const std::string& urlPrefix, std::ofstream& outFile) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize libcurl" << std::endl;
        return;
    }

    while (true) {
        std::string url;
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [&] { return !taskQueue.empty() || stopFlag; });

            if (stopFlag)
                break;

            url = taskQueue.front();
            taskQueue.pop();
        }

        url = urlPrefix + url;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/121.0.0.0 Safari/537.36");

        long response_code;
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
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
    }

    // Cleanup curl handle
    curl_easy_cleanup(curl);
}

int main() {
    std::string urlPrefix = "https://we.tl/t-";
    std::ofstream outFile("redirected_urls.txt", std::ios::app);

    int userThreads = 0;
    int numThreads = 5;

    // Number of threads you want to create
    std::vector<std::thread> threads;

    std::cout << "Enter amount of threads you want to use (Default: 5): ";
    std::cin >> userThreads;

    if (userThreads == 0)
        userThreads = 5;
    numThreads = userThreads;

    std::cout << "Program running. Don't worry, I will notify you when I found something. :)" << '\n';
    std::cout << "Amount of threads used: " << numThreads << '\n';
    // Start threads
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(performCurlRequest, std::ref(urlPrefix), std::ref(outFile));
    }

    // Keep feeding tasks until interrupted
    while (true) {
        std::string strUrl = generateUrlSlug(10);
        {
            std::lock_guard<std::mutex> lock(mtx);
            taskQueue.push(strUrl);
        }
        cv.notify_one(); // Notify one thread that a task is available
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Adjust sleep duration as needed
    }

    // Set stop flag and notify all threads to stop
    {
        std::lock_guard<std::mutex> lock(mtx);
        stopFlag = true;
    }
    cv.notify_all();

    // Join all threads
    for (auto& thread : threads) {
        thread.join();
    }

    // Close the file
    outFile.close();

    return 0;
}