#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/netfilter.h>
#include <linux/netfilter/x_tables.h>
#include <linux/netfilter/xt_limit.h>
#include <net/netfilter/nf_conntrack.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_ipv4/ip_tables.h>

#include <net/netfilter/nf_conntrack_labels.h>

#define XT_TIMER_RECHECK_ESTABLISHED	0
#define XT_TIMER_RECHECK_RELATED		1

#define XT_TIMER_DISABLED	0
#define XT_TIMER_ENABLED		1
#define XT_TIMER_EXPIRED		2

static LIST_HEAD(xt_timer_list);
static DEFINE_SPINLOCK(xt_timer_lock);

struct xt_recheck_mtinfo {
	uint32_t mode;
};

/*#define XT_TIME_EXPIRED 0x01*/
#define XT_RECHECK_MODE 0x02
#define XT_RECHECK_DROP 0x04

static bool
recheck_mt(const struct sk_buff *skb, struct xt_action_param *par)
{
	const struct xt_recheck_mtinfo *info = par->matchinfo;
	enum ip_conntrack_info ctinfo;
	struct nf_conn *ct, *master;
	struct net *net;
	uint32_t xtgenid;
	int old = XT_TIMER_EXPIRED;
	
	ct = nf_ct_get(skb, &ctinfo);
	if (!ct)
		return false;

	net = nf_ct_net(ct);
	master = master_ct(ct);

	switch (info->mode) {
	case XT_TIMER_RECHECK_RELATED:
		if (master && 
		    nf_ct_is_dying(master) &&
		    master->xt_status == XT_RECHECK_DROP)
			return false;
		return true;
	case XT_TIMER_RECHECK_ESTABLISHED:
		if (master) { 
			if (nf_ct_is_dying(master) &&
			    master->xt_status == XT_RECHECK_DROP)
				return false;
			return true;
		}
		if (atomic_cmpxchg(&ct->timer_status, old, XT_TIMER_DISABLED) == old) {
			pr_debug("DBG recheck found marked ct %p, recheck it\n", ct);
			ct->xt_status = XT_RECHECK_MODE;
			return false;
		}
		xtgenid = atomic_read(&net->xt.filter_table_genid);
		if (xtgenid != atomic_read(&ct->fwgenid)) {
			pr_debug("DBG recheck by genid ct %p %d, xtgendi %d\n", ct, atomic_read(&ct->fwgenid), xtgenid);
			atomic_set(&ct->fwgenid, xtgenid);
			ct->xt_status = XT_RECHECK_MODE;
			return false;
		}
	}

	return true;
}

static unsigned int recheck_drop(struct sk_buff *skb, const struct xt_action_param *par)
{
	enum ip_conntrack_info ctinfo;
	struct nf_conn *ct;

	ct = nf_ct_get(skb, &ctinfo);
	if (!ct || ctinfo == IP_CT_NEW)
		return NF_DROP;
	
	if (ct->xt_status == XT_RECHECK_MODE) {
		ct->xt_status = XT_RECHECK_DROP;
		nf_ct_kill(ct);
	}

	return NF_DROP;
}

static void xt_ct_delete_timer(struct nf_conn *ct, int new_state)
{
	list_del(&ct->recheck_list);

	atomic_set(&ct->timer_status, new_state);

	nf_ct_put(ct);
}

static void xt_time_recheck(unsigned long ul_ct)
{
	struct nf_conn *ct = (struct nf_conn *)ul_ct;

	spin_lock(&xt_timer_lock);
	xt_ct_delete_timer(ct, XT_TIMER_EXPIRED);
	spin_unlock(&xt_timer_lock);
}

static unsigned int timer_recheck_accept(struct sk_buff *skb, const struct xt_action_param *par)
{
	struct nf_conn *ct;
	enum ip_conntrack_info ctinfo;
	int old = XT_TIMER_DISABLED;

	ct = nf_ct_get(skb, &ctinfo);
	if (!ct)
		return NF_ACCEPT;

	if (ctinfo == IP_CT_NEW)
		setup_timer(&ct->xt_timeout, xt_time_recheck, (unsigned long)ct);

	mod_timer(&ct->xt_timeout, jiffies + ct->xt_time_diff * HZ);

	if (atomic_cmpxchg(&ct->timer_status, old, XT_TIMER_ENABLED) != old)
		return NF_ACCEPT;

	nf_conntrack_get(&ct->ct_general);

	spin_lock(&xt_timer_lock);
	list_add(&ct->recheck_list, &xt_timer_list);
	spin_unlock(&xt_timer_lock);

	return NF_ACCEPT;
}

static struct xt_target timer_recheck_tg[] __read_mostly = {
	{
		.name	= "TDROP",
		.family	= NFPROTO_IPV4,
		.target	= recheck_drop,
		.hooks	= (1 << NF_INET_LOCAL_IN) |
			  (1 << NF_INET_FORWARD)  |
			  (1 << NF_INET_LOCAL_OUT),
		.me	= THIS_MODULE,
	},
	{
		.name	= "TACCEPT",
		.family	= NFPROTO_IPV4,
		.target	= timer_recheck_accept,
		.hooks	= (1 << NF_INET_LOCAL_IN) |
			  (1 << NF_INET_FORWARD)  |
			  (1 << NF_INET_LOCAL_OUT),
		.me	= THIS_MODULE,
	},
};

static struct xt_match recheck_mt_reg __read_mostly = {
	.name		= "recheck",
	.family		= NFPROTO_IPV4,
	.match		= recheck_mt,
	.matchsize	= sizeof(struct xt_recheck_mtinfo),
	.me		= THIS_MODULE,
};

static void xt_clear_list(void)
{
	struct nf_conn *ct, *tmp;
	int timer_run;

restart:
	timer_run = 0;

	spin_lock_bh(&xt_timer_lock);
	list_for_each_entry_safe(ct, tmp, &xt_timer_list, recheck_list) {
		/* del_timer_sync cannot be used here 
		 * because xt_timer_lock is held in the timer function 
		 */
		if (!del_timer(&ct->xt_timeout)) {
			timer_run++;
			continue;
		}
		xt_ct_delete_timer(ct, XT_TIMER_DISABLED);
	}
	spin_unlock_bh(&xt_timer_lock);

	/* If we find some running timers, we will wait for them to
	 * finish while we'll be trying to acquire the spin_lock. 
	 * So no need to use cpu_relax(), schedule() etc.
	 */
	if (timer_run && !list_empty(&xt_timer_list))
		goto restart;
}

static int __init ipt_tm_init(void)
{
	int ret;
	ret = xt_register_match(&recheck_mt_reg);
	if (ret)
		goto out;
	
	ret = xt_register_targets(timer_recheck_tg, ARRAY_SIZE(timer_recheck_tg));
	if (ret)
		goto unreg_matches;

out:
	return ret;
unreg_matches:
	xt_unregister_match(&recheck_mt_reg);
	goto out;
}

static void __exit ipt_tm_exit(void)
{
	xt_unregister_targets(timer_recheck_tg, ARRAY_SIZE(timer_recheck_tg));
	xt_unregister_match(&recheck_mt_reg);
	xt_clear_list();
}

module_init(ipt_tm_init);
module_exit(ipt_tm_exit);
MODULE_LICENSE ("GPL");
