#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <xtables.h>
#include <limits.h> /* INT_MAX in ip_tables.h */
#include <linux/netfilter_ipv4/ip_tables.h>

static void TDROP_help(void)
{
	printf("TDROP, no options provided\n");
}

static void TDROP_init(struct xt_entry_target *t)
{
}

static void TDROP_parse(struct xt_option_call *cb)
{
}

static void TDROP_print(const void *ip, const struct xt_entry_target *target,
                           int numeric)
{
}

static void TDROP_save(const void *ip, const struct xt_entry_target *target)
{
}

static struct xtables_target tdrop_target = {
	.name		= "TDROP",
	.version	= XTABLES_VERSION,
	.family		= NFPROTO_IPV4,
	.help		= TDROP_help,
	.init		= TDROP_init,
 	.x6_parse	= TDROP_parse,
	.print		= TDROP_print,
	.save		= TDROP_save,
};

void _init(void)
{
	xtables_register_target(&tdrop_target);
}
