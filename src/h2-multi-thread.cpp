#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <curl/curl.h>

// 下载回调函数
size_t write_callback(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    return fwrite(ptr, size, nmemb, stream);
}

// 下载函数
void download(const std::string &url)
{
    CURL *curl = curl_easy_init();
    if (curl)
    {
        CURLcode res;
        FILE *fp = fopen(("downloaded_" + url.substr(url.find_last_of('/') + 1)).c_str(), "wb");
        if (fp)
        {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
            // 忽略证书
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

            res = curl_easy_perform(curl);
            if (res != CURLE_OK)
                std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            fclose(fp);
        }
        curl_easy_cleanup(curl);
    }
}

// https://192.168.110.251:11002/hello.txt
// download("https://192.168.110.251:11002/hq");

int main()
{
    std::vector<std::string> urls = {
        "https://192.168.110.251:11002/hq",
        "https://192.168.110.251:11002/proxygen_static",
        "https://192.168.110.251:11002/proxygen_push",
        "https://192.168.110.251:11002/proxygen_echo"};

    constexpr int num_threads = 4;
    std::thread threads[4];

    // 创建多个线程
    for (int i = 0; i < num_threads; ++i)
    {
        threads[i] = std::thread(download, urls[i]);
        threads[i].join();
    }

    return 0;
}
