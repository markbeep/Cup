#ifndef CUP_COMMANDS
#define CUP_COMMANDS
#include <discord/disco.h>

static uint64_t owner_id = 205704051856244736UL;
void count_on_ready(bot_client_t *bot);
void count_on_message(bot_client_t *bot, struct discord_message *message);

#endif
