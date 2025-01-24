#include "web_crawler.h"

void verify_database(Database *db) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT url, title FROM pages";

    if (sqlite3_prepare_v2(db->db, sql, -1, &stmt, 0) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            printf("URL: %s\nTitle: %s\n\n",
                   sqlite3_column_text(stmt, 0),
                   sqlite3_column_text(stmt, 1));
        }
    }
    sqlite3_finalize(stmt);
}

int db_init(Database *database)
{
    int rc = sqlite3_open("mydb.db", &database->db);
    database->err_msg = 0;

    rc = sqlite3_exec(database->db, "PRAGMA journal_mode=WAL", NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to set WAL mode: %s\n", sqlite3_errmsg(database->db));
        return rc;
    }

    rc = sqlite3_exec(database->db, "PRAGMA journal_mode=WAL", NULL, NULL, NULL);
    rc = sqlite3_exec(database->db, "PRAGMA synchronous=NORMAL", NULL, NULL, NULL);
    rc = sqlite3_exec(database->db, "BEGIN TRANSACTION", NULL, NULL, NULL);

    const char *sql = "CREATE TABLE IF NOT EXISTS pages ("
                "url TEXT PRIMARY KEY,"
                "title TEXT,"
                "content TEXT,"
                "last_crawled INTEGER,"
                "page_rank REAL DEFAULT 1.0,"
                "outbound_links TEXT"
                ")";

    rc = sqlite3_exec(database->db, sql, 0, 0, &database->err_msg);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL Error: %s\n", sqlite3_errmsg(database->db));
        sqlite3_free(database->err_msg);
        sqlite3_close(database->db);

        return sqlite3_errcode(database->db);
    }


    return SQLITE_OK;
}

void *thread_function(void *arg)
{
    ThreadArgs *args = (ThreadArgs *)arg;
    CURL *curl = curl_easy_init();
    if (curl == NULL) {
        fprintf(stderr, "Libcurl Initialization Failed");
        pthread_exit(NULL);
    }

    // Tidydoc
    TidyDoc thread_tidy_doc = tidyCreate();
    TidyBuffer errbuf = {0};

    // Config
    tidyOptSetBool(thread_tidy_doc, TidyQuiet, yes);
    tidyOptSetBool(thread_tidy_doc, TidyShowWarnings, no);
    tidySetErrorBuffer(thread_tidy_doc, &errbuf);

    while (true) {
        pthread_mutex_lock(&queue_mutex);

        if (queue_empty(args->queue)) {
            pthread_mutex_unlock(&queue_mutex);
            break;
        }

        char *url = queue_dequeue(args->queue);
        pthread_mutex_unlock(&queue_mutex);

        if (args->c_depth->current_depth > MAXIMUM_DEPTH) {
            free(url);
            break;
        }

        pthread_mutex_lock(&set_mutex);
        if (hash_set_contains(args->set, url)) {
            pthread_mutex_unlock(&set_mutex);
            free(url);
            continue;
        }

        hash_set_add(args->set, url);
        pthread_mutex_unlock(&set_mutex);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_chunk);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) args->response);

        const CURLcode result = curl_easy_perform(curl);
        if (result == CURLE_OK) {
            tidyParseString(thread_tidy_doc, args->response->string);

            pthread_mutex_lock(&queue_mutex);
            pthread_mutex_lock(&set_mutex);

            parse_html_data(args->queue, args->set, tidyGetRoot(thread_tidy_doc), args->c_depth, args->database, thread_tidy_doc, url);

            pthread_mutex_unlock(&set_mutex);
            pthread_mutex_unlock(&queue_mutex);
        }

        free(args->response->string);
        args->response->string = malloc(1);
        args->response->size = 0;

        pthread_mutex_lock(&queue_mutex);
        args->c_depth->urls_at_current_depth--;

        if (args->c_depth->urls_at_current_depth == 0) {
            args->c_depth->current_depth++;
            args->c_depth->urls_at_current_depth = args->c_depth->urls_at_next_depth;
            args->c_depth->urls_at_next_depth = 0;
        }
        pthread_mutex_unlock(&queue_mutex);
    }

    tidyBufFree(&errbuf);
    tidyRelease(thread_tidy_doc);
    curl_easy_cleanup(curl);
    return NULL;
}

