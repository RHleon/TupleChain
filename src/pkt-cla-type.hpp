#ifndef __PKT_CLA_TYPE__
#define __PKT_CLA_TYPE__

#include <inttypes.h>
#include <stdio.h>
#include <vector>


inline 
uint32_t ip_to_u(uint8_t * ip_addr);

struct pkt_cla_port{
  uint16_t port;
  uint8_t mask;
};

typedef struct pkt_cla_port pkt_cla_port;

struct pkt_classify{
  uint32_t s_addr;
  uint8_t sa_mask;
  uint32_t d_addr;
  uint8_t da_mask;
  pkt_cla_port s_port;
  pkt_cla_port d_port;
  uint8_t proto;
  uint8_t pr_mask;

  bool read(FILE* file, uint16_t &sp_left, uint16_t & sp_right, uint16_t & dp_left, uint16_t & dp_right)
  {
    uint8_t s_addr_str[4]={0, 0, 0, 0};
    uint8_t d_addr_str[4]={0, 0, 0, 0};
    char raw_buff[512]="";

    if(!fgets(raw_buff, 512, file)){
      return false;
    }
    else{
      sscanf(raw_buff, "@%hhu.%hhu.%hhu.%hhu/%hhu %hhu.%hhu.%hhu.%hhu/%hhu %hu : %hu %hu : %hu %hhx/%hhx", 
                                                                                    &s_addr_str[0], &s_addr_str[1], &s_addr_str[2], &s_addr_str[3] ,&sa_mask,
                                                                                    &d_addr_str[0], &d_addr_str[1], &d_addr_str[2], &d_addr_str[3] ,&da_mask,
                                                                                    &sp_left, &sp_right, 
                                                                                    &dp_left, &dp_right,
                                                                                    &proto, &pr_mask);
      
      pr_mask = (uint8_t)32;

      s_addr = ip_to_u(s_addr_str);
      d_addr = ip_to_u(d_addr_str);
      return true;
    }
  }
};

typedef struct pkt_classify pkt_classify;

struct pc_pkt{
  uint32_t s_addr;
  uint32_t d_addr;
  uint16_t s_port;
  uint16_t d_port;
  uint8_t proto;
};
typedef struct pc_pkt pc_pkt;

inline 
uint32_t ip_to_u(uint8_t * ip_addr){
  uint32_t res_ip = 0;

  for(int i=0; i<4; i++){
    res_ip = res_ip*256 + ip_addr[i];
  }

  return res_ip;
}

#endif

