#include <stdio.h>
#include <string.h>

#include "common.h"
#include "w5100s.h"
#include "socket.h"

#include "util.h"

/**socket number for NetBIOS Name service**/
//#define NETBIOS_SOCK    6
#ifndef SOCK_NETBIOS
    #define SOCK_NETBIOS    6
#endif

/** default port number for "NetBIOS Name service */
#define NETBIOS_PORT     137

/** size of a NetBIOS name */
#define NETBIOS_NAME_LEN 16
/**NetBIOS message maximum length*/
#define NETBIOS_MSG_MAX_LEN 512
/** The Time-To-Live for NetBIOS name responds (in seconds)
 * Default is 300000 seconds (3 days, 11 hours, 20 minutes) */
#define NETBIOS_NAME_TTL 10//300000

/** NetBIOS header flags */
#define NETB_HFLAG_RESPONSE           0x8000U
#define NETB_HFLAG_OPCODE             0x7800U
#define NETB_HFLAG_OPCODE_NAME_QUERY  0x0000U
#define NETB_HFLAG_AUTHORATIVE        0x0400U
#define NETB_HFLAG_TRUNCATED          0x0200U
#define NETB_HFLAG_RECURS_DESIRED     0x0100U
#define NETB_HFLAG_RECURS_AVAILABLE   0x0080U
#define NETB_HFLAG_BROADCAST          0x0010U
#define NETB_HFLAG_REPLYCODE          0x0008U
#define NETB_HFLAG_REPLYCODE_NOERROR  0x0000U

/** NetBIOS name flags */
#define NETB_NFLAG_UNIQUE             0x8000U
#define NETB_NFLAG_NODETYPE           0x6000U
#define NETB_NFLAG_NODETYPE_HNODE     0x6000U
#define NETB_NFLAG_NODETYPE_MNODE     0x4000U
#define NETB_NFLAG_NODETYPE_PNODE     0x2000U
#define NETB_NFLAG_NODETYPE_BNODE     0x0000U

/** NetBIOS message header */
#pragma pack(1)
typedef struct _NETBIOS_HDR {
  uint16_t trans_id;
  uint16_t flags;
  uint16_t questions;
  uint16_t answerRRs;
  uint16_t authorityRRs;
  uint16_t additionalRRs;
}NETBIOS_HDR;


/** NetBIOS message name part */

typedef struct _NETBIOS_NAME_HDR {
  uint8_t  nametype;
  uint8_t  encname[(NETBIOS_NAME_LEN*2)+1];
  uint16_t type;
  uint16_t cls;
  uint32_t ttl;
  uint16_t datalen;
  uint16_t flags;
  //ip_addr_p_t addr;
  uint8_t addr[4];
}NETBIOS_NAME_HDR;

char NETBIOS_SOCK = SOCK_NETBIOS;
char netbios_rx_buf[NETBIOS_MSG_MAX_LEN];
char netbios_tx_buf[NETBIOS_MSG_MAX_LEN];
char netname[16];

/** NetBIOS message */

typedef struct _NETBIOS_RESP
{
  NETBIOS_HDR      resp_hdr;
  NETBIOS_NAME_HDR resp_name;
}NETBIOS_RESP;
#pragma pack()

/** NetBIOS decoding name */
static int
netbios_name_decoding( char *name_enc, char *name_dec, int name_dec_len)
{
  char *pname;
  char  cname;
  char  cnbname;
  int   index = 0;

  /* Start decoding netbios name. */
  pname  = name_enc;
  for (;;) {
    /* Every two characters of the first level-encoded name
     * turn into one character in the decoded name. */
    cname = *pname;
    if (cname == '\0')
      break;    /* no more characters */
    if (cname == '.')
      break;    /* scope ID follows */
    if (cname < 'A' || cname > 'Z') {
      /* Not legal. */
      return -1;
    }
    cname -= 'A';
    cnbname = cname << 4;
    pname++;

    cname = *pname;
    if (cname == '\0' || cname == '.') {
      /* No more characters in the name - but we're in
       * the middle of a pair.  Not legal. */
      return -1;
    }
    if (cname < 'A' || cname > 'Z') {
      /* Not legal. */
      return -1;
    }
    cname -= 'A';
    cnbname |= cname;
    pname++;

    /* Do we have room to store the character? */
    if (index < NETBIOS_NAME_LEN) {
      /* Yes - store the character. */
      name_dec[index++] = (cnbname!=' '?cnbname:'\0');
    }
  }

  return 0;
}

