#include <iostream>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <unistd.h>
#include <string>

#define DRIVER_NAME "nl80211"

enum
{
    WIFI_ATTR_UNSPEC,
    WIFI_ATTR_WIPHY,
    WIFI_ATTR_IFINDEX,
    WIFI_ATTR_BEACON,
    __WIFI_ATTR_AFTER_LAST,
    WIFI_ATTR_MAX = __WIFI_ATTR_AFTER_LAST - 1,
};

enum
{
    WIFI_CMD_UNSPEC,
    WIFI_CMD_TRIGGER_BEACON,
    __WIFI_CMD_AFTER_LAST,
    WIFI_CMD_MAX = __WIFI_CMD_AFTER_LAST - 1,
};

int main()
{
    struct nl_sock *nlSock = nl_socket_alloc();
    if (!nlSock)
    {
        std::cerr << "Failed to allocate netlink socket." << std::endl;
        return -1;
    }

    if (nl_connect(nlSock, NETLINK_GENERIC) < 0)
    {
        std::cerr << "Failed to connect to generic netlink." << std::endl;
        nl_socket_free(nlSock);
        return -1;
    }

    int driverId = genl_ctrl_resolve(nlSock, DRIVER_NAME);
    if (driverId < 0)
    {
        std::cerr << "Failed to resolve driver name." << std::endl;
        nl_socket_free(nlSock);
        return -1;
    }

    struct nl_msg *nlMsg = nlmsg_alloc();
    if (!nlMsg)
    {
        std::cerr << "Failed to allocate netlink message." << std::endl;
        nl_socket_free(nlSock);
        return -1;
    }

    const std::string ssid = "RemoteID"; // Substitua "MeuSSID" pelo SSID desejado
    uint32_t frequency = 2437;              // Frequência em MHz, substitua pelo valor desejado

    int ssidLen = ssid.length();
    int attrLen = nla_total_size(ssidLen);

    // Montagem da mensagem netlink
    genlmsg_put(nlMsg, NL_AUTO_PID, NL_AUTO_SEQ, driverId, 0, NLM_F_REQUEST, WIFI_CMD_TRIGGER_BEACON, 0);

    nlmsg_reserve(nlMsg, 0, attrLen);

    struct nlattr *attr = nla_reserve(nlMsg, WIFI_ATTR_BEACON, attrLen);
    if (!attr)
    {
        std::cerr << "Failed to reserve attribute space for SSID." << std::endl;
        nlmsg_free(nlMsg);
        nl_socket_free(nlSock);
        return -1;
    }

    /* nla_put_string(nlMsg, WIFI_ATTR_BEACON, ssid.c_str());
    nla_put_flag(nlMsg, 1);
    nla_put_u32(nlMsg, WIFI_ATTR_BEACON, frequency);
    uint32_t retrievedChannel = nla_get_u32(attr);
    std::cout << retrievedChannel << std::endl; */

    nla_put(nlMsg, WIFI_ATTR_BEACON, ssidLen, ssid.c_str());

    while (true)
    {

        genlmsg_put(nlMsg, NL_AUTO_PID, NL_AUTO_SEQ, driverId, 0, NLM_F_REQUEST, WIFI_CMD_TRIGGER_BEACON, 0);

        if (nl_send_auto(nlSock, nlMsg) < 0)
        {
            std::cerr << "Failed to send netlink message." << std::endl;
            nlmsg_free(nlMsg);
            nl_socket_free(nlSock);
            return -1;
        }

        std::cout << "Sinal Wi-Fi enviado com o SSID: " << ssid << std::endl;
        sleep(1);
    }

    return 0;
}
