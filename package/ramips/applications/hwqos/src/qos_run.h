#ifndef __QOS_RUN_H__
#define __QOS_RUN_H__

struct qos_rule_s;
struct af_group_s;

/* Flush anything about QoS */
void QoSFlush(void);

/* Init Ralin SW QoS, WAN interface is needed, ex: "eth2.2", or "ppp0"  */
void QoSCreate(char *wanif);

/*
 * Run DRR/SPQ/SPQDRR QoS
 *
 * interface:   To apply the QoS model on egress packets on WAN, please fill the string "imq1" ;
 *      To apply the QoS model on ingress packets on WAN, please fill the string "imq0" .
 *
 * rules:   The QoS rules. Please see the comment of struct qos_rules_s below.
 *
 * rule_count:  The QoS rules count.
 *
 * chain:   To apply the QoS rules on egress packets on WAN, please assign QOS_POSTROUTING_RULE_CHAIN;
 *      To apply the QoS rules on ingress packets on WAN, please assign QOS_PREROUTING_RULE_CHAIN.
 *
 * bandwidth:   The maximum bandwidth of WAN interface in kbit/s. (Its direction depends on the
 *              'interface' argument assigned previously.)
 *
 * af_group:    Four groups attributes, including the min/max bandwidth of each group in kbit/s.
 *      Use QOS_QUEUE_MAX/QOS_QUEUE_MIN macro to setup this structure with floating value.
 *
 */
void DRRQoSRun(char *interface, struct qos_rule_s *rules, int rule_count, const char *chain, int bandwidth_dl, int bandwidth_up, struct af_group_s *af_group, int dir);
void SPQQoSRun(char *wanif, struct qos_rule_s *rules, int rule_count, const char *chain, int bandwidth_dl, int bandwidth_up,  struct af_group_s *af_group, int dir);
void SPQDRRQoSRun(char *interface, struct qos_rule_s *rules, int rule_count, const char *chain, int bandwidth_dl, int bandwidth_up, struct af_group_s *af_group, int dir);

struct qos_rule_s
{
    int     af_index;           /*  which queue this rule belong to, QOS_QUEUE[4-1] */
    /*      Queue1 is the highest priority. */
    /*      Queue2 is the high priority. */
    /*      Queue3 is default priority. */
    /*      Queue4 is the lowest priority. */

    int     dp_index;           /*  reserved                                    */

    char    mac_address[18];    /*  src mac address, ex: "11:22:33:44:55:66"    */

    char    sip[32];            /*  source ip,      ex: "111.222.111.222"       */

    char    dip[32];            /*  dest ip                                     */

    char    pktlenfrom[8];      /*  packet length, from         ex: "163"       */
    char    pktlento[8];        /*  packet length, to       ex: "234"           */

    char    protocol[16];       /* "TCP", "UDP", "ICMP", or "Application"       */

    char    sprf[8], sprt[8];   /*  TCP/UDP source port range
                                    ex: sprf= "79", sprt="93"                   */

    char    dprf[8], dprt[8];   /*  TCP/UDP dest port range
                                    ex: dprf= "12345", sprt="12678"             */

    char    layer7[64];         /*  layer7 filter name. see /etc_ro/l7-protocols.
                                    Bind with "Application".                    */

    char    dscp[8];            /*  DSCP class name, ex: "AF42", "AF13", "BE",
                                    or "EF".                                    */

    char    remarker[8];        /*  remark DSCP class, ex: "AF23", "AF13", "BE", "EF",
                                    or "N/A"(not change).                       */
};


#define QOS_QUEUE1      5       /* highest queue */
#define QOS_QUEUE2      2       /* middle  queue */
#define QOS_QUEUE3      6       /* default queue */
#define QOS_QUEUE4      1       /* lowest  queue */


struct port_based_rule_s
{
    unsigned int port;
    unsigned int priority;
};

struct af_group_s
{
    int af1_ceil, af1_rate;
    int af2_ceil, af2_rate;
    int af3_ceil, af3_rate;
    int af4_ceil, af4_rate;
    int af5_ceil, af5_rate;
    int af6_ceil, af6_rate;
};

extern int debug_buildrule;

#define FIFO_QUEUE_LEN  100

#define DEFAULT_GROUP           QOS_QUEUE3

#define QOS_QUEUE1_MIN(x)       (x.af5_rate)
#define QOS_QUEUE1_MAX(x)       (x.af5_ceil)
#define QOS_QUEUE2_MIN(x)       (x.af2_rate)
#define QOS_QUEUE2_MAX(x)       (x.af2_ceil)
#define QOS_QUEUE3_MIN(x)       (x.af6_rate)
#define QOS_QUEUE3_MAX(x)       (x.af6_ceil)
#define QOS_QUEUE4_MIN(x)       (x.af1_rate)
#define QOS_QUEUE4_MAX(x)       (x.af1_ceil)
#define QOS_QUEUE5_MIN(x)       (x.af4_rate)    /* not used */
#define QOS_QUEUE5_MAX(x)       (x.af4_ceil)    /* not used */
#define QOS_QUEUE6_MIN(x)       (x.af3_rate)    /* not used */
#define QOS_QUEUE6_MAX(x)       (x.af3_ceil)    /* not used */

#define QOS_PREROUTING_RULE_CHAIN   "qos_prerouting_rule_chain"
#define QOS_POSTROUTING_RULE_CHAIN  "qos_postrouting_rule_chain"

#define QOS_PREROUTING_IMQ_CHAIN    "qos_prerouting_imq_chain"
#define QOS_POSTROUTING_IMQ_CHAIN   "qos_postrouting_imq_chain"

#define CLASSIFIER_RULE         0
#define REMARK_RULE             1
#define RETURN_RULE             2

#define QOS_MODEL_DRR           1
#define QOS_MODEL_SPQ           2
#define QOS_MODEL_SPQ_DRR       3
#define QOS_MODEL_REMARKONLY    4
#define QOS_MODEL_SFQ         5

#define HARD_LIMIT_RATE_MIN         2.0         // kbits

#define AF1_MAX_LATENCY             500         // ms
#define AF2_MAX_LATENCY             500         // ms
#define AF3_MAX_LATENCY             500         // ms
#define AF4_MAX_LATENCY             500         // ms
#define AF5_MAX_LATENCY             300         // ms (EF)
#define AF6_MAX_LATENCY             500         // ms (BE)



#endif


