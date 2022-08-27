#include "../commands.h"
#include <sys/time.h>

static int count = 0;                                    // keeps track of what the last sent count is
static uint64_t bot_to_watch = 0UL;                      // what bot to send a count after
static uint64_t count_channel_id = 996746797236105236UL; // channel to count in
// static uint64_t count_channel_id = 948506487716737034UL; // debugging count channel

// used to count the efficiency of the counting channel
static int counts_sent = 0, counts_sent_prev = 0;  // how many count messages were sent
static double background_task_loop_seconds = 60.0; // repeat the background task every X seconds
static bool background_task_started = false;       // prevents multiple background tasks from starting
double eff_points[1000];                           // efficiency over time
const size_t eff_array_size = 1000;                // size of the efficiency array
size_t eff_n = 0;                                  // total eff added

// COMMANDS
static void cmd_watch(bot_client_t *bot, struct discord_message *message);
static void cmd_count(bot_client_t *bot, struct discord_message *message);
// CALLBACKS
static void last_message_cb(bot_client_t *bot, size_t size, struct discord_message **arr, void *data);
extern void store_ping_callback(bot_client_t *bot, struct discord_message *msg, void *w);
// HELPERS
static uint64_t get_watch_id(void);
static void set_watch_id(uint64_t id);
static void send_count(bot_client_t *bot, int n);
static void get_last_message_count(bot_client_t *bot);
static void background_task(void *w, CURL *handle);
// EXTERN HELPERS
void draw_efficiency_graph(char *fp, double **points, size_t n, char *title, char *x_label, char *y_label);

void count_on_ready(bot_client_t *bot) {
    // checks what the last count is and sends the next count if needed
    bot_to_watch = get_watch_id();
    get_last_message_count(bot);

    for (size_t i = 0; i < eff_array_size; i++)
        eff_points[i] = 0;
    // calling the background task starts it
    if (!background_task_started) {
        background_task_started = true;
        background_task((void *)bot, NULL);
    }
}

void count_on_message(bot_client_t *bot, struct discord_message *message) {
    // no content or no member (so not a guild message)
    if (!message->content || !message->member || !message->user)
        return;

    // commands to call (bots are ignored)
    if (!message->user->bot) {
        if (strncmp(message->content, "-count", 6) == 0) {
            cmd_count(bot, message);
            return;
        }
        if (message->user->id == owner_id && strncmp(message->content, "-watch", 6) == 0) {
            cmd_watch(bot, message);
            return;
        }
    }

    // if it's not the count channel
    if (message->channel_id != count_channel_id)
        return;

    counts_sent++;

    // if it's not the bot I'm watching nor the owner
    if (message->user->id != owner_id && message->user->id != bot_to_watch)
        return;
    int n = (int)strtol(message->content, NULL, 10);
    if (n >= count) { // ignore any number below our count
        count = n;
        send_count(bot, n + 1);
    }
}

/* #####################################

    COMMANDS

   ##################################### */

static void cmd_count(bot_client_t *bot, struct discord_message *message) {
    float eff = 1.0 * counts_sent_prev / background_task_loop_seconds;

    char s1[20];
    sprintf(s1, "`%d`", count);
    struct discord_embed_field f1 = {
        .name = "Count",
        .value = s1,
        ._inline = true,
    };
    char s2[50];
    sprintf(s2, "`%.2f` msgs/sec (%.0fs)", eff, background_task_loop_seconds);
    struct discord_embed_field f2 = {
        .name = "Efficiency",
        .value = s2,
        ._inline = true,
    };
    struct discord_embed_field *fs1[] = {&f1, &f2};
    struct discord_embed embed = {
        .color = 0xadd8e6,
        .fields = fs1,
        .fields_count = 2,
    };

    // reorders the points so that they are ordered oldest to newest
    double **points = malloc(sizeof(double) * eff_array_size);
    for (size_t i = 0; i < eff_array_size; i++) {
        points[i] = malloc(sizeof(double) * 2);
        points[i][0] = i;
        points[i][1] = eff_points[(i + eff_n) % eff_array_size];
    }

    // generates the image and adds it to the message
    draw_efficiency_graph("tmp_image.png", points, eff_array_size, "Count Efficiency", "minutes", "msgs / sec");
    struct discord_attachment attachment = {
        .filename = "tmp_image.png",
    };
    struct discord_attachment *attachments[] = {&attachment};
    struct discord_create_message msg = {
        .embed = &embed,
        .attachments = attachments,
        .attachments_count = 1,
    };

    discord_channel_send_message(bot, NULL, message->channel_id, &msg, false);

    // CLEANUP
    for (size_t i = 0; i < eff_array_size; i++) {
        free(points[i]);
    }
    free(points);
}

