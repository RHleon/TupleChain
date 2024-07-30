#include <pkt-cla-type.hpp>
#include <pkt-cla-helper.hpp>

int split_one_port_range(std::vector<pkt_cla_port> & port_v, uint16_t & left, uint16_t & right)
{
	uint8_t right_zero_nb = 0;
	uint32_t tmp_left = 0;
	pkt_cla_port tmp_pcp;

	if(left > right){
		return 0;
	}else if(left == right){
		tmp_pcp.port = left;
		tmp_pcp.mask = 16;
		port_v.push_back(tmp_pcp);
		return 1;
	}else{
		tmp_left = left;
		while(tmp_left < right){
			//calculate the trailing zero of tmp_left

			// printf("%u ", tmp_left);

			right_zero_nb = (uint8_t)__builtin_ctz(tmp_left );

#ifndef __linux__
			if(tmp_left == 0)
				right_zero_nb = (uint8_t)16;
#endif

			right_zero_nb = right_zero_nb > 16 ? 16 : right_zero_nb;

			// printf("%u ", right_zero_nb);

			while( tmp_left + (((1U)<<right_zero_nb) - 1) > right ){
				right_zero_nb--;
			}
			tmp_pcp.port = tmp_left;
			tmp_pcp.mask = 16 - right_zero_nb;

			// printf("(%u/%u), ", tmp_pcp.port, tmp_pcp.mask);

			port_v.push_back(tmp_pcp);
			tmp_left = tmp_left + ((1U)<<right_zero_nb);
		}
	}

	return (int)( port_v.size() );
}

int split_pc_rules(pkt_classify & pc, uint16_t & sp_left, uint16_t & sp_right, uint16_t & dp_left, uint16_t & dp_right)
{
	std::vector<pkt_cla_port> s_port_v;
	std::vector<pkt_cla_port> d_port_v;
	int i=0, j=0;
	int res;

	//split src and dst port scope
	// printf("port (%u, %u):\n", sp_left, sp_right);
	res = split_one_port_range(s_port_v, sp_left, sp_right);
	// printf("\n");
	// printf("ports from %hu to %hu were divided into %d sub-scope\n", sp_left, sp_right, res);

	// printf("port (%u, %u):\n", dp_left, dp_right);
	res = split_one_port_range(d_port_v, dp_left, dp_right);
	// printf("\n");
	// printf("ports from %hu to %hu were divided into %d sub-scope\n", dp_left, dp_right, res);
	
	//fill pkt_classify vector
	for(i=0; i < s_port_v.size(); i++){
		for(j=0; j < d_port_v.size(); j++){
			pc.s_port.port = s_port_v[i].port;
			pc.s_port.mask = s_port_v[i].mask;
			pc.d_port.port = d_port_v[i].port;
			pc.d_port.mask = d_port_v[i].mask;
			pc_vec.push_back(pc);
		}
	}

	return s_port_v.size() * d_port_v.size();
}

int read_addr_data_raw(FILE* file, pkt_classify &pc, uint16_t &sp_left, uint16_t & sp_right, uint16_t & dp_left, uint16_t & dp_right){
	if( pc.read(file, sp_left, sp_right, dp_left, dp_right) )
		return 1;
	else
		return 0;
}

int init_pc_vec(FILE * fp)
{
    uint16_t sp_left;
	uint16_t sp_right;
	uint16_t dp_left;
	uint16_t dp_right;
	pkt_classify pc;
	uint64_t count = 0;
	uint64_t ret = 0;

	//read one raw of packet classification rule
    while(read_addr_data_raw(fp, pc, sp_left, sp_right, dp_left, dp_right)){
		ret = split_pc_rules(pc, sp_left, sp_right, dp_left, dp_right);
		
		count += ret;
	}

    return (int)(count);
}