void do_netbios(void)
{
  unsigned char state;
  unsigned int len;

  unsigned char rem_ip_addr[4];
  uint16_t rem_udp_port;
  char netbios_name[NETBIOS_NAME_LEN+1];
  NETBIOS_HDR* netbios_hdr;
  NETBIOS_NAME_HDR* netbios_name_hdr;

  state = getSn_SR(NETBIOS_SOCK);
  switch(state)
  {
  case SOCK_UDP:
    if((len=getSn_RX_RSR(NETBIOS_SOCK))>0)
    {
      len=recvfrom(NETBIOS_SOCK,(unsigned char*)&netbios_rx_buf,len,rem_ip_addr,&rem_udp_port);
      //printf("netbios rx len=%d\r\n",len);
     // printf("rem_ip_addr=%d.%d.%d.%d:%d\r\n",rem_ip_addr[0],rem_ip_addr[1],rem_ip_addr[2],rem_ip_addr[3],rem_udp_port);
      netbios_hdr = (NETBIOS_HDR*)netbios_rx_buf;
      netbios_name_hdr = (NETBIOS_NAME_HDR*)(netbios_hdr+1);
      /* if the packet is a NetBIOS name query question */
      if (((netbios_hdr->flags & ntohs(NETB_HFLAG_OPCODE)) == ntohs(NETB_HFLAG_OPCODE_NAME_QUERY)) &&
          ((netbios_hdr->flags & ntohs(NETB_HFLAG_RESPONSE)) == 0) &&
           (netbios_hdr->questions == ntohs(1))) 
      {
  
        /* decode the NetBIOS name */
        netbios_name_decoding( (char*)(netbios_name_hdr->encname), netbios_name, sizeof(netbios_name));
       // printf("name is %s\r\n",netbios_name);
        /* if the packet is for us */
        if (strcasecmp(netbios_name,netname) == 0) 
        {
          uint8_t ip_addr[4];
          NETBIOS_RESP *resp = (NETBIOS_RESP*)netbios_tx_buf;
          /* prepare NetBIOS header response */
          resp->resp_hdr.trans_id      = netbios_hdr->trans_id;
          resp->resp_hdr.flags         = htons(NETB_HFLAG_RESPONSE |
                                               NETB_HFLAG_OPCODE_NAME_QUERY |
                                               NETB_HFLAG_AUTHORATIVE |
                                               NETB_HFLAG_RECURS_DESIRED);
          resp->resp_hdr.questions     = 0;
          resp->resp_hdr.answerRRs     = htons(1);
          resp->resp_hdr.authorityRRs  = 0;
          resp->resp_hdr.additionalRRs = 0;

          /* prepare NetBIOS header datas */
          memcpy( resp->resp_name.encname, netbios_name_hdr->encname, sizeof(netbios_name_hdr->encname));
          resp->resp_name.nametype     = netbios_name_hdr->nametype;
          resp->resp_name.type         = netbios_name_hdr->type;
          resp->resp_name.cls          = netbios_name_hdr->cls;
          resp->resp_name.ttl          = htonl(NETBIOS_NAME_TTL);
          resp->resp_name.datalen      = htons(sizeof(resp->resp_name.flags)+sizeof(resp->resp_name.addr));
          resp->resp_name.flags        = htons(NETB_NFLAG_NODETYPE_BNODE);
          getSIPR(ip_addr);
          memcpy(resp->resp_name.addr,ip_addr,4);
          //ip_addr_copy(resp->resp_name.addr, netif_default->ip_addr);
          
          /* send the NetBIOS response */
          sendto(NETBIOS_SOCK, (unsigned char*)resp, sizeof(NETBIOS_RESP), rem_ip_addr, rem_udp_port);

          //if(ConfigMsg.debug_flag) printf("NETBIOS Name Mantched!\r\n");
        }
      }
      
    }
    break;
  case SOCK_CLOSED:
    close(NETBIOS_SOCK);
    socket(NETBIOS_SOCK, Sn_MR_UDP, NETBIOS_PORT, 0);
    break;
  default:
    break;
  }
}

void set_netbios_devname(uint8_t * devname)
{
    a2A(devname, netname); // Init NETBIOS device name
}

