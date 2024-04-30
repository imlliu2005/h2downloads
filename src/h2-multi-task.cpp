/* <DESC>
 * Multiplexed HTTP/2 downloads over a single connection
 * </DESC>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <iostream>
#include <vector>

/* somewhat unix-specific */
// #include <sys/time.h>
// #include <unistd.h>

/* curl stuff */
#include <curl/curl.h>
#include <curl/mprintf.h>

#ifndef CURLPIPE_MULTIPLEX
/* This little trick makes sure that we do not enable pipelining for libcurls
   old enough to not have this symbol. It is _not_ defined to zero in a recent
   libcurl header. */
#define CURLPIPE_MULTIPLEX 0
#endif

struct transfer
{
    CURL *curl;
    unsigned int num;
    FILE *out;
};

#define NUM_HANDLES 1000

static void dump(const char *text, unsigned int num, unsigned char *ptr, size_t size,
                 char nohex)
{
    size_t i;
    size_t c;

    unsigned int width = 0x10;

    if (nohex)
        /* without the hex output, we can fit more on screen */
        width = 0x40;

    fprintf(stderr, "%u %s, %lu bytes (0x%lx)\n",
            num, text, (unsigned long)size, (unsigned long)size);

    for (i = 0; i < size; i += width)
    {

        fprintf(stderr, "%4.4lx: ", (unsigned long)i);

        if (!nohex)
        {
            /* hex not disabled, show it */
            for (c = 0; c < width; c++)
                if (i + c < size)
                    fprintf(stderr, "%02x ", ptr[i + c]);
                else
                    fputs("   ", stderr);
        }

        for (c = 0; (c < width) && (i + c < size); c++)
        {
            /* check for 0D0A; if found, skip past and start a new line of output */
            if (nohex && (i + c + 1 < size) && ptr[i + c] == 0x0D &&
                ptr[i + c + 1] == 0x0A)
            {
                i += (c + 2 - width);
                break;
            }
            fprintf(stderr, "%c",
                    (ptr[i + c] >= 0x20) && (ptr[i + c] < 0x80) ? ptr[i + c] : '.');
            /* check again for 0D0A, to avoid an extra \n if it's at width */
            if (nohex && (i + c + 2 < size) && ptr[i + c + 1] == 0x0D &&
                ptr[i + c + 2] == 0x0A)
            {
                i += (c + 3 - width);
                break;
            }
        }
        fputc('\n', stderr); /* newline */
    }
}

static int my_trace(CURL *handle, curl_infotype type,
                    char *data, size_t size,
                    void *userp)
{
    const char *text;
    struct transfer *t = (struct transfer *)userp;
    unsigned int num = t->num;
    (void)handle; /* prevent compiler warning */

    switch (type)
    {
    case CURLINFO_TEXT:
        fprintf(stderr, "== %u Info: %s", num, data);
        return 0;
    case CURLINFO_HEADER_OUT:
        text = "=> Send header";
        break;
    case CURLINFO_DATA_OUT:
        text = "=> Send data";
        break;
    case CURLINFO_SSL_DATA_OUT:
        text = "=> Send SSL data";
        break;
    case CURLINFO_HEADER_IN:
        text = "<= Recv header";
        break;
    case CURLINFO_DATA_IN:
        text = "<= Recv data";
        break;
    case CURLINFO_SSL_DATA_IN:
        text = "<= Recv SSL data";
        break;
    default: /* in case a new one is introduced to shock us */
        return 0;
    }

    dump(text, num, (unsigned char *)data, size, 1);
    return 0;
}

// url = "https://192.168.110.251:11002/hello.txt"
// url = "https://192.168.110.251:5443/hq"
static void setup(struct transfer *t, int num, std::string url)
{
    char filename[128];
    CURL *hnd;

    hnd = t->curl = curl_easy_init();

    curl_msnprintf(filename, 128, "dl-%d", num);

    t->out = fopen(filename, "wb");

    if (!t->out)
    {
        fprintf(stderr, "error: could not open file %s for writing: %s\n",
                filename, strerror(errno));
        exit(1);
    }

    /* write to this file */
    curl_easy_setopt(hnd, CURLOPT_WRITEDATA, t->out);

    /* set the URL */
    std::cout << "url -----> " << url << std::endl;
    curl_easy_setopt(hnd, CURLOPT_URL, url.c_str());

    /* please be verbose */
    curl_easy_setopt(hnd, CURLOPT_VERBOSE, 1L);
    // curl_easy_setopt(hnd, CURLOPT_DEBUGFUNCTION, my_trace);
    curl_easy_setopt(hnd, CURLOPT_DEBUGDATA, t);

    // 忽略证书
    curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYPEER, 0L);

    /* enlarge the receive buffer for potentially higher transfer speeds */
    curl_easy_setopt(hnd, CURLOPT_BUFFERSIZE, 100000L);

    /* HTTP/2 please */
    curl_easy_setopt(hnd, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);

#if (CURLPIPE_MULTIPLEX > 0)
    /* wait for pipe connection to confirm */
    std::cout << "CURLPIPE_MULTIPLEX -----> " << CURLPIPE_MULTIPLEX << std::endl;
    curl_easy_setopt(hnd, CURLOPT_PIPEWAIT, 1L);
#endif
}

/*
 * Download many transfers over HTTP/2, using the same connection!
 */
int main(int argc, char **argv)
{
    struct transfer trans[NUM_HANDLES];
    CURLM *multi_handle;
    int i;
    int still_running = 0; /* keep number of running handles */
    int num_transfers;
    if (argc > 1)
    {
        /* if given a number, do that many transfers */
        num_transfers = atoi(argv[1]);
        if ((num_transfers < 1) || (num_transfers > NUM_HANDLES))
        {
            num_transfers = 3; /* a suitable low default */
        }
    }
    else
    {
        num_transfers = 3; /* suitable default */
    }

    /* init a multi stack */
    multi_handle = curl_multi_init();
    std::vector<std::string> urls = {
        "https://192.168.110.251:11002/hq",
        "https://192.168.110.251:11002/proxygen_static",
        "https://192.168.110.251:11002/proxygen_echo"};

    for (i = 0; i < num_transfers; i++)
    {
        setup(&trans[i], i, urls[i]);
        /* add the individual transfer */
        curl_multi_add_handle(multi_handle, trans[i].curl);
    }

    // CURLMOPT_PIPELINING 从 cURL 7.43.0 开始，该值是位掩码，如果可能，您还可以传递 2 以尝试在已有的 HTTP/2 连接上复用多路新传输。传递 3 指示 cURL 请求彼此独立的管道和多路复用
    curl_multi_setopt(multi_handle, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX);

    do
    {
        CURLMcode mc = curl_multi_perform(multi_handle, &still_running);

        if (still_running)
        { /* wait for activity, timeout or "nothing" */
            mc = curl_multi_poll(multi_handle, NULL, 0, 1000, NULL);
        }

        if (mc)
        {
            std::cout << "mc -----> " << mc << std::endl;
            break;
        }

    } while (still_running);

    for (i = 0; i < num_transfers; i++)
    {
        curl_multi_remove_handle(multi_handle, trans[i].curl);
        curl_easy_cleanup(trans[i].curl);
    }

    curl_multi_cleanup(multi_handle);

    return 0;
}
