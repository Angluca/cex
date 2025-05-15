#include "cex.h"
#include <zip.h>

Exception
extract_zip(char* zip_path, char* extract_dir)
{
    Exc result = Error.runtime;
    struct zip* archive = NULL;
    char buffer[4096];

    e$ret(os.fs.mkdir(extract_dir));

    // Open the ZIP archive
    int err;
    e$except_null (archive = zip_open(zip_path, 0, &err)) {
        io.printf("\nError opening ZIP archive: %s\n", zip_strerror(archive));
        goto end;
    }

    i32 num_files = zip_get_num_entries(archive, 0);
    struct zip_stat stat;
    for (i32 i = 0; i < num_files; i++) {
        if (zip_stat_index(archive, i, 0, &stat) != 0) {
            fprintf(stderr, "Error reading file info at index %d\n", i);
            goto end;
        }

        // Skip directories (they're created when files are extracted)
        if (stat.name[strlen(stat.name) - 1] == '/') { continue; }

        // Build the full output path
        char output_path[1024];
        e$goto(str.sprintf(output_path, sizeof(output_path), "%s/%s", extract_dir, stat.name), end);
        e$goto(os.fs.mkpath(output_path), end);


        // Open the file in the archive
        struct zip_file* file = zip_fopen_index(archive, i, 0);
        if (!file) {
            fprintf(stderr, "Error opening file '%s' in archive\n", stat.name);
            goto end;
        }

        // Create the output file
        FILE* outfile = fopen(output_path, "wb");
        if (!outfile) {
            fprintf(stderr, "Error creating output file '%s'\n", output_path);
            zip_fclose(file);
            goto end;
        }

        // Extract the file contents
        zip_int64_t bytes_read;
        zip_int64_t total_read = 0;
        while (total_read < (zip_int64_t)stat.size) {
            bytes_read = zip_fread(file, buffer, sizeof(buffer));
            if (bytes_read < 0) {
                fprintf(stderr, "Error reading file '%s'\n", stat.name);
                goto end;
            }

            fwrite(buffer, 1, bytes_read, outfile);
            total_read += bytes_read;
        }

        // Close files
        fclose(outfile);
        zip_fclose(file);

        printf("Extracted: %s\n", output_path);
    }

    result = EOK;
end:
    // Close the archive
    zip_close(archive);
    return result;
}

Exception
cex_unzip(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    char* archive = "./build/sqlite-amalgamation-3490200.zip";
    e$assert(os.path.exists(archive) && "no archive, run ./cex app run cex_downloader");
    e$ret(extract_zip(archive, "./build/"));
    return EOK;
}
