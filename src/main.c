/**
 * @file main.c
 * @author markbeep
 * @brief Example file that receives a number and one ups it in the specified channel.
 * @date 2022-07-24
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "commands.h"
#include <stdio.h>
#include <stdlib.h>
#include <utils/disco_logging.h>

static uint64_t on_ready_channel_id = 1004090600934617238UL; // channel to announce when the bot starts up
extern void count_on_ready(bot_client_t *bot);
extern void count_on_message(bot_client_t *bot, struct discord_message *message);
extern void ping_on_message(bot_client_t *bot, struct discord_message *msg);

void on_ready(bot_client_t *bot) {
    printf("====================================\n");
    printf("Successfully logged in\n");
    printf("Username:\t%s\n", bot->user->username);
    printf("User ID:\t%ju\n", bot->user->id);
    printf("====================================\n\n");
    fflush(stdout);

    // sends a message to a channel when the bot started up
    char str[30], dt[50];
    _d_datetime(dt); // gets the current datetime as a string
    sprintf(str, "<@%ju>", owner_id);
    struct discord_embed_footer footer = {
        .text = dt,
    };
    struct discord_embed embed = {
        .color = 0xadd8e6,
        .footer = &footer,
        .description = "Bot started up!",
    };
    struct discord_create_message message = {
        .embed = &embed,
    };
    discord_channel_send_message(bot, str, on_ready_channel_id, &message, NULL);

    // calls the on_ready function inside the count.c file
    count_on_ready(bot);
}

void on_message(bot_client_t *bot, struct discord_message *message) {
    count_on_message(bot, message);
    ping_on_message(bot, message);
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
    char *bot_token = getenv("DISCORD_TOKEN");
    if (!bot_token) {
        printf("No bot token found\n");
        exit(1);
    }

    // starts the bot. This function blocks
    discord_start_bot(&callbacks, bot_token, &conf);
    free(bot_token);
    return 0;
}
