#ifndef CUP_COUNT
#define CUP_COUNT

#include "../commands.h"
#include <sys/time.h>

static int count = 0;                                    // keeps track of what the last sent count is
static uint64_t bot_to_watch = 0UL;                      // what bot to send a count after
static uint64_t count_channel_id = 996746797236105236UL; // channel to count in
// static uint64_t count_channel_id = 948506487716737034UL; // debugging count channel

// used to count the efficiency of the counting channel
static struct timeval timer_short, timer_long; // short should be <1 min while long can be >1 min
static int counts_sent_short = 0, counts_sent_long = 0;

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
    discord_channel_send_message(bot, s, count_channel_id, NULL, false);
}

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

/**
 * @brief Returns the amount of s passed since the timer.
 *
 * @param timer
 * @return float
 */
static float get_time_passed(struct timeval timer) {
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec - timer.tv_sec + (now.tv_usec - timer.tv_usec) / 1000000.0f;
}

static void cmd_count(bot_client_t *bot, struct discord_message *message) {
    // calculate the efficiency
    float s_pass = get_time_passed(timer_long);
    if (s_pass <= 0.f)
        s_pass = 0.0001;
    float eff = counts_sent_long / s_pass;

    char s1[20];
    sprintf(s1, "`%d`", count);
    struct discord_embed_field f1 = {
        .name = "Count",
        .value = s1,
        ._inline = true,
    };
    char s2[50];
    sprintf(s2, "`%.2f` msgs/sec (%.0fs)", eff, s_pass);
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
    struct discord_create_message msg = {.embed = &embed};
    discord_channel_send_message(bot, NULL, message->channel_id, &msg, false);
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

void count_on_ready(bot_client_t *bot) {
    // checks what the last count is and sends the next count if needed
    bot_to_watch = get_watch_id();
    get_last_message_count(bot);
    // sets the timer to now
    gettimeofday(&timer_short, NULL);
    gettimeofday(&timer_long, NULL);
}

void count_on_message(bot_client_t *bot, struct discord_message *message) {
    // no content or no member (so not a guild message)
    if (!message->content || !message->member)
        return;

    // commands to call
    if (strncmp(message->content, "!count", 6) == 0) {
        cmd_count(bot, message);
        return;
    }
    if (message->user->id == owner_id && strncmp(message->content, "!watch", 6) == 0) {
        cmd_watch(bot, message);
        return;
    }

    // if it's not the count channel
    if (message->channel_id != count_channel_id)
        return;

    counts_sent_short++;
    counts_sent_long++;

    // if it's not the bot I'm watching nor the owner
    if (message->user->id != owner_id && message->user->id != bot_to_watch)
        return;
    int n = (int)strtol(message->content, NULL, 10);
    if (n >= count) { // ignore any number below our count
        count = n;
        send_count(bot, n + 1);
    }

    // update efficiency if short timer is above 60s
    if (get_time_passed(timer_short) > 45.0f) {
        timer_long = timer_short;
        counts_sent_long = counts_sent_short;
        counts_sent_short = 0;
        gettimeofday(&timer_short, NULL);
    }
}

#endif
