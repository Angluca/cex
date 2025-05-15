#define CEX_IMPLEMENTATION
#include "cex.h"
#include <curl/curl.h>
#include <stdio.h>
#include <string.h>

size_t
write_data(void* ptr, size_t size, size_t nmemb, FILE* stream)
{
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

Exception
download()
{
    Exc result = EOK;
    char* sqlite_url = "https://www.sqlite.org/2025/sqlite-amalgamation-3490200.zip";
    char* out_file = "./build/sqlite-amalgamation-3490200.zip";
    CURL* curl = curl_easy_init();
    if (curl) {
        FILE* fp;
        log$info("Downloading: %s\n", sqlite_url);
        e$ret(io.fopen(&fp, out_file, "wb"));
        curl_easy_setopt(curl, CURLOPT_URL, sqlite_url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        e$except_true (curl_easy_perform(curl)) { result = Error.runtime; }
        curl_easy_cleanup(curl);
        io.fclose(&fp);
        if (result != EOK) {
            e$ret(os.fs.remove(out_file)); // don't keep bad file!
        } else {
            os_fs_stat_s stat = os.fs.stat(out_file);
            e$assert(stat.is_valid);
            log$info("Done %s, size: %lu\n", out_file, stat.size);
        }

    } else {
        result = Error.os;
    }
    e$assert(result != EOK || os.path.exists(out_file));
    return result;
}

int
main(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    // This will emit traceback
    e$except (err, download()) { return 1; }
    return 0;
}
