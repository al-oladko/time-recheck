#include <xtables.h>
#include <string.h>
#include <stdio.h>

/*----------------------------------------------*/
struct xt_recheck_genid_mtinfo {
     uint32_t mode;
};
/*----------------------------------------------*/
#define XT_TIMER_RECHECK_ESTABLISHED      0
#define XT_TIMER_RECHECK_RELATED    1

enum {
	O_CHECK_GENID = 0,
	O_CHECK_RELATED,
};

static void recheck_genid_mt_help(void)
{
	printf(
"recheck match options:\n"
" --recheck-established    Recheck connection by timer expired\n"
" --recheck-related  Check related connection\n");
}

static const struct xt_option_entry recheck_genid_mt_opts[] = {
	{.name = "recheck-established", .id = O_CHECK_GENID, .type = XTTYPE_NONE,},
	{.name = "recheck-related", .id = O_CHECK_RELATED, .type = XTTYPE_NONE,},
	XTOPT_TABLEEND,
};

static void recheck_genid_mt_print(const void *ip, const struct xt_entry_match *match,
                       int numeric)
{

	const struct xt_recheck_genid_mtinfo *info = (struct xt_recheck_genid_mtinfo *)match->data;

	switch(info->mode) {
	case XT_TIMER_RECHECK_ESTABLISHED:
		printf("recheck-established ");
		break;
	case XT_TIMER_RECHECK_RELATED:
		printf("recheck-related ");
		break;
	}
}

static void recheck_genid_mt_save(const void *ip, const struct xt_entry_match *match)
{
	const struct xt_recheck_genid_mtinfo *info = (struct xt_recheck_genid_mtinfo *)match->data;

	switch(info->mode) {
	case XT_TIMER_RECHECK_ESTABLISHED:
		printf(" --recheck-established");
		break;
	case XT_TIMER_RECHECK_RELATED:
		printf(" --recheck-related ");
		break;
	}
}

static void recheck_genid_mt_parse(struct xt_option_call *cb)
{
	struct xt_recheck_genid_mtinfo *info = cb->data;

	xtables_option_parse(cb);
	switch (cb->entry->id) {
	case O_CHECK_GENID:
		info->mode = XT_TIMER_RECHECK_ESTABLISHED;
		break;
	case O_CHECK_RELATED:
		info->mode = XT_TIMER_RECHECK_RELATED;
		break;
	}
}

static struct xtables_match recheck_genid_mt_reg[] = {
	{
		.version       = XTABLES_VERSION,
		.name          = "recheck",
		.revision      = 0,
		.family        = NFPROTO_IPV4,
		.size          = XT_ALIGN(sizeof(struct xt_recheck_genid_mtinfo)),
		.userspacesize = XT_ALIGN(sizeof(struct xt_recheck_genid_mtinfo)),
		.help          = recheck_genid_mt_help,
		.print         = recheck_genid_mt_print,
		.save          = recheck_genid_mt_save,
		.x6_parse      = recheck_genid_mt_parse,
		.x6_options    = recheck_genid_mt_opts,
	},
};

void _init(void)
{
	xtables_register_matches(recheck_genid_mt_reg, ARRAY_SIZE(recheck_genid_mt_reg));
}
