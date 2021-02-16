#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <xtables.h>
#include <limits.h> /* INT_MAX in ip_tables.h */
#include <linux/netfilter_ipv4/ip_tables.h>

static void TACCEPT_help(void)
{
	printf("TACCEPT, no options provided\n");
}

static void TACCEPT_init(struct xt_entry_target *t)
{
}

static void TACCEPT_parse(struct xt_option_call *cb)
{
}

static void TACCEPT_print(const void *ip, const struct xt_entry_target *target,
                           int numeric)
{
}

static void TACCEPT_save(const void *ip, const struct xt_entry_target *target)
{
}

static struct xtables_target taccept_target = {
	.name		= "TACCEPT",
	.version	= XTABLES_VERSION,
	.family		= NFPROTO_IPV4,
	.help		= TACCEPT_help,
	.init		= TACCEPT_init,
 	.x6_parse	= TACCEPT_parse,
	.print		= TACCEPT_print,
	.save		= TACCEPT_save,
};

void _init(void)
{
	xtables_register_target(&taccept_target);
}
