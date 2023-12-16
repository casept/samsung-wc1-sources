/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

/***
  This file is part of systemd.

  Copyright 2014 Lennart Poettering

  systemd is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  systemd is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with systemd; If not, see <http://www.gnu.org/licenses/>.
***/

#include <netinet/tcp.h>

#include "missing.h"
#include "strv.h"
#include "socket-util.h"
#include "af-list.h"
#include "resolved-dns-domain.h"
#include "resolved-dns-scope.h"

#define MULTICAST_RATELIMIT_INTERVAL_USEC (1*USEC_PER_SEC)
#define MULTICAST_RATELIMIT_BURST 1000

int dns_scope_new(Manager *m, DnsScope **ret, Link *l, DnsProtocol protocol, int family) {
        DnsScope *s;

        assert(m);
        assert(ret);

        s = new0(DnsScope, 1);
        if (!s)
                return -ENOMEM;

        s->manager = m;
        s->link = l;
        s->protocol = protocol;
        s->family = family;

        LIST_PREPEND(scopes, m->dns_scopes, s);

        dns_scope_llmnr_membership(s, true);

        log_debug("New scope on link %s, protocol %s, family %s", l ? l->name : "*", dns_protocol_to_string(protocol), family == AF_UNSPEC ? "*" : af_to_name(family));

        /* Enforce ratelimiting for the multicast protocols */
        RATELIMIT_INIT(s->ratelimit, MULTICAST_RATELIMIT_INTERVAL_USEC, MULTICAST_RATELIMIT_BURST);

        *ret = s;
        return 0;
}

DnsScope* dns_scope_free(DnsScope *s) {
        DnsTransaction *t;
        DnsResourceRecord *rr;

        if (!s)
                return NULL;

        log_debug("Removing scope on link %s, protocol %s, family %s", s->link ? s->link->name : "*", dns_protocol_to_string(s->protocol), s->family == AF_UNSPEC ? "*" : af_to_name(s->family));

        dns_scope_llmnr_membership(s, false);

        while ((t = s->transactions)) {

                /* Abort the transaction, but make sure it is not
                 * freed while we still look at it */

                t->block_gc++;
                dns_transaction_complete(t, DNS_TRANSACTION_ABORTED);
                t->block_gc--;

                dns_transaction_free(t);
        }

        while ((rr = hashmap_steal_first(s->conflict_queue)))
                dns_resource_record_unref(rr);

        hashmap_free(s->conflict_queue);
        sd_event_source_unref(s->conflict_event_source);

        dns_cache_flush(&s->cache);
        dns_zone_flush(&s->zone);

        LIST_REMOVE(scopes, s->manager->dns_scopes, s);
        strv_free(s->domains);
        free(s);

        return NULL;
}

DnsServer *dns_scope_get_dns_server(DnsScope *s) {
        assert(s);

        if (s->protocol != DNS_PROTOCOL_DNS)
                return NULL;

        if (s->link)
                return link_get_dns_server(s->link);
        else
                return manager_get_dns_server(s->manager);
}

void dns_scope_next_dns_server(DnsScope *s) {
        assert(s);

        if (s->protocol != DNS_PROTOCOL_DNS)
                return;

        if (s->link)
                link_next_dns_server(s->link);
        else
                manager_next_dns_server(s->manager);
}

