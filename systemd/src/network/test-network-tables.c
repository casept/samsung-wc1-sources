#include "networkd.h"
#include "networkd-link.h"
#include "networkd-netdev-bond.h"
#include "networkd-netdev-macvlan.h"
#include "dhcp6-internal.h"
#include "dhcp6-protocol.h"
#include "rtnl-internal.h"
#include "ethtool-util.h"

#include "test-tables.h"

int main(int argc, char **argv) {
        test_table(bond_mode, NETDEV_BOND_MODE);
        /* test_table(link_state, LINK_STATE);  -- not a reversible mapping */
        test_table(link_operstate, LINK_OPERSTATE);
        test_table(dhcp_support, DHCP_SUPPORT);
        test_table(netdev_kind, NETDEV_KIND);
        test_table(dhcp6_message_status, DHCP6_STATUS);
        test_table(duplex, DUP);
        test_table(wol, WOL);
        test_table(nl_union_link_info_data, NL_UNION_LINK_INFO_DATA);

        test_table_sparse(macvlan_mode, NETDEV_MACVLAN_MODE);
        test_table_sparse(dhcp6_message_type, DHCP6_MESSAGE);

        return EXIT_SUCCESS;
}
