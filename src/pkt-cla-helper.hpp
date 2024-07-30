#ifndef __PKT_CLA_HELPER__
#define __PKT_CLA_HELPER__

#include <inttypes.h>
#include <stdio.h>
#include <vector>

#include <pkt-cla-type.hpp>

extern std::vector< pkt_classify > pc_vec;

//api

//split one port scope
int split_one_port_range(std::vector<pkt_cla_port> & port_v, 
                          uint16_t & left, 
                          uint16_t & right);

//split pc rule
int split_pc_rules(pkt_classify & pc, 
                    uint16_t & sp_left, 
                    uint16_t & sp_right, 
                    uint16_t & dp_left, 
                    uint16_t & dp_right);

//read pc rule
int read_addr_data_raw(FILE* file, pkt_classify &pc, 
                        uint16_t &sp_left, 
                        uint16_t & sp_right, 
                        uint16_t & dp_left, 
                        uint16_t & dp_right);

//init packet classifiacation rule vector
int init_pc_vec(FILE * fp);

#endif