int dns_scope_emit(DnsScope *s, DnsPacket *p) {
        union in_addr_union addr;
        int ifindex = 0, r;
        int family;
        uint16_t port;
        uint32_t mtu;
        int fd;

        assert(s);
        assert(p);
        assert(p->protocol == s->protocol);

        if (s->link) {
                mtu = s->link->mtu;
                ifindex = s->link->ifindex;
        } else
                mtu = manager_find_mtu(s->manager);

        if (s->protocol == DNS_PROTOCOL_DNS) {
                DnsServer *srv;

                if (DNS_PACKET_QDCOUNT(p) > 1)
                        return -ENOTSUP;

                srv = dns_scope_get_dns_server(s);
                if (!srv)
                        return -ESRCH;

                family = srv->family;
                addr = srv->address;
                port = 53;

                if (p->size > DNS_PACKET_UNICAST_SIZE_MAX)
                        return -EMSGSIZE;

                if (p->size > mtu)
                        return -EMSGSIZE;

                if (family == AF_INET)
                        fd = manager_dns_ipv4_fd(s->manager);
                else if (family == AF_INET6)
                        fd = manager_dns_ipv6_fd(s->manager);
                else
                        return -EAFNOSUPPORT;
                if (fd < 0)
                        return fd;

        } else if (s->protocol == DNS_PROTOCOL_LLMNR) {

                if (DNS_PACKET_QDCOUNT(p) > 1)
                        return -ENOTSUP;

                if (!ratelimit_test(&s->ratelimit))
                        return -EBUSY;

                family = s->family;
                port = 5355;

                if (family == AF_INET) {
                        addr.in = LLMNR_MULTICAST_IPV4_ADDRESS;
                        fd = manager_llmnr_ipv4_udp_fd(s->manager);
                } else if (family == AF_INET6) {
                        addr.in6 = LLMNR_MULTICAST_IPV6_ADDRESS;
                        fd = manager_llmnr_ipv6_udp_fd(s->manager);
                } else
                        return -EAFNOSUPPORT;
                if (fd < 0)
                        return fd;
        } else
                return -EAFNOSUPPORT;

        r = manager_send(s->manager, fd, ifindex, family, &addr, port, p);
        if (r < 0)
                return r;

        return 1;
}

int dns_scope_tcp_socket(DnsScope *s, int family, const union in_addr_union *address, uint16_t port) {
        _cleanup_close_ int fd = -1;
        union sockaddr_union sa = {};
        socklen_t salen;
        static const int one = 1;
        int ret, r;

        assert(s);
        assert((family == AF_UNSPEC) == !address);

        if (family == AF_UNSPEC) {
                DnsServer *srv;

                srv = dns_scope_get_dns_server(s);
                if (!srv)
                        return -ESRCH;

                sa.sa.sa_family = srv->family;
                if (srv->family == AF_INET) {
                        sa.in.sin_port = htobe16(port);
                        sa.in.sin_addr = srv->address.in;
                        salen = sizeof(sa.in);
                } else if (srv->family == AF_INET6) {
                        sa.in6.sin6_port = htobe16(port);
                        sa.in6.sin6_addr = srv->address.in6;
                        sa.in6.sin6_scope_id = s->link ? s->link->ifindex : 0;
                        salen = sizeof(sa.in6);
                } else
                        return -EAFNOSUPPORT;
        } else {
                sa.sa.sa_family = family;

                if (family == AF_INET) {
                        sa.in.sin_port = htobe16(port);
                        sa.in.sin_addr = address->in;
                        salen = sizeof(sa.in);
                } else if (family == AF_INET6) {
                        sa.in6.sin6_port = htobe16(port);
                        sa.in6.sin6_addr = address->in6;
                        sa.in6.sin6_scope_id = s->link ? s->link->ifindex : 0;
                        salen = sizeof(sa.in6);
                } else
                        return -EAFNOSUPPORT;
        }

        fd = socket(sa.sa.sa_family, SOCK_STREAM|SOCK_CLOEXEC|SOCK_NONBLOCK, 0);
        if (fd < 0)
                return -errno;

        r = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
        if (r < 0)
                return -errno;

        if (s->link) {
                uint32_t ifindex = htobe32(s->link->ifindex);

                if (sa.sa.sa_family == AF_INET) {
                        r = setsockopt(fd, IPPROTO_IP, IP_UNICAST_IF, &ifindex, sizeof(ifindex));
                        if (r < 0)
                                return -errno;
                } else if (sa.sa.sa_family == AF_INET6) {
                        r = setsockopt(fd, IPPROTO_IPV6, IPV6_UNICAST_IF, &ifindex, sizeof(ifindex));
                        if (r < 0)
                                return -errno;
                }
        }

        if (s->protocol == DNS_PROTOCOL_LLMNR) {
                /* RFC 4795, section 2.5 requires the TTL to be set to 1 */

                if (sa.sa.sa_family == AF_INET) {
                        r = setsockopt(fd, IPPROTO_IP, IP_TTL, &one, sizeof(one));
                        if (r < 0)
                                return -errno;
                } else if (sa.sa.sa_family == AF_INET6) {
                        r = setsockopt(fd, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &one, sizeof(one));
                        if (r < 0)
                                return -errno;
                }
        }

        r = connect(fd, &sa.sa, salen);
        if (r < 0 && errno != EINPROGRESS)
                return -errno;

        ret = fd;
        fd = -1;

        return ret;
}