static void cmd_watch(bot_client_t *bot, struct discord_message *message) {
    struct discord_embed embed = {0};
    struct discord_create_message msg = {.embed = &embed};
    embed.color = 0xadd8e6;
    char s[50];
    embed.description = s;

    // parse input
    char *token, *rest = message->content;
    token = strtok_r(rest, " ", &rest);
    token = strtok_r(rest, " ", &rest);
    if (token) {
        bot_to_watch = (uint64_t)strtoull(token, NULL, 10);
        set_watch_id(bot_to_watch);
    }
    if (bot_to_watch == 0) {
        sprintf(s, "Watch = nobody <:sadge:851469686578741298>");
    } else {
        sprintf(s, "Watch = <@%ju>", bot_to_watch);
    }
    discord_channel_send_message(bot, NULL, message->channel_id, &msg, NULL);
}

/* #####################################

    CALLBACKS

   ##################################### */

static void last_message_cb(bot_client_t *bot, size_t size, struct discord_message **arr, void *data) {
    (void)data;
    int res = -1;
    if (size > 0 && arr[0] && arr[0]->content) {
        res = (int)strtol(arr[0]->content, NULL, 10);
        free(arr);
    }
    count = res;
    if (count >= 0)
        send_count(bot, count + 1);
}

/* #####################################

    HELPERS

   ##################################### */

/**
 * @brief Get the watch id from the watch file
 *
 * @return uint64_t
 */
static uint64_t get_watch_id(void) {
    FILE *fp = fopen("../data/watch.txt", "r");
    if (!fp)
        return 0;
    uint64_t id = -1;
    fscanf(fp, "%ju", &id);
    fclose(fp);
    return id;
}

/**
 * @brief Set the watch id in the watch file
 *
 * @param id
 */
static void set_watch_id(uint64_t id) {
    FILE *fp = fopen("../data/watch.txt", "w");
    char str[22];
    sprintf(str, "%ju", id);
    fwrite(str, 1, strnlen(str, 22), fp);
    fclose(fp);
}

/**
 * @brief Sends the specified number in the count channel
 *
 * @param bot
 * @param n
 */
static void send_count(bot_client_t *bot, int n) {
    char s[20];
    sprintf(s, "%d", n);

    struct discord_message_cb *cb = (struct discord_message_cb *)malloc(sizeof(struct discord_message_cb));
    cb->cb = &store_ping_callback;
    struct timeval *start = malloc(sizeof(struct timeval));
    gettimeofday(start, NULL);
    cb->data = start;
    discord_channel_send_message(bot, s, count_channel_id, NULL, cb);
}

/**
 * @brief Gets the last sent integer in the channel.
 *
 * @param bot
 */
static void get_last_message_count(bot_client_t *bot) {

    struct discord_multiple_message_cb *cb = (struct discord_multiple_message_cb *)malloc(sizeof(struct discord_multiple_message_cb));
    cb->data = NULL;
    cb->cb = &last_message_cb;
    discord_get_messages(bot, count_channel_id, 1, 0, 0, 0, cb);
}

static void background_task(void *w, CURL *handle) {
    bot_client_t *bot = (bot_client_t *)w;
    (void)handle;

    // updates the sent count
    counts_sent_prev = counts_sent;
    counts_sent = 0;

    // add to efficiency array
    double eff = counts_sent_prev / background_task_loop_seconds;
    eff_n++;
    eff_points[eff_n % eff_array_size] = eff;

    // to start the timer for when the background task should be repeated
    static struct timeval future_timer;
    gettimeofday(&future_timer, NULL);
    future_timer.tv_sec += background_task_loop_seconds;
    t_pool_add_work(bot->thread_pool, &background_task, w, future_timer);
}
