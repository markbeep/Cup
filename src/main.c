/**
 * @file main.c
 * @author markbeep
 * @brief Example file that receives a number and one ups it in the specified channel.
 * @version 0.1
 * @date 2022-07-24
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <discord/disco.h>
#include <discord/message.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils/disco_logging.h>

char *bot_token; // will be assigned in the main function

static int count = 0; // keeps track of what the last sent count is
static uint64_t bot_to_watch = 0UL;
static uint64_t count_channel_id = 996746797236105236UL;
static uint64_t on_ready_channel_id = 1004090600934617238UL;
static uint64_t owner_id = 205704051856244736UL;

static uint64_t get_watch_id(void) {
    FILE *fp = fopen("../data/watch.txt", "r");
    if (!fp)
        return 0;
    uint64_t id = -1;
    fscanf(fp, "%ju", &id);
    fclose(fp);
    return id;
}

static void set_watch_id(uint64_t id) {
    FILE *fp = fopen("../data/watch.txt", "w");
    char str[22];
    sprintf(str, "%ju", id);
    fwrite(str, 1, strnlen(str, 22), fp);
    fclose(fp);
}

static void send_count(bot_client_t *bot, int n) {
    char s[20];
    sprintf(s, "%d", n);
    discord_channel_send_message(bot, s, count_channel_id, NULL, false);
}

/**
 * @brief Gets the last sent integer in the channel.
 *
 * @param bot
 */
static bool get_last_message_count(bot_client_t *bot) {
    struct discord_message **arr;
    int arr_size = discord_get_messages(bot, count_channel_id, &arr, 1, 0, 0, 0);
    int res = -1;
    bool send_count = false;
    if (arr_size > 0 && arr[0] && arr[0]->content) {
        res = (int)strtol(arr[0]->content, NULL, 10);
        if (arr[0]->user->id == bot_to_watch)
            send_count = true;
        free(arr);
    }
    count = res;
    return send_count;
}

void on_ready(bot_client_t *bot) {
    printf("====================================\n");
    printf("Successfully logged in\n");
    printf("Username:\t%s\n", bot->user->username);
    printf("User ID:\t%ju\n", bot->user->id);
    printf("====================================\n\n");
    fflush(stdout);
    char str[50];
    sprintf(str, "<@%ju>: Bot started up", owner_id);
    discord_channel_send_message(bot, str, on_ready_channel_id, NULL, false);

    // checks what the last count is and sends the next count if needed
    bot_to_watch = get_watch_id();
    if (get_last_message_count(bot))
        send_count(bot, count + 1);
}

static void cmd_count(bot_client_t *bot, struct discord_message *message) {
    struct discord_embed embed = {0};
    struct discord_create_message msg = {.embed = &embed};
    embed.color = 0xadd8e6;
    char s[40];
    embed.description = s;
    sprintf(s, "Count = `%d`", count);
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
    discord_channel_send_message(bot, NULL, message->channel_id, &msg, false);
}

void on_message(bot_client_t *bot, struct discord_message *message) {
    // no content or no member (so not a guild message)
    if (!message->content || !message->member)
        goto cleanup;

    // commands to call
    if (strncmp(message->content, "!count", 6) == 0) {
        cmd_count(bot, message);
        goto cleanup;
    }
    if (message->user->id == owner_id && strncmp(message->content, "!watch", 6) == 0) {
        cmd_watch(bot, message);
        goto cleanup;
    }

    // if it's not the bot I'm watching (nor the owner) or not the count channel
    if ((message->user->id != owner_id && message->user->id != bot_to_watch) ||
        message->channel_id != count_channel_id)
        goto cleanup;
    int n = (int)strtol(message->content, NULL, 10);
    if (n >= count) { // ignore any number below our count
        count = n;
        send_count(bot, n + 1);
    }

cleanup:
    discord_destroy_message(message);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    // init to 0. Without this some errors could show up
    discord_event_callbacks_t callbacks = {0};
    callbacks.on_ready = &on_ready;
    callbacks.on_message = &on_message;

    // optionally sets the allowed messages in the cache
    struct discord_config conf = {0};
    conf.message_cache_size = 2;

    // Make errors show up in the console
    d_set_log_level(D_LOG_ERR | D_LOG_NOTICE);

    // gets the token from the .env file
    bot_token = getenv("DISCORD_TOKEN");
    if (!bot_token) {
        printf("No bot token found\n");
        exit(1);
    }

    // starts the bot. This function blocks
    discord_start_bot(&callbacks, bot_token, &conf);
    free(bot_token);
    return 0;
}