DnsScopeMatch dns_scope_good_domain(DnsScope *s, int ifindex, uint64_t flags, const char *domain) {
        char **i;

        assert(s);
        assert(domain);

        if (ifindex != 0 && (!s->link || s->link->ifindex != ifindex))
                return DNS_SCOPE_NO;

        if ((SD_RESOLVED_FLAGS_MAKE(s->protocol, s->family) & flags) == 0)
                return DNS_SCOPE_NO;

        STRV_FOREACH(i, s->domains)
                if (dns_name_endswith(domain, *i) > 0)
                        return DNS_SCOPE_YES;

        if (dns_name_root(domain) != 0)
                return DNS_SCOPE_NO;

        if (is_localhost(domain))
                return DNS_SCOPE_NO;

        if (s->protocol == DNS_PROTOCOL_DNS) {
                if (dns_name_endswith(domain, "254.169.in-addr.arpa") == 0 &&
                    dns_name_endswith(domain, "0.8.e.f.ip6.arpa") == 0 &&
                    dns_name_single_label(domain) == 0)
                        return DNS_SCOPE_MAYBE;

                return DNS_SCOPE_NO;
        }

        if (s->protocol == DNS_PROTOCOL_MDNS) {
                if (dns_name_endswith(domain, "254.169.in-addr.arpa") > 0 ||
                    dns_name_endswith(domain, "0.8.e.f.ip6.arpa") > 0 ||
                    (dns_name_endswith(domain, "local") > 0 && dns_name_equal(domain, "local") == 0))
                        return DNS_SCOPE_MAYBE;

                return DNS_SCOPE_NO;
        }

        if (s->protocol == DNS_PROTOCOL_LLMNR) {
                if (dns_name_endswith(domain, "in-addr.arpa") > 0 ||
                    dns_name_endswith(domain, "ip6.arpa") > 0 ||
                    dns_name_single_label(domain) > 0)
                        return DNS_SCOPE_MAYBE;

                return DNS_SCOPE_NO;
        }

        assert_not_reached("Unknown scope protocol");
}

int dns_scope_good_key(DnsScope *s, DnsResourceKey *key) {
        assert(s);
        assert(key);

        if (s->protocol == DNS_PROTOCOL_DNS)
                return true;

        /* On mDNS and LLMNR, send A and AAAA queries only on the
         * respective scopes */

        if (s->family == AF_INET && key->class == DNS_CLASS_IN && key->type == DNS_TYPE_AAAA)
                return false;

        if (s->family == AF_INET6 && key->class == DNS_CLASS_IN && key->type == DNS_TYPE_A)
                return false;

        return true;
}

