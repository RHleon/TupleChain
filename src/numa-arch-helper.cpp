#ifdef __linux__
#define _GUN_SOURCE
#include <numa-arch-helper.hpp>
#include <thread>
#include <iostream>
#include <sstream>
#include <sched.h> //sched_getcpu(), sched_setaffinty()
#include <string>// stoi
#include <errno.h>
#include <stdio.h>//popen pclose
#include <sys/types.h>//getpid()
#include <unistd.h>
#include <stack>
#include <vector>
#include <set>
#include <sstream>
#include <assert.h>
#include <string.h>//memset
#include <stdlib.h>//strtol()

namespace nm{

int numa_nb = 0;

std::vector< std::stack<int> > numa_cpu_vecs;
std::vector< std::set<int> > numa_cpu_sets;

//parse cpu ids
std::vector<int>
parse_cpu_id_from_cmd(const char * cmdline){
    std::vector<int> cpu_ids;
    char* temp_pos;
    int var_cpu1, var_cpu2;

    while(*cmdline != ' ' && *cmdline != '\n'){
        var_cpu1 = (int)strtol(cmdline, &temp_pos, 10);
        //single cpu id
        if(*temp_pos == ',' || *temp_pos == '\n'){
            cpu_ids.push_back(var_cpu1);
						if(*temp_pos != '\n')
           		cmdline = (temp_pos + 1);
						else
							cmdline = temp_pos;
        }
        //cpu id section
        else if(*temp_pos == '-'){
            cmdline = (temp_pos + 1);
            var_cpu2 = (int)strtol(cmdline, &temp_pos, 10);
            cmdline = temp_pos;
            for(int i = var_cpu1; i <= var_cpu2; i++){
                cpu_ids.push_back(i);
            }
        }
				else{
						cmdline = temp_pos;
				}
    }
    
    return cpu_ids;
}

//parse cpu env
int init_numa_cpu_env(){
    std::cout<<"parse numa arch env..."<<std::endl;

    //TODO
    FILE *cmd_stream = NULL;
    char buff[1024];
    memset(buff, 0 ,sizeof(buff));

    if((cmd_stream = popen("lscpu", "r")) == NULL ){
        perror("cmd parse error");
    }
    
    while(fgets(buff, sizeof(buff), cmd_stream) != NULL){
        char * pos;

        pos = strstr(buff, "NUMA node");

        if(pos != NULL){
            pos += sizeof("NUMA node") - 1;
            
            //get numa nodes number
            if(*pos == '('){
                pos = strstr(pos, ":");
                pos++;

                //delete bank
                while(*pos == ' ')
                    pos++;
                numa_nb = (int)strtol(pos, NULL, 10);
                // printf("NUMA nodes %d\n", numa_nb);
                memset(buff, 0, sizeof(buff));
            }
            //get cpu id of each numa node
            else{
                int cpu_id = (int)strtol(pos, NULL, 10);
                // printf("node %d\n", cpu_id);
                pos = strstr(pos, ":");
                pos++;

                //delete blank
                while(*pos == ' ')
                    pos++;
                // printf("%s\n", pos);

                //parse cpu ids, store them into stack and set
                std::vector<int> cpu_ids_temp = parse_cpu_id_from_cmd(pos);
                std::stack<int> cpu_stack_temp;
                std::set<int> cpu_set_temp;

                for(std::vector<int>::iterator iter = cpu_ids_temp.begin(); iter != cpu_ids_temp.end(); iter++){
                    cpu_stack_temp.push(*iter);
                }
                cpu_set_temp.insert(cpu_ids_temp.begin(), cpu_ids_temp.end());

                numa_cpu_vecs.push_back(cpu_stack_temp);
                numa_cpu_sets.push_back(cpu_set_temp);
            }

            continue;
        }
    }

    std::cout<<"done!"<<std::endl;

    pclose(cmd_stream);
    return 0;
}

int parse_numa_node(int cpu_id){

    //check which numa node the #cpu_id cpu belongs to
    for(int i = 0; i<numa_cpu_sets.size(); i++){
        if( numa_cpu_sets[i].find(i) != numa_cpu_sets[i].end() ){
            return i;
        }
    }
    return 0;
}

int get_suitable_cpu(int numa_node){
    int cpu_id = 1;

    cpu_id = numa_cpu_vecs[numa_node].top();
    numa_cpu_vecs[numa_node].pop();
    
    return cpu_id;
}

int thread_migrate(std::thread & m_thread){

    unsigned long long tid = 0;
    int cpu_id = 0;
    std::stringstream ss;

    //get main process cpu id
    cpu_id = sched_getcpu();

    //get thread id
    ss<<m_thread.get_id();
    ss>>tid;

    //get a cpu core in same numa node as main thread
    cpu_set_t cpu_set;
    int tar_cpu = get_suitable_cpu(parse_numa_node(cpu_id));

    //migrate thread to target cpu core
    CPU_ZERO(&cpu_set);
    CPU_SET(tar_cpu, &cpu_set);
    int res = pthread_setaffinity_np(tid, sizeof(cpu_set), &cpu_set);
    if(res != 0){
        perror("Affinity set error:");
    }
    assert(res == 0);

    return tar_cpu;
}

}//end of namespace nm

#endif