double run_crawler(Set *set, Queue *queue)
{
    Database database = {0};
    database.err_msg =0;
    const int db_status = db_init(&database);
    if (db_status != SQLITE_OK) {
        fprintf(stderr, "Database initialization failed: %d\n", db_status);
        return -1;
    }

    CrawlerDepthData c_depth = {0, 1, 0};
    queue_enqueue(queue, "https://www.google.com");

    pthread_cond_signal(&queue_cond);

    const time_t start = time(NULL);

    pthread_t threads[NUM_OF_THREADS];
    ThreadArgs thread_args[NUM_OF_THREADS];

    for (int i = 0; i < NUM_OF_THREADS; i++) {
        thread_args[i].set = set;
        thread_args[i].queue = queue;
        thread_args[i].c_depth = &c_depth;

        thread_args[i].response = malloc(sizeof(Response));
        thread_args[i].response->string = malloc(1);
        thread_args[i].response->size = 0;
        thread_args[i].database = &database;

        pthread_create(&threads[i], NULL, thread_function, (void *)&thread_args[i]);
        printf("created thread %d", i);
    }

    for (int i = 0; i < NUM_OF_THREADS; i++) {
        pthread_join(threads[i], NULL);
        printf("Joined thread %d\n", i);


        free(thread_args[i].response->string);
        free(thread_args[i].response);
    }

    verify_database(&database);
    sqlite3_exec(database.db, "COMMIT", NULL, NULL, NULL);
    if (database.db) {
        sqlite3_close(database.db);
        database.db = NULL;
    }

    const time_t end = time(NULL);
    const double total_search_time = difftime(end, start);

    return total_search_time;
}

static size_t write_chunk(void *data, size_t size, size_t nmemb, void *userdata)
{
    const size_t real_size = size * nmemb;

    Response *response = (Response *) userdata;

    char *ptr = realloc(response->string, response->size + real_size + 1);

    if (ptr == NULL) {
        fprintf(stderr, "Write Data Chunk Failed");
        return CURL_WRITEFUNC_ERROR;
    }

    response->string = ptr;
    memcpy(&(response->string[response->size]), data, real_size);
    response->size += real_size;
    response->string[response->size] = '\0';

    return real_size;
}

static void parse_html_data(Queue *queue, Set *set, TidyNode tnode, CrawlerDepthData *c_depth, Database *db, TidyDoc doc, const char *current_url) {
    if (c_depth->current_depth >= MAXIMUM_DEPTH) return;

    TidyBuffer content = {0};
    TidyBuffer title_buf = {0};
    char *title = NULL;

    for (TidyNode child = tidyGetChild(tnode); child; child = tidyGetNext(child)) {
        const TidyTagId tag_id = tidyNodeGetId(child);

        if (tag_id == TidyTag_TITLE) {
            printf("Found title tag\n");
            tidyNodeGetText(doc, child, &title_buf);
            printf("Raw title: %s\n", (char*)title_buf.bp);
            title = strdup((char*)title_buf.bp);
        }

        if (tag_id == TidyTag_P) {
            TidyBuffer para_buf = {0};
            tidyNodeGetText(doc, child, &para_buf);
            tidyBufAppend(&content, para_buf.bp, para_buf.size);
            tidyBufFree(&para_buf);
        }

        if (tag_id == TidyTag_A) {
            const TidyAttr href_attr = tidyAttrGetById(child, TidyAttr_HREF);
            if (href_attr) {
                const char *url = tidyAttrValue(href_attr);
                if (url && strstr(url, "http") != NULL && !hash_set_contains(set, url)) {
                    char *url_copy = malloc(strlen(url) + 1);
                    strcpy(url_copy, url);
                    queue_enqueue(queue, url_copy);
                    c_depth->urls_at_next_depth++;
                }
            }
        }

        parse_html_data(queue, set, child, c_depth, db, doc, current_url);
    }

    if (content.size > 0) {
        store_page(db, current_url, title ? title : "", (char*)content.bp);
    }

    tidyBufFree(&content);
}

int store_page(Database *db, const char *url, const char *title, const char *content) {
    printf("Storing page: %s\n", url);
    const char *sql = "INSERT OR REPLACE INTO pages (url, title, content, last_crawled) VALUES (?, ?, ?, ?)";
    sqlite3_stmt *stmt;

    int rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL prepare failed: %s\n", sqlite3_errmsg(db->db));
        return rc;
    }

    sqlite3_bind_text(stmt, 1, url, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, title, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, content, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 4, time(NULL));

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_exec(db->db, "COMMIT", NULL, NULL, NULL);

    return (rc == SQLITE_DONE) ? SQLITE_OK : rc;
}
