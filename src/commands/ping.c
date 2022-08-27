#include <discord/disco.h>

static long total_ping = 0;
static int pings_added = 0;
static int messages_until_measure = 0;
// to calculate the median ping
static int medians_added = 0;
static int pings[100];

static void
cmd_ping(bot_client_t *bot, struct discord_message *message);
static void pong_callback(bot_client_t *bot, struct discord_message *msg, void *data);
void store_ping_callback(bot_client_t *bot, struct discord_message *msg, void *w);
extern float get_time_passed(struct timeval timer); // defined in count.c

void ping_on_message(bot_client_t *bot, struct discord_message *msg) {
    if (!msg->user || msg->user->bot || !msg->content)
        return;
    if (strncmp(msg->content, "-ping", 6) == 0) {
        cmd_ping(bot, msg);
        return;
    }
}

static void cmd_ping(bot_client_t *bot, struct discord_message *message) {
    struct timeval *start = malloc(sizeof(struct timeval));
    gettimeofday(start, NULL);

    // creates the embed
    struct discord_embed embed = {
        .description = "Pinging...",
        .color = 0x8000,
    };
    struct discord_create_message send_message = {
        .embed = &embed,
    };

    // creates the callback, because we want to receive the message
    struct discord_message_cb *cb = (struct discord_message_cb *)malloc(sizeof(struct discord_message_cb));
    cb->cb = &pong_callback;
    cb->data = start;
    discord_channel_send_message(bot, NULL, message->channel_id, &send_message, cb);
}

static void pong_callback(bot_client_t *bot, struct discord_message *msg, void *data) {
    // gets the time passed
    struct timeval stop, *start = (struct timeval *)data;
    gettimeofday(&stop, NULL);
    char time_passed[120];
    long delta = (stop.tv_sec - start->tv_sec) * 1000 + (stop.tv_usec - start->tv_usec) / 1000;
    long avg_ping = pings_added > 0 ? total_ping / pings_added : 0;
    int median_ping = medians_added > 0 ? pings[medians_added / 2] : 0;
    sprintf(time_passed,
            "Ping: `%ld` ms\nHeartbeat: `%ld` ms\nAvg Ping: `%ld` ms (n = %d)\nMed Ping: `%d` ms (m = %d)",
            delta,
            bot->heartbeat_latency,
            avg_ping,
            pings_added,
            median_ping,
            medians_added);

    struct discord_embed embed = {
        .description = time_passed,
        .color = 0x8000,
    };
    struct discord_create_message send_message = {
        .embed = &embed,
    };
    // edits the message right after the message has been sent to check the latency
    discord_channel_edit_message(bot, NULL, msg->channel_id, msg->id, &send_message);
    // don't forget to destroy the received message in the end to avoid memory leaks
    discord_destroy_message(msg);
}

void store_ping_callback(bot_client_t *bot, struct discord_message *msg, void *w) {
    (void)bot;
    struct timeval t0 = *((struct timeval *)w);
    if (messages_until_measure <= 0) {
        long ping = get_time_passed(t0);
        // AVERAGE
        if (ping < 1000) { // ignore outliers above 1s
            total_ping += ping;
            pings_added++;
            messages_until_measure = 100;
        }
        // MEDIAN
        // if we have 100 elements, remove 40 and keep the middle 60
        if (medians_added == 100) {
            for (int i = 0; i < 60; i++) {
                pings[i] = pings[i + 20];
            }
            medians_added = 60;
        }
        // elements are added using bubble sort
        for (int i = 0; i < medians_added - 1; i++) {
            if (pings[i] > ping) {
                int tmp = pings[i];
                pings[i] = ping;
                ping = tmp;
            }
        }
        pings[medians_added++] = ping;

    } else {
        messages_until_measure--;
    }

    free(w);
    discord_destroy_message(msg);
}