int dns_scope_llmnr_membership(DnsScope *s, bool b) {
        int fd;

        assert(s);

        if (s->protocol != DNS_PROTOCOL_LLMNR)
                return 0;

        assert(s->link);

        if (s->family == AF_INET) {
                struct ip_mreqn mreqn = {
                        .imr_multiaddr = LLMNR_MULTICAST_IPV4_ADDRESS,
                        .imr_ifindex = s->link->ifindex,
                };

                fd = manager_llmnr_ipv4_udp_fd(s->manager);
                if (fd < 0)
                        return fd;

                /* Always first try to drop membership before we add
                 * one. This is necessary on some devices, such as
                 * veth. */
                if (b)
                        setsockopt(fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreqn, sizeof(mreqn));

                if (setsockopt(fd, IPPROTO_IP, b ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP, &mreqn, sizeof(mreqn)) < 0)
                        return -errno;

        } else if (s->family == AF_INET6) {
                struct ipv6_mreq mreq = {
                        .ipv6mr_multiaddr = LLMNR_MULTICAST_IPV6_ADDRESS,
                        .ipv6mr_interface = s->link->ifindex,
                };

                fd = manager_llmnr_ipv6_udp_fd(s->manager);
                if (fd < 0)
                        return fd;

                if (b)
                        setsockopt(fd, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP, &mreq, sizeof(mreq));

                if (setsockopt(fd, IPPROTO_IPV6, b ? IPV6_ADD_MEMBERSHIP : IPV6_DROP_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
                        return -errno;
        } else
                return -EAFNOSUPPORT;

        return 0;
}

int dns_scope_good_dns_server(DnsScope *s, int family, const union in_addr_union *address) {
        assert(s);
        assert(address);

        if (s->protocol != DNS_PROTOCOL_DNS)
                return 1;

        if (s->link)
                return !!link_find_dns_server(s->link,  family, address);
        else
                return !!manager_find_dns_server(s->manager, family, address);
}

static int dns_scope_make_reply_packet(
                DnsScope *s,
                uint16_t id,
                int rcode,
                DnsQuestion *q,
                DnsAnswer *answer,
                DnsAnswer *soa,
                bool tentative,
                DnsPacket **ret) {

        _cleanup_(dns_packet_unrefp) DnsPacket *p = NULL;
        unsigned i;
        int r;

        assert(s);
        assert(ret);

        if ((!q || q->n_keys <= 0)
            && (!answer || answer->n_rrs <= 0)
            && (!soa || soa->n_rrs <= 0))
                return -EINVAL;

        r = dns_packet_new(&p, s->protocol, 0);
        if (r < 0)
                return r;

        DNS_PACKET_HEADER(p)->id = id;
        DNS_PACKET_HEADER(p)->flags = htobe16(DNS_PACKET_MAKE_FLAGS(
                                                              1 /* qr */,
                                                              0 /* opcode */,
                                                              0 /* c */,
                                                              0 /* tc */,
                                                              tentative,
                                                              0 /* (ra) */,
                                                              0 /* (ad) */,
                                                              0 /* (cd) */,
                                                              rcode));

        if (q) {
                for (i = 0; i < q->n_keys; i++) {
                        r = dns_packet_append_key(p, q->keys[i], NULL);
                        if (r < 0)
                                return r;
                }

                DNS_PACKET_HEADER(p)->qdcount = htobe16(q->n_keys);
        }

        if (answer) {
                for (i = 0; i < answer->n_rrs; i++) {
                        r = dns_packet_append_rr(p, answer->rrs[i], NULL);
                        if (r < 0)
                                return r;
                }

                DNS_PACKET_HEADER(p)->ancount = htobe16(answer->n_rrs);
        }

        if (soa) {
                for (i = 0; i < soa->n_rrs; i++) {
                        r = dns_packet_append_rr(p, soa->rrs[i], NULL);
                        if (r < 0)
                                return r;
                }

                DNS_PACKET_HEADER(p)->arcount = htobe16(soa->n_rrs);
        }

        *ret = p;
        p = NULL;

        return 0;
}

static void dns_scope_verify_conflicts(DnsScope *s, DnsPacket *p) {
        unsigned n;

        assert(s);
        assert(p);

        if (p->question)
                for (n = 0; n < p->question->n_keys; n++)
                        dns_zone_verify_conflicts(&s->zone, p->question->keys[n]);
        if (p->answer)
                for (n = 0; n < p->answer->n_rrs; n++)
                        dns_zone_verify_conflicts(&s->zone, p->answer->rrs[n]->key);
}

void dns_scope_process_query(DnsScope *s, DnsStream *stream, DnsPacket *p) {
        _cleanup_(dns_packet_unrefp) DnsPacket *reply = NULL;
        _cleanup_(dns_answer_unrefp) DnsAnswer *answer = NULL, *soa = NULL;
        bool tentative = false;
        int r, fd;

        assert(s);
        assert(p);

        if (p->protocol != DNS_PROTOCOL_LLMNR)
                return;

        if (p->ipproto == IPPROTO_UDP) {
                /* Don't accept UDP queries directed to anything but
                 * the LLMNR multicast addresses. See RFC 4795,
                 * section 2.5.*/

                if (p->family == AF_INET && !in_addr_equal(AF_INET, &p->destination, (union in_addr_union*) &LLMNR_MULTICAST_IPV4_ADDRESS))
                        return;

                if (p->family == AF_INET6 && !in_addr_equal(AF_INET6, &p->destination, (union in_addr_union*) &LLMNR_MULTICAST_IPV6_ADDRESS))
                        return;
        }

        r = dns_packet_extract(p);
        if (r < 0) {
                log_debug("Failed to extract resources from incoming packet: %s", strerror(-r));
                return;
        }

        if (DNS_PACKET_C(p)) {
                /* Somebody notified us about a possible conflict */
                dns_scope_verify_conflicts(s, p);
                return;
        }

        r = dns_zone_lookup(&s->zone, p->question, &answer, &soa, &tentative);
        if (r < 0) {
                log_debug("Failed to lookup key: %s", strerror(-r));
                return;
        }
        if (r == 0)
                return;

        if (answer)
                dns_answer_order_by_scope(answer, in_addr_is_link_local(p->family, &p->sender) > 0);

        r = dns_scope_make_reply_packet(s, DNS_PACKET_ID(p), DNS_RCODE_SUCCESS, p->question, answer, soa, tentative, &reply);
        if (r < 0) {
                log_debug("Failed to build reply packet: %s", strerror(-r));
                return;
        }

        if (stream)
                r = dns_stream_write_packet(stream, reply);
        else {
                if (!ratelimit_test(&s->ratelimit))
                        return;

                if (p->family == AF_INET)
                        fd = manager_llmnr_ipv4_udp_fd(s->manager);
                else if (p->family == AF_INET6)
                        fd = manager_llmnr_ipv6_udp_fd(s->manager);
                else {
                        log_debug("Unknown protocol");
                        return;
                }
                if (fd < 0) {
                        log_debug("Failed to get reply socket: %s", strerror(-fd));
                        return;
                }

                /* Note that we always immediately reply to all LLMNR
                 * requests, and do not wait any time, since we
                 * verified uniqueness for all records. Also see RFC
                 * 4795, Section 2.7 */

                r = manager_send(s->manager, fd, p->ifindex, p->family, &p->sender, p->sender_port, reply);
        }

        if (r < 0) {
                log_debug("Failed to send reply packet: %s", strerror(-r));
                return;
        }
}

DnsTransaction *dns_scope_find_transaction(DnsScope *scope, DnsQuestion *question, bool cache_ok) {
        DnsTransaction *t;

        assert(scope);
        assert(question);

        /* Try to find an ongoing transaction that is a equal or a
         * superset of the specified question */

        LIST_FOREACH(transactions_by_scope, t, scope->transactions) {

                /* Refuse reusing transactions that completed based on
                 * cached data instead of a real packet, if that's
                 * requested. */
                if (!cache_ok &&
                    IN_SET(t->state, DNS_TRANSACTION_SUCCESS, DNS_TRANSACTION_FAILURE) &&
                    !t->received)
                        continue;

                if (dns_question_is_superset(t->question, question) > 0)
                        return t;
        }

        return NULL;
}

static int dns_scope_make_conflict_packet(
                DnsScope *s,
                DnsResourceRecord *rr,
                DnsPacket **ret) {

        _cleanup_(dns_packet_unrefp) DnsPacket *p = NULL;
        int r;

        assert(s);
        assert(rr);
        assert(ret);

        r = dns_packet_new(&p, s->protocol, 0);
        if (r < 0)
                return r;

        DNS_PACKET_HEADER(p)->flags = htobe16(DNS_PACKET_MAKE_FLAGS(
                                                              0 /* qr */,
                                                              0 /* opcode */,
                                                              1 /* conflict */,
                                                              0 /* tc */,
                                                              0 /* t */,
                                                              0 /* (ra) */,
                                                              0 /* (ad) */,
                                                              0 /* (cd) */,
                                                              0));
        random_bytes(&DNS_PACKET_HEADER(p)->id, sizeof(uint16_t));
        DNS_PACKET_HEADER(p)->qdcount = htobe16(1);
        DNS_PACKET_HEADER(p)->arcount = htobe16(1);

        r = dns_packet_append_key(p, rr->key, NULL);
        if (r < 0)
                return r;

        r = dns_packet_append_rr(p, rr, NULL);
        if (r < 0)
                return r;

        *ret = p;
        p = NULL;

        return 0;
}

static int on_conflict_dispatch(sd_event_source *es, usec_t usec, void *userdata) {
        DnsScope *scope = userdata;
        int r;

        assert(es);
        assert(scope);

        scope->conflict_event_source = sd_event_source_unref(scope->conflict_event_source);

        for (;;) {
                _cleanup_(dns_resource_record_unrefp) DnsResourceRecord *rr = NULL;
                _cleanup_(dns_packet_unrefp) DnsPacket *p = NULL;

                rr = hashmap_steal_first(scope->conflict_queue);
                if (!rr)
                        break;

                r = dns_scope_make_conflict_packet(scope, rr, &p);
                if (r < 0) {
                        log_error("Failed to make conflict packet: %s", strerror(-r));
                        return 0;
                }

                r = dns_scope_emit(scope, p);
                if (r < 0)
                        log_debug("Failed to send conflict packet: %s", strerror(-r));
        }

        return 0;
}

int dns_scope_notify_conflict(DnsScope *scope, DnsResourceRecord *rr) {
        usec_t jitter;
        int r;

        assert(scope);
        assert(rr);

        /* We don't send these queries immediately. Instead, we queue
         * them, and send them after some jitter delay. */
        r = hashmap_ensure_allocated(&scope->conflict_queue, dns_resource_key_hash_func, dns_resource_key_compare_func);
        if (r < 0) {
                log_oom();
                return r;
        }

        /* We only place one RR per key in the conflict
         * messages, not all of them. That should be enough to
         * indicate where there might be a conflict */
        r = hashmap_put(scope->conflict_queue, rr->key, rr);
        if (r == -EEXIST || r == 0)
                return 0;
        if (r < 0) {
                log_debug("Failed to queue conflicting RR: %s", strerror(-r));
                return r;
        }

        dns_resource_record_ref(rr);

        if (scope->conflict_event_source)
                return 0;

        random_bytes(&jitter, sizeof(jitter));
        jitter %= LLMNR_JITTER_INTERVAL_USEC;

        r = sd_event_add_time(scope->manager->event,
                              &scope->conflict_event_source,
                              clock_boottime_or_monotonic(),
                              now(clock_boottime_or_monotonic()) + jitter,
                              LLMNR_JITTER_INTERVAL_USEC,
                              on_conflict_dispatch, scope);
        if (r < 0) {
                log_debug("Failed to add conflict dispatch event: %s", strerror(-r));
                return r;
        }

        return 0;
}

void dns_scope_check_conflicts(DnsScope *scope, DnsPacket *p) {
        unsigned i;
        int r;

        assert(scope);
        assert(p);

        if (p->protocol != DNS_PROTOCOL_LLMNR)
                return;

        if (DNS_PACKET_RRCOUNT(p) <= 0)
                return;

        if (DNS_PACKET_C(p) != 0)
                return;

        if (DNS_PACKET_T(p) != 0)
                return;

        if (manager_our_packet(scope->manager, p))
                return;

        r = dns_packet_extract(p);
        if (r < 0) {
                log_debug("Failed to extract packet: %s", strerror(-r));
                return;
        }

        log_debug("Checking for conflicts...");

        for (i = 0; i < p->answer->n_rrs; i++) {

                /* Check for conflicts against the local zone. If we
                 * found one, we won't check any further */
                r = dns_zone_check_conflicts(&scope->zone, p->answer->rrs[i]);
                if (r != 0)
                        continue;

                /* Check for conflicts against the local cache. If so,
                 * send out an advisory query, to inform everybody */
                r = dns_cache_check_conflicts(&scope->cache, p->answer->rrs[i], p->family, &p->sender);
                if (r <= 0)
                        continue;

                dns_scope_notify_conflict(scope, p->answer->rrs[i]);
        }
}